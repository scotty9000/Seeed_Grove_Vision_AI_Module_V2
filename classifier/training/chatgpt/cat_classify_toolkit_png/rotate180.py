from pathlib import Path
from PIL import Image, ImageOps
import sys

if len(sys.argv) != 2:
    print("Usage:")
    print(r'  python rotate180.py "C:\path\to\image_bank"')
    sys.exit(1)

image_bank = Path(sys.argv[1])

if not image_bank.exists():
    print(f"ERROR: Folder does not exist: {image_bank}")
    sys.exit(1)

output_root = image_bank.parent / f"{image_bank.name}_rotated"

folders = ["cat", "no_cat"]
extensions = {".jpg", ".jpeg", ".png", ".bmp", ".webp"}

total_count = 0
counts = {}

for folder_name in folders:
    source = image_bank / folder_name
    destination = output_root / folder_name

    if not source.exists():
        print(f"ERROR: Missing folder: {source}")
        sys.exit(1)

    destination.mkdir(parents=True, exist_ok=True)

    count = 0

    for file in source.iterdir():
        if not file.is_file() or file.suffix.lower() not in extensions:
            continue

        with Image.open(file) as im:
            # Correct any EXIF orientation first
            im = ImageOps.exif_transpose(im)

            # Rotate exactly 180 degrees
            im = im.transpose(Image.Transpose.ROTATE_180)

            output_file = destination / file.name

            if file.suffix.lower() in {".jpg", ".jpeg"}:
                im.save(output_file, quality=95)
            else:
                im.save(output_file)

        count += 1
        total_count += 1

    counts[folder_name] = count


print()
print("Rotation complete")
print("-----------------")
print(f"CAT images rotated:     {counts['cat']}")
print(f"NO_CAT images rotated:  {counts['no_cat']}")
print(f"TOTAL images rotated:   {total_count}")
print()
print(f"Output folder:")
print(output_root)
