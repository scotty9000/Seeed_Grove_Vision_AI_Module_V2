import json
from pathlib import Path

from common import (
    IMAGE_BANK,
    IMAGE_EXTENSIONS,
    ROOT,
    garden_sources,
    read_manifest,
    write_manifest,
)


LAST_BATCH = ROOT / "last_batch.json"


def find_master(candidate_id):
    matches = [
        p for p in IMAGE_BANK.iterdir()
        if (
            p.is_file()
            and p.suffix.lower() in IMAGE_EXTENSIONS
            and p.stem == candidate_id
        )
    ]

    if not matches:
        return None

    if len(matches) > 1:
        names = ", ".join(sorted(p.name for p in matches))
        raise RuntimeError(
            f"More than one master image found for {candidate_id}: {names}"
        )

    return matches[0]


def load_last_batch():
    if not LAST_BATCH.exists():
        raise SystemExit(
            "last_batch.json was not found. Run next_batch.py first so the batch "
            "and nominated background are recorded."
        )

    try:
        with LAST_BATCH.open("r", encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, json.JSONDecodeError) as exc:
        raise SystemExit(f"Could not read {LAST_BATCH.name}: {exc}")

    if not isinstance(data, dict) or not data.get("background"):
        raise SystemExit(f"{LAST_BATCH.name} does not contain a valid background.")
    if not isinstance(data.get("candidate_ids"), list) or not data["candidate_ids"]:
        raise SystemExit(f"{LAST_BATCH.name} does not contain candidate IDs.")

    return data


def normalise_background(value):
    value = value.strip()
    if value and not Path(value).suffix:
        value += ".png"
    return value


def main():
    IMAGE_BANK.mkdir(exist_ok=True)
    rows = read_manifest()
    last = load_last_batch()

    candidate_ids = last["candidate_ids"]
    by_id = {row.get("candidate_id"): row for row in rows}

    missing_manifest = [cid for cid in candidate_ids if cid not in by_id]
    if missing_manifest:
        raise SystemExit(
            "These IDs from last_batch.json are missing from manifest.csv: "
            + ", ".join(missing_manifest)
        )

    statuses = [by_id[cid].get("status") for cid in candidate_ids]
    if all(status == "generated" for status in statuses):
        print("The last batch is already marked generated. No changes made.")
        return
    if any(status != "planned" for status in statuses):
        raise SystemExit(
            "The last batch has mixed/non-planned statuses. No changes made."
        )

    masters = {}
    missing_images = []
    for cid in candidate_ids:
        image = find_master(cid)
        if image is None:
            missing_images.append(cid)
        else:
            masters[cid] = image

    if missing_images:
        print("No manifest changes made. Missing image file(s) for:")
        for cid in missing_images:
            print(f"  {cid}")
        return

    nominated = last["background"]
    entered = input(f"Background image actually used [{nominated}]: ")
    background = normalise_background(entered) or nominated

    # Catch typing mistakes before changing the manifest.
    sources = garden_sources()
    if background not in sources:
        print(
            f"Background '{background}' was not found in the grass_backgrounds folder. "
            "No manifest changes made."
        )
        return

    changed = 0
    for cid in candidate_ids:
        row = by_id[cid]
        image = masters[cid]
        row["source_background"] = background
        row["status"] = "generated"
        row["master_path"] = image.relative_to(ROOT).as_posix()
        print(
            f'{cid}: planned -> generated | {background} | {image.name}'
        )
        changed += 1

    write_manifest(rows)

    # Keep the JSON as the cycle state. If the user entered a different BG,
    # the next run will continue from the background actually used.
    last["background"] = background
    last["synced"] = True
    with LAST_BATCH.open("w", encoding="utf-8") as f:
        json.dump(last, f, indent=2)
        f.write("\n")

    print(f"Updated {changed} manifest row(s).")
    print(f"Background recorded for this batch: {background}")


if __name__ == "__main__":
    main()
