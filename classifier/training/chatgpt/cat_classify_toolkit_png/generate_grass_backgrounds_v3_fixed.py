#!/usr/bin/env python3
# Generate varied synthetic grass-like backgrounds for classifier training.
#
# Run from your project folder:
#   py generate_grass_backgrounds.py
#
# Optional:
#   py generate_grass_backgrounds.py --count 200 --size 512 --seed 12345

import argparse
import csv
import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageEnhance, ImageFont


def clamp(a, lo=0, hi=255):
    return np.clip(a, lo, hi)


def multiscale_noise(rng, size, scales=(8, 20, 48, 96)):
    total = np.zeros((size, size), dtype=np.float32)
    weights = []

    for s in scales:
        small = rng.normal(0, 1, (s, s)).astype(np.float32)
        span = float(np.ptp(small))
        norm = (small - float(small.min())) / (span + 1e-6)
        im = Image.fromarray((norm * 255).astype(np.uint8))
        im = im.resize((size, size), Image.Resampling.BICUBIC)

        arr = np.asarray(im, dtype=np.float32) / 255.0
        arr = arr * 2.0 - 1.0

        weight = 1.0 / math.sqrt(s)
        total += arr * weight
        weights.append(weight)

    total /= sum(weights)
    return total


def add_blades(img, rng, density, blade_len, blade_width, brightness):
    draw = ImageDraw.Draw(img, "RGBA")
    w, h = img.size
    n = int(w * h * density)

    for _ in range(n):
        x = int(rng.integers(0, w))
        y = int(rng.integers(0, h))

        length = max(2, int(rng.normal(blade_len, blade_len * 0.35)))
        angle = rng.normal(-math.pi / 2, 0.35)

        x2 = x + math.cos(angle) * length
        y2 = y + math.sin(angle) * length

        alpha = int(rng.integers(25, 85))
        delta = int(rng.normal(0, brightness))

        if delta >= 0:
            col = (
                min(255, 40 + delta),
                min(255, 85 + delta),
                min(255, 25 + delta // 2),
                alpha,
            )
        else:
            d = abs(delta)
            col = (20, max(30, 70 - d), 15, alpha)

        draw.line((x, y, x2, y2), fill=col, width=max(1, blade_width))


def add_patch_mottling(img, rng, amount):
    overlay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay, "RGBA")
    w, h = img.size

    patches = int(10 + amount * 30)
    for _ in range(patches):
        cx = int(rng.integers(0, w))
        cy = int(rng.integers(0, h))
        rx = int(rng.integers(max(12, w // 25), max(21, w // 7)))
        ry = int(rng.integers(max(12, h // 25), max(21, h // 7)))

        kind = rng.choice(["dark", "light", "brown", "moss"])
        alpha = int(rng.integers(10, 45))

        if kind == "dark":
            fill = (15, 45, 10, alpha)
        elif kind == "light":
            fill = (115, 155, 60, alpha)
        elif kind == "brown":
            fill = (105, 85, 40, alpha)
        else:
            fill = (75, 110, 45, alpha)

        draw.ellipse((cx - rx, cy - ry, cx + rx, cy + ry), fill=fill)

    overlay = overlay.filter(
        ImageFilter.GaussianBlur(radius=max(4, img.size[0] // 45))
    )
    return Image.alpha_composite(img.convert("RGBA"), overlay).convert("RGB")


def add_soft_shadow(img, rng, strength):
    if strength <= 0:
        return img

    overlay = Image.new("L", img.size, 0)
    draw = ImageDraw.Draw(overlay)
    w, h = img.size

    # Generate two random points, then SORT the coordinates.
    # Pillow requires x0 <= x1 and y0 <= y1.
    xa = int(rng.integers(-w // 3, w // 2 + 1))
    xb = int(rng.integers(w // 2, w + w // 3 + 1))
    ya = int(rng.integers(-h // 4, h + 1))
    yb = int(rng.integers(0, h + h // 3 + 1))

    x0, x1 = sorted((xa, xb))
    y0, y1 = sorted((ya, yb))

    # Ensure a non-zero drawable ellipse.
    if x1 <= x0:
        x1 = x0 + 1
    if y1 <= y0:
        y1 = y0 + 1

    draw.ellipse((x0, y0, x1, y1), fill=int(110 * strength))
    overlay = overlay.filter(
        ImageFilter.GaussianBlur(radius=max(20, w // 12))
    )

    arr = np.asarray(img, dtype=np.float32)
    sh = np.asarray(overlay, dtype=np.float32) / 255.0
    factor = 1.0 - sh[..., None] * (0.55 * strength)
    arr *= factor

    return Image.fromarray(clamp(arr).astype(np.uint8))


def add_sparse_debris(img, rng, probability):
    if rng.random() > probability:
        return

    draw = ImageDraw.Draw(img, "RGBA")
    w, h = img.size
    colours = [
        (125, 90, 40, 110),
        (155, 120, 60, 90),
        (65, 55, 25, 100),
        (190, 170, 95, 70),
    ]

    count = int(rng.integers(3, 15))
    for _ in range(count):
        x = int(rng.integers(0, w))
        y = int(rng.integers(0, h))
        r = int(rng.integers(max(1, w // 300), max(4, w // 90)))
        colour = colours[int(rng.integers(0, len(colours)))]
        draw.ellipse((x - r, y - r, x + r, y + r), fill=colour)


def make_grass(seed, size):
    rng = np.random.default_rng(seed)

    style = rng.choice([
        "short_even",
        "patchy",
        "dark_damp",
        "dry_mixed",
        "mossy",
        "sun_shade",
        "rough",
    ])

    palettes = {
        "short_even": ((45, 105, 38), (95, 145, 62)),
        "patchy": ((45, 90, 32), (120, 135, 65)),
        "dark_damp": ((25, 75, 30), (70, 115, 50)),
        "dry_mixed": ((65, 95, 35), (145, 135, 70)),
        "mossy": ((45, 90, 38), (105, 125, 55)),
        "sun_shade": ((40, 95, 35), (135, 165, 70)),
        "rough": ((35, 85, 32), (110, 135, 55)),
    }

    c1, c2 = palettes[str(style)]
    mix = float(rng.uniform(0.2, 0.8))
    base = np.array(c1, dtype=np.float32) * (1 - mix) + np.array(c2, dtype=np.float32) * mix

    low = multiscale_noise(rng, size)
    fine = rng.normal(0, 1, (size, size)).astype(np.float32)

    contrast = float(rng.uniform(18, 42))
    fine_strength = float(rng.uniform(4, 14))

    arr = np.zeros((size, size, 3), dtype=np.float32)
    arr[:] = base

    arr[..., 0] += low * contrast * float(rng.uniform(0.6, 1.0))
    arr[..., 1] += low * contrast * float(rng.uniform(0.9, 1.25))
    arr[..., 2] += low * contrast * float(rng.uniform(0.45, 0.8))
    arr += fine[..., None] * fine_strength

    arr[..., 0] *= float(rng.uniform(0.90, 1.10))
    arr[..., 1] *= float(rng.uniform(0.92, 1.10))
    arr[..., 2] *= float(rng.uniform(0.88, 1.08))

    img = Image.fromarray(clamp(arr).astype(np.uint8))

    patch_amount = float(rng.uniform(0.2, 1.0))
    img = add_patch_mottling(img, rng, patch_amount)

    add_blades(
        img, rng,
        density=float(rng.uniform(0.0015, 0.0045)),
        blade_len=float(rng.uniform(size * 0.008, size * 0.022)),
        blade_width=1,
        brightness=int(rng.integers(10, 35)),
    )
    add_blades(
        img, rng,
        density=float(rng.uniform(0.004, 0.010)),
        blade_len=float(rng.uniform(size * 0.003, size * 0.010)),
        blade_width=1,
        brightness=int(rng.integers(6, 20)),
    )

    if str(style) == "sun_shade":
        shadow_strength = float(rng.uniform(0.0, 1.0))
    else:
        shadow_strength = float(rng.uniform(0.0, 0.45))

    img = add_soft_shadow(img, rng, shadow_strength)
    add_sparse_debris(img, rng, probability=float(rng.uniform(0.1, 0.55)))

    if rng.random() < 0.35:
        img = img.filter(
            ImageFilter.GaussianBlur(radius=float(rng.uniform(0.3, 1.1)))
        )

    img = ImageEnhance.Contrast(img).enhance(float(rng.uniform(0.88, 1.15)))
    img = ImageEnhance.Brightness(img).enhance(float(rng.uniform(0.82, 1.18)))
    img = ImageEnhance.Color(img).enhance(float(rng.uniform(0.72, 1.20)))

    params = {
        "style": str(style),
        "shadow_strength": round(shadow_strength, 3),
        "patch_amount": round(patch_amount, 3),
        "base_r": round(float(base[0]), 1),
        "base_g": round(float(base[1]), 1),
        "base_b": round(float(base[2]), 1),
    }
    return img, params


def make_preview(paths, out_path, cols=5, thumb=160):
    if not paths:
        return

    rows = math.ceil(len(paths) / cols)
    gap = 8
    label_h = 22

    sheet = Image.new(
        "RGB",
        (
            cols * (thumb + gap) + gap,
            rows * (thumb + label_h + gap) + gap,
        ),
        "white",
    )
    draw = ImageDraw.Draw(sheet)

    try:
        font = ImageFont.truetype("DejaVuSans.ttf", 12)
    except Exception:
        font = ImageFont.load_default()

    for i, p in enumerate(paths):
        with Image.open(p) as im:
            t = im.convert("RGB").resize(
                (thumb, thumb), Image.Resampling.BILINEAR
            )

        col = i % cols
        row = i // cols
        x = gap + col * (thumb + gap)
        y = gap + row * (thumb + label_h + gap)

        sheet.paste(t, (x, y))
        draw.text((x, y + thumb + 3), p.stem, fill="black", font=font)

    sheet.save(out_path, quality=90)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--count", type=int, default=100)
    ap.add_argument("--size", type=int, default=512)
    ap.add_argument("--seed", type=int, default=20261002)
    ap.add_argument("--output", default="grass_backgrounds")
    args = ap.parse_args()

    if args.count < 1:
        raise SystemExit("--count must be at least 1")
    if args.size < 128:
        raise SystemExit("--size should be at least 128")

    out = Path(args.output)
    out.mkdir(parents=True, exist_ok=True)

    master_rng = np.random.default_rng(args.seed)
    rows = []
    paths = []

    for i in range(1, args.count + 1):
        seed = int(master_rng.integers(0, 2**31 - 1))
        img, params = make_grass(seed, args.size)

        name = f"GRASS_BG_{i:04d}.png"
        path = out / name
        img.save(path, "PNG", optimize=True)
        paths.append(path)

        rows.append({
            "background_id": f"GRASS_BG_{i:04d}",
            "filename": name,
            "seed": seed,
            **params,
        })

    with (out / "grass_backgrounds.csv").open(
        "w", newline="", encoding="utf-8"
    ) as f:
        writer = csv.DictWriter(f, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)

    make_preview(paths[:min(25, len(paths))], out / "preview.jpg")

    print(f"Generated {args.count} grass backgrounds in: {out.resolve()}")
    print(f"Preview:  {out / 'preview.jpg'}")
    print(f"Metadata: {out / 'grass_backgrounds.csv'}")


if __name__ == "__main__":
    main()
