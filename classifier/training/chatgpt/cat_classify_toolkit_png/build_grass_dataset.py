#!/usr/bin/env python3
"""
Build a grass-focused 192x192 grayscale CAT / NO_CAT dataset
from the existing colour master image_bank.

Run from the project folder, e.g.:

    py .\build_grass_dataset.py

Defaults:
    manifest:   manifest.csv
    image bank: image_bank
    output:     grass_192

The script:
  * uses only generated CAT_#### / NO_CAT_#### masters
  * takes a lower-central square crop to suppress fences/paving/background structure
  * converts the crop to 192x192 grayscale PNG
  * keeps CAT and NO_CAT in separate folders
  * creates preview contact sheets for quick checking
  * writes grass_dataset.csv with exactly what was used

Your original colour masters are never modified.
"""

import argparse
import csv
import re
import shutil
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ID_RE = re.compile(r"^(?:CAT|NO_CAT)_\d+$", re.I)
SOURCE_RE = re.compile(r"(IMG_20260912_\d+\.(?:jpg|jpeg|png))", re.I)

IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".webp", ".bmp"}

# A lower-central crop removes much of the fixed garden geometry while
# retaining the lawn where the generated subjects normally appear.
DEFAULT_CROP_SCALE = 0.64
DEFAULT_BOTTOM_FRACTION = 0.97
TARGET_SIZE = 192


def normalise_key(s):
    return re.sub(r"[^a-z0-9]+", "", (s or "").lower())


def find_column(fieldnames, candidates):
    wanted = {normalise_key(x) for x in candidates}
    for f in fieldnames or []:
        if normalise_key(f) in wanted:
            return f
    return None


def candidate_from_row(row):
    # Prefer an explicitly named ID column.
    for key in ("candidate_id", "candidate", "id", "image_id", "name"):
        for actual in row:
            if normalise_key(actual) == normalise_key(key):
                value = str(row.get(actual, "")).strip()
                if ID_RE.fullmatch(value):
                    return value.upper()

    # Otherwise search all cells.
    for value in row.values():
        value = str(value or "").strip()
        if ID_RE.fullmatch(value):
            return value.upper()
    return None


def source_from_row(row):
    for value in row.values():
        m = SOURCE_RE.search(str(value or ""))
        if m:
            return m.group(1)
    return ""


def subject_from_row(row):
    preferred = (
        "subject", "subject_type", "object", "object_type",
        "negative_type", "scene", "description", "variant"
    )
    for key in preferred:
        for actual in row:
            if normalise_key(actual) == normalise_key(key):
                value = str(row.get(actual, "")).strip()
                if value:
                    return value
    return ""


def state_from_row(row):
    for key in ("state", "status"):
        for actual in row:
            if normalise_key(actual) == normalise_key(key):
                return str(row.get(actual, "")).strip().lower()
    return ""


def build_master_index(image_bank):
    index = {}
    for p in image_bank.rglob("*"):
        if p.is_file() and p.suffix.lower() in IMAGE_EXTS:
            stem = p.stem.upper()
            if ID_RE.fullmatch(stem):
                index.setdefault(stem, p)
    return index


def crop_box(width, height, crop_scale, bottom_fraction):
    side = int(round(min(width, height) * crop_scale))
    side = max(1, min(side, width, height))

    cx = width / 2.0
    left = int(round(cx - side / 2.0))
    left = max(0, min(left, width - side))

    bottom = int(round(height * bottom_fraction))
    bottom = max(side, min(bottom, height))
    top = bottom - side
    top = max(0, min(top, height - side))
    bottom = top + side

    return (left, top, left + side, bottom)


def grass_fraction(rgb_image):
    """
    Rough diagnostic only: fraction of pixels that look green-ish.
    It does NOT decide whether an image is accepted.
    """
    im = rgb_image.resize((128, 128))
    pixels = im.load()
    green = 0
    total = im.width * im.height
    for y in range(im.height):
        for x in range(im.width):
            r, g, b = pixels[x, y]
            if g >= 45 and g > r * 0.90 and g > b * 1.03:
                green += 1
    return green / total if total else 0.0


