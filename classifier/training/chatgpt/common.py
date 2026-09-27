from __future__ import annotations
from pathlib import Path
import csv
import hashlib
import yaml
from PIL import Image

ROOT = Path(__file__).resolve().parent
MANIFEST = ROOT / "candidate_manifest.csv"
MANIFEST_FIELDS = [
    "candidate_id", "class", "status", "source_background", "subject_type",
    "pose", "direction", "distance_m", "lighting", "master_path", "prompt", "notes"
]

def load_spec(path: Path | None = None) -> dict:
    path = path or (ROOT / "dataset_spec.yaml")
    with open(path, "r", encoding="utf-8") as f:
        return yaml.safe_load(f)

def ensure_manifest():
    if not MANIFEST.exists():
        with open(MANIFEST, "w", newline="", encoding="utf-8") as f:
            csv.DictWriter(f, fieldnames=MANIFEST_FIELDS).writeheader()

def read_manifest() -> list[dict]:
    ensure_manifest()
    with open(MANIFEST, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))

def write_manifest(rows: list[dict]):
    with open(MANIFEST, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=MANIFEST_FIELDS)
        w.writeheader()
        for row in rows:
            w.writerow({k: row.get(k, "") for k in MANIFEST_FIELDS})

def append_manifest(row: dict):
    ensure_manifest()
    with open(MANIFEST, "a", newline="", encoding="utf-8") as f:
        csv.DictWriter(f, fieldnames=MANIFEST_FIELDS).writerow(
            {k: row.get(k, "") for k in MANIFEST_FIELDS}
        )

def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()

def centre_square_grayscale(src: Path, dst: Path, size: int, quality: int):
    with Image.open(src) as im:
        im = im.convert("RGB")
        w, h = im.size
        side = min(w, h)
        left = (w - side) // 2
        top = (h - side) // 2
        im = im.crop((left, top, left + side, top + side))
        im = im.resize((size, size), Image.Resampling.LANCZOS)
        im = im.convert("L")
        dst.parent.mkdir(parents=True, exist_ok=True)
        im.save(dst, "JPEG", quality=quality)
