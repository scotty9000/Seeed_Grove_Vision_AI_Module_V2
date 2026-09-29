import csv
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent

SPEC_FILE = ROOT / "dataset_spec.yaml"
MANIFEST = ROOT / "manifest.csv"

IMAGE_BANK = ROOT / "image_bank"
EI_192 = ROOT / "ei_192"

IMAGE_EXTENSIONS = {".jpg", ".jpeg", ".png", ".webp", ".bmp"}

MANIFEST_FIELDS = [
    "candidate_id",
    "class",
    "status",
    "source_background",
    "subject_type",
    "pose",
    "direction",
    "distance_m",
    "master_path",
]


def load_spec():
    if not SPEC_FILE.exists():
        raise FileNotFoundError(f"Missing {SPEC_FILE.name}")

    with SPEC_FILE.open("r", encoding="utf-8") as f:
        return yaml.safe_load(f)


def garden_dir():
    spec = load_spec()
    rel = spec["garden_sources"]["directory"]
    return ROOT / rel


def ensure_dirs():
    garden_dir().mkdir(exist_ok=True)
    IMAGE_BANK.mkdir(exist_ok=True)
    EI_192.mkdir(exist_ok=True)


def garden_sources():
    ensure_dirs()
    gdir = garden_dir()

    sources = sorted(
        p.name
        for p in gdir.iterdir()
        if p.is_file() and p.suffix.lower() in IMAGE_EXTENSIONS
    )

    minimum = int(load_spec()["garden_sources"].get("minimum_required", 1))

    if len(sources) < minimum:
        raise RuntimeError(
            f"Need at least {minimum} garden source image(s) in {gdir}"
        )

    return sources


def read_manifest():
    if not MANIFEST.exists():
        return []

    with MANIFEST.open("r", newline="", encoding="utf-8-sig") as f:
        return list(csv.DictReader(f))


def write_manifest(rows):
    ensure_dirs()
    tmp = MANIFEST.with_suffix(".tmp")

    with tmp.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=MANIFEST_FIELDS,
            extrasaction="ignore",
        )
        writer.writeheader()
        for row in rows:
            writer.writerow({k: row.get(k, "") for k in MANIFEST_FIELDS})

    tmp.replace(MANIFEST)