def load_font(size=15):
    for name in ("arial.ttf", "DejaVuSans.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except Exception:
            pass
    return ImageFont.load_default()


def make_preview(records, output_path, title, columns=6, thumb=150):
    if not records:
        return

    font = load_font(14)
    title_font = load_font(18)
    label_h = 42
    gap = 8
    header_h = 36

    rows = (len(records) + columns - 1) // columns
    width = columns * (thumb + gap) + gap
    height = header_h + rows * (thumb + label_h + gap) + gap

    sheet = Image.new("RGB", (width, height), "white")
    draw = ImageDraw.Draw(sheet)
    draw.text((gap, 7), title, fill="black", font=title_font)

    for i, rec in enumerate(records):
        col = i % columns
        row = i // columns
        x = gap + col * (thumb + gap)
        y = header_h + gap + row * (thumb + label_h + gap)

        with Image.open(rec["output"]) as im:
            preview = im.convert("RGB").resize((thumb, thumb), Image.Resampling.NEAREST)
        sheet.paste(preview, (x, y))

        label1 = rec["candidate_id"]
        label2 = f'green {rec["green_fraction"]:.0%}'
        draw.text((x, y + thumb + 2), label1, fill="black", font=font)
        draw.text((x, y + thumb + 20), label2, fill="black", font=font)

    sheet.save(output_path, quality=92)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", default="manifest.csv",
                        help="Manifest CSV (default: manifest.csv)")
    parser.add_argument("--image-bank", default="image_bank",
                        help="Master image folder (default: image_bank)")
    parser.add_argument("--output", default="grass_192",
                        help="Output folder (default: grass_192)")
    parser.add_argument("--crop-scale", type=float, default=DEFAULT_CROP_SCALE,
                        help=f"Square crop as fraction of image size (default: {DEFAULT_CROP_SCALE})")
    parser.add_argument("--bottom", type=float, default=DEFAULT_BOTTOM_FRACTION,
                        help=f"Crop bottom position as fraction of image height (default: {DEFAULT_BOTTOM_FRACTION})")
    args = parser.parse_args()

    manifest = Path(args.manifest)
    image_bank = Path(args.image_bank)
    output = Path(args.output)

    if not manifest.exists():
        raise SystemExit(f"Manifest not found: {manifest.resolve()}")
    if not image_bank.exists():
        raise SystemExit(f"Image bank not found: {image_bank.resolve()}")
    if not (0.25 <= args.crop_scale <= 1.0):
        raise SystemExit("--crop-scale should be between 0.25 and 1.0")
    if not (args.crop_scale <= args.bottom <= 1.0):
        raise SystemExit("--bottom must be >= --crop-scale and <= 1.0")

    # Rebuild derived dataset only. Masters stay untouched.
    if output.exists():
        shutil.rmtree(output)
    cat_dir = output / "CAT"
    no_cat_dir = output / "NO_CAT"
    cat_dir.mkdir(parents=True, exist_ok=True)
    no_cat_dir.mkdir(parents=True, exist_ok=True)

    master_index = build_master_index(image_bank)

    with manifest.open("r", newline="", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        rows = list(reader)

    records = []
    skipped_planned = []
    missing_master = []

    for row in rows:
        candidate_id = candidate_from_row(row)
        if not candidate_id:
            continue

        state = state_from_row(row)
        if state and state not in {"generated", "done", "complete", "completed", "ready"}:
            skipped_planned.append(candidate_id)
            continue

        master = master_index.get(candidate_id)
        if not master:
            missing_master.append(candidate_id)
            continue

        label = "NO_CAT" if candidate_id.startswith("NO_CAT_") else "CAT"
        out_dir = no_cat_dir if label == "NO_CAT" else cat_dir
        out_path = out_dir / f"{candidate_id}.png"

        with Image.open(master) as im:
            rgb = im.convert("RGB")
            box = crop_box(rgb.width, rgb.height, args.crop_scale, args.bottom)
            cropped_rgb = rgb.crop(box)
            gfrac = grass_fraction(cropped_rgb)

            final = cropped_rgb.convert("L").resize(
                (TARGET_SIZE, TARGET_SIZE),
                Image.Resampling.LANCZOS
            )
            final.save(out_path, "PNG", optimize=True)

        records.append({
            "candidate_id": candidate_id,
            "label": label,
            "source": source_from_row(row),
            "subject": subject_from_row(row),
            "master": str(master),
            "output": str(out_path),
            "crop_box": ",".join(map(str, box)),
            "green_fraction": gfrac,
        })

    records.sort(key=lambda r: r["candidate_id"])

    # Audit CSV
    audit_path = output / "grass_dataset.csv"
    with audit_path.open("w", newline="", encoding="utf-8") as f:
        fieldnames = [
            "candidate_id", "label", "source", "subject",
            "master", "output", "crop_box", "green_fraction"
        ]
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for rec in records:
            row = dict(rec)
            row["green_fraction"] = f'{row["green_fraction"]:.4f}'
            w.writerow(row)

    cat_records = [r for r in records if r["label"] == "CAT"]
    no_cat_records = [r for r in records if r["label"] == "NO_CAT"]

    make_preview(
        cat_records,
        output / "CAT_preview.jpg",
        "Grass-only CAT crops — check that every crop still contains the cat"
    )
    make_preview(
        no_cat_records,
        output / "NO_CAT_preview.jpg",
        "Grass-only NO_CAT crops — check that the intended hard negative remains visible"
    )

    print()
    print("Grass dataset built.")
    print(f"  CAT:     {len(cat_records)}")
    print(f"  NO_CAT:  {len(no_cat_records)}")
    print(f"  TOTAL:   {len(records)}")
    print()
    print(f"Output:    {output.resolve()}")
    print(f"Audit CSV: {audit_path.resolve()}")
    print()
    print("IMPORTANT: inspect CAT_preview.jpg and NO_CAT_preview.jpg before training.")
    print("If a subject has been cropped out, do NOT use that crop as labelled.")

    if missing_master:
        print()
        print(f"Missing master files: {len(missing_master)}")
        print("  " + ", ".join(missing_master[:20]))
        if len(missing_master) > 20:
            print("  ...")

    if skipped_planned:
        print()
        print(f"Skipped non-generated manifest rows: {len(skipped_planned)}")


if __name__ == "__main__":
    main()
