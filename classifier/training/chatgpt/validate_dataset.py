from __future__ import annotations
from pathlib import Path
import argparse
import hashlib
import tempfile
import zipfile
from PIL import Image
from common import load_spec

FOLDERS = ("cat_train", "no_cat_train", "cat_test", "no_cat_test")

def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(1024*1024), b""):
            h.update(b)
    return h.hexdigest()

def find_root(root):
    if all((root / f).is_dir() for f in FOLDERS):
        return root
    for d in [p for p in root.iterdir() if p.is_dir()]:
        if all((d / f).is_dir() for f in FOLDERS):
            return d
    raise RuntimeError("Could not find the four required dataset folders.")

def validate(root):
    spec = load_spec()
    expected = {
        "cat_train": int(spec["targets"]["cat_train"]),
        "no_cat_train": int(spec["targets"]["no_cat_train"]),
        "cat_test": int(spec["targets"]["cat_test"]),
        "no_cat_test": int(spec["targets"]["no_cat_test"]),
    }
    size = (int(spec["dataset"]["image_width"]), int(spec["dataset"]["image_height"]))

    errors, hashes = [], {}
    extra_dirs = sorted(p.name for p in root.iterdir() if p.is_dir() and p.name not in FOLDERS)
    if extra_dirs:
        errors.append("Unexpected folders: " + ", ".join(extra_dirs))

    for folder in FOLDERS:
        files = sorted(p for p in (root / folder).iterdir() if p.is_file())
        if len(files) != expected[folder]:
            errors.append(f"{folder}: expected {expected[folder]} files, found {len(files)}")
        for p in files:
            try:
                with Image.open(p) as im:
                    if im.size != size:
                        errors.append(f"{folder}/{p.name}: size {im.size}, expected {size}")
                    if im.mode != "L":
                        errors.append(f"{folder}/{p.name}: mode {im.mode}, expected L")
            except Exception as e:
                errors.append(f"{folder}/{p.name}: unreadable ({e})")
                continue
            h = sha256(p)
            if h in hashes:
                errors.append(f"Exact duplicate: {folder}/{p.name} == {hashes[h]}")
            else:
                hashes[h] = f"{folder}/{p.name}"

    if errors:
        print("VALIDATION FAILED")
        for e in errors:
            print(" -", e)
        raise SystemExit(1)
    print("VALIDATION PASSED")
    print("Counts: 100 CAT train, 120 NO_CAT train, 20 CAT test, 20 NO_CAT test")
    print("All images: 192x192 grayscale; no exact duplicate hashes.")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dataset", help="Dataset ZIP or extracted dataset directory.")
    args = ap.parse_args()
    p = Path(args.dataset).resolve()
    if p.is_dir():
        validate(find_root(p))
    else:
        with tempfile.TemporaryDirectory() as td:
            with zipfile.ZipFile(p) as z:
                z.extractall(td)
            validate(find_root(Path(td)))

if __name__ == "__main__":
    main()
