from __future__ import annotations
from pathlib import Path
import argparse
import csv
import shutil
import tempfile
import zipfile

from common import ROOT, centre_square_grayscale, load_spec, read_manifest, sha256_file

FOLDERS = ("cat_train", "no_cat_train", "cat_test", "no_cat_test")

def images_in(folder: Path):
    return sorted(p for p in folder.iterdir() if p.is_file() and p.suffix.lower() in {".jpg",".jpeg",".png",".webp"})

def extract_existing(bundle: Path, temp: Path):
    with zipfile.ZipFile(bundle) as z:
        z.extractall(temp)
    # Allow either root/folders or a single wrapper directory.
    if all((temp / f).is_dir() for f in FOLDERS):
        return temp
    dirs = [p for p in temp.iterdir() if p.is_dir()]
    for d in dirs:
        if all((d / f).is_dir() for f in FOLDERS):
            return d
    raise RuntimeError("Existing ZIP does not contain the expected four dataset folders.")

def main():
    ap = argparse.ArgumentParser(description="Build expanded EI dataset while keeping the previous 20+20 test set locked.")
    ap.add_argument("--existing", required=True, help="Previous approved 40/40 train + 20/20 test ZIP.")
    ap.add_argument("--output", default="EI192_EXPANDED_100CAT_120NOCAT_LOCKED_TEST.zip")
    ap.add_argument("--ignore-existing-hash", action="store_true")
    args = ap.parse_args()

    spec = load_spec()
    old_zip = Path(args.existing).resolve()
    if not old_zip.exists():
        raise SystemExit(f"Existing dataset not found: {old_zip}")

    expected_hash = spec["existing_dataset"]["sha256"]
    actual_hash = sha256_file(old_zip)
    if expected_hash and actual_hash != expected_hash and not args.ignore_existing_hash:
        raise SystemExit(
            "Existing bundle SHA256 does not match the locked reference.\n"
            f"Expected: {expected_hash}\nActual:   {actual_hash}\n"
            "Use the original approved bundle or pass --ignore-existing-hash deliberately."
        )

    approved = [r for r in read_manifest() if r["status"] == "approved" and r["master_path"]]
    approved_cat = [r for r in approved if r["class"] == "cat"]
    approved_no = [r for r in approved if r["class"] == "no_cat"]

    target_cat = int(spec["targets"]["cat_train"])
    target_no = int(spec["targets"]["no_cat_train"])
    old_cat = int(spec["existing_dataset"]["cat_train_count"])
    old_no = int(spec["existing_dataset"]["no_cat_train_count"])
    need_cat = target_cat - old_cat
    need_no = target_no - old_no

    if len(approved_cat) < need_cat or len(approved_no) < need_no:
        raise SystemExit(
            "Not enough newly approved training candidates yet.\n"
            f"CAT: approved new {len(approved_cat)}, need {need_cat}\n"
            f"NO_CAT: approved new {len(approved_no)}, need {need_no}"
        )

    work = ROOT / "_build"
    if work.exists():
        shutil.rmtree(work)
    for f in FOLDERS:
        (work / f).mkdir(parents=True, exist_ok=True)

    size = int(spec["dataset"]["image_width"])
    quality = int(spec["dataset"]["jpeg_quality"])
    source_manifest = []

    with tempfile.TemporaryDirectory() as td:
        old_root = extract_existing(old_zip, Path(td))

        # Copy locked test set exactly, with sequential names.
        for cls in ("cat", "no_cat"):
            old_test = images_in(old_root / f"{cls}_test")
            expected = int(spec["targets"][f"{cls}_test"])
            if len(old_test) != expected:
                raise RuntimeError(f"Locked {cls}_test expected {expected}, found {len(old_test)}")
            for i, src in enumerate(old_test, 1):
                dst = work / f"{cls}_test" / f"{cls}_test_{i:04d}.jpg"
                shutil.copy2(src, dst)

        # Existing approved training images.
        for cls in ("cat", "no_cat"):
            old_train = images_in(old_root / f"{cls}_train")
            expected_old = int(spec["existing_dataset"][f"{cls}_train_count"])
            if len(old_train) != expected_old:
                raise RuntimeError(f"Existing {cls}_train expected {expected_old}, found {len(old_train)}")
            for i, src in enumerate(old_train, 1):
                dst = work / f"{cls}_train" / f"{cls}_train_{i:04d}.jpg"
                shutil.copy2(src, dst)
                source_manifest.append([dst.name, cls, "existing_approved_training", src.name])

    # Add new approved full-resolution masters, in approval/manifest order.
    for cls, rows, start, target in (
        ("cat", approved_cat, old_cat + 1, target_cat),
        ("no_cat", approved_no, old_no + 1, target_no),
    ):
        used_hashes = {sha256_file(p) for p in images_in(work / f"{cls}_train")}
        out_index = start
        for r in rows:
            if out_index > target:
                break
            src = ROOT / r["master_path"]
            if not src.exists():
                print(f"skip missing master: {r['candidate_id']} {src}")
                continue
            dst = work / f"{cls}_train" / f"{cls}_train_{out_index:04d}.jpg"
            centre_square_grayscale(src, dst, size, quality)
            h = sha256_file(dst)
            if h in used_hashes:
                dst.unlink()
                print(f"skip exact duplicate after conversion: {r['candidate_id']}")
                continue
            used_hashes.add(h)
            source_manifest.append([dst.name, cls, r["candidate_id"], r["source_background"]])
            out_index += 1
        if out_index <= target:
            raise SystemExit(f"Ran out of usable approved {cls} candidates; still need {target - out_index + 1}")

    # Separate source manifest - deliberately NOT included in the EI ZIP.
    manifest_path = ROOT / "FINAL_SOURCE_MANIFEST.csv"
    with open(manifest_path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["output_file","class","source_id","background_source"])
        w.writerows(source_manifest)

    out_zip = ROOT / args.output
    if out_zip.exists():
        out_zip.unlink()
    with zipfile.ZipFile(out_zip, "w", zipfile.ZIP_DEFLATED) as z:
        for folder in FOLDERS:
            for p in sorted((work / folder).iterdir()):
                z.write(p, f"{folder}/{p.name}")

    print(f"Built: {out_zip}")
    print(f"SHA256: {sha256_file(out_zip)}")
    print(f"Source manifest (outside ZIP): {manifest_path}")

if __name__ == "__main__":
    main()
