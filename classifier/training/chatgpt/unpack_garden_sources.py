from pathlib import Path
import argparse
import zipfile
from common import ROOT, load_spec

def main():
    ap = argparse.ArgumentParser(description="Extract and verify the supplied real garden photographs.")
    ap.add_argument("zipfile", help="Path to Photos-1-001.zip")
    args = ap.parse_args()

    spec = load_spec()
    expected = set(spec["garden_sources"]["filenames"])
    out = ROOT / spec["garden_sources"]["directory"]
    out.mkdir(exist_ok=True)

    with zipfile.ZipFile(args.zipfile) as z:
        for member in z.namelist():
            name = Path(member).name
            if name in expected:
                with z.open(member) as src, open(out / name, "wb") as dst:
                    dst.write(src.read())

    present = {p.name for p in out.glob("*") if p.is_file()}
    missing = sorted(expected - present)
    if missing:
        raise SystemExit("Missing expected garden photos:\n  " + "\n  ".join(missing))
    print(f"OK: {len(expected)} supplied garden photographs are ready in {out}")

if __name__ == "__main__":
    main()
