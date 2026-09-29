import argparse

from PIL import Image, ImageOps

from common import (
    EI_192,
    IMAGE_BANK,
    IMAGE_EXTENSIONS,
    load_spec,
)


def main():
    parser = argparse.ArgumentParser(
        description="Create Edge Impulse-ready PNG copies from generated masters."
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Overwrite existing derived images.",
    )
    args = parser.parse_args()

    spec = load_spec()
    ds = spec["dataset"]

    width = int(ds["image_width"])
    height = int(ds["image_height"])
    color_mode = str(ds["color_mode"]).lower()
    final_format = str(ds["final_format"]).lower()

    if width != height:
        raise RuntimeError(
            "This workflow expects a square Edge Impulse input."
        )

    if color_mode != "grayscale":
        raise RuntimeError(
            "This converter currently expects color_mode: grayscale."
        )

    if final_format != "png":
        raise RuntimeError(
            "This converter currently expects final_format: png."
        )

    IMAGE_BANK.mkdir(exist_ok=True)
    EI_192.mkdir(exist_ok=True)

    converted = 0
    skipped = 0
    warnings = 0

    for src in sorted(IMAGE_BANK.iterdir()):
        if not src.is_file() or src.suffix.lower() not in IMAGE_EXTENSIONS:
            continue

        dst = EI_192 / f"{src.stem}.png"

        if dst.exists() and not args.force:
            skipped += 1
            continue

        with Image.open(src) as img:
            img = ImageOps.exif_transpose(img)

            if img.width != img.height:
                print(
                    f"WARNING: skipping non-square master "
                    f"{src.name}: {img.width}x{img.height}"
                )
                warnings += 1
                continue

            img = img.resize(
                (width, height),
                Image.Resampling.LANCZOS,
            )

            img = img.convert("L")

            img.save(
                dst,
                format="PNG",
                optimize=True,
            )

        print(f"{src.name} -> {dst.name}")
        converted += 1

    print()
    print(f"Converted:       {converted}")
    print(f"Already existed: {skipped}")
    print(f"Warnings:        {warnings}")


if __name__ == "__main__":
    main()
