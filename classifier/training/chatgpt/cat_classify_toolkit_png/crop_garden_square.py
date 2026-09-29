from PIL import Image, ImageOps

from common import ROOT, IMAGE_EXTENSIONS


SOURCE_DIR = ROOT / "garden_photos"
OUTPUT_DIR = ROOT / "garden_photos_square"


def bottom_square(img):
    width, height = img.size
    side = min(width, height)

    left = (width - side) // 2
    top = height - side

    return img.crop((left, top, left + side, top + side))


def main():
    SOURCE_DIR.mkdir(exist_ok=True)
    OUTPUT_DIR.mkdir(exist_ok=True)

    count = 0

    for src in sorted(SOURCE_DIR.iterdir()):
        if not src.is_file() or src.suffix.lower() not in IMAGE_EXTENSIONS:
            continue

        with Image.open(src) as img:
            img = ImageOps.exif_transpose(img)
            original_size = img.size
            square = bottom_square(img)

            dst = OUTPUT_DIR / src.name

            kwargs = {}
            if src.suffix.lower() in {".jpg", ".jpeg"}:
                kwargs["quality"] = 95

            square.save(dst, **kwargs)

        print(
            f"{src.name}: {original_size[0]}x{original_size[1]} "
            f"-> {square.width}x{square.height}"
        )
        count += 1

    print(f"\nCreated {count} square garden photo(s).")


if __name__ == "__main__":
    main()
