import argparse
import json
import re

from common import ROOT, garden_sources, read_manifest


LAST_BATCH = ROOT / "last_batch.json"
PLACEHOLDER_BG = "GRASS_BG_XXX"


def format_row(row):
    fields = [
        row.get("candidate_id", ""),
        row.get("subject_type", ""),
        row.get("pose", ""),
        row.get("direction", ""),
        row.get("distance_m", ""),
    ]
    return " | ".join(fields)


def load_last_batch():
    if not LAST_BATCH.exists():
        return None
    try:
        with LAST_BATCH.open("r", encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, json.JSONDecodeError):
        return None

    if not isinstance(data, dict):
        return None
    if not isinstance(data.get("candidate_ids"), list):
        return None
    if not data.get("background"):
        return None
    return data


def save_last_batch(background, batch):
    data = {
        "background": background,
        "candidate_ids": [row["candidate_id"] for row in batch],
        "synced": False,
    }
    with LAST_BATCH.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
        f.write("\n")


def candidate_number(candidate_id):
    match = re.search(r"(\d+)$", candidate_id or "")
    return int(match.group(1)) if match else None


def choose_first_background(batch, sources):
    # On the first run of the new workflow, try to start with the background
    # matching the first candidate number (e.g. CAT_0036 -> GRASS_BG_0036.png).
    number = candidate_number(batch[0].get("candidate_id", ""))
    if number is not None:
        wanted = f"GRASS_BG_{number:04d}.png"
        if wanted in sources:
            return wanted

    # If the manifest still contains a real background rather than the
    # placeholder, prefer that if it exists.
    manifest_bg = batch[0].get("source_background", "")
    if manifest_bg and manifest_bg != PLACEHOLDER_BG and manifest_bg in sources:
        return manifest_bg

    return sources[0]


def next_background(previous_background, sources):
    if previous_background in sources:
        index = sources.index(previous_background)
        return sources[(index + 1) % len(sources)]
    return sources[0]


def print_prompt(background, batch, planned_remaining):
    print("Copy/paste everything below into ChatGPT, and attach the named background image.\n")
    print("--- COPY FROM HERE ---")
    print(f"BACKGROUND FOR THIS WHOLE BATCH: {background}")
    print()
    print(
        "Use the attached background image as the ACTUAL background for every image "
        "in this batch. Use the SAME background for all outputs. Do not redraw, replace "
        "or invent the lawn; preserve its grass pattern, lighting, colour and texture as "
        "closely as possible."
    )
    print()
    print(
        "Generate ONE separate square colour master image for each line below. The camera "
        "is very low at grass height. Distance controls apparent size and vertical position: "
        "nearer subjects are larger/lower, farther subjects are smaller/higher. Add only the "
        "specified subject; NO_CAT lines must contain no cat. empty_grass means leave the "
        "background unchanged. Do not make a collage. Name each output exactly by candidate "
        "ID, ZIP the whole batch, and provide a clickable download link."
    )
    print()
    for row in batch:
        print(format_row(row))
    print("--- COPY TO HERE ---")
    print()
    print(f"Background selected: {background}")
    print(f"Batch: {len(batch)}")
    print(f"Planned remaining after this batch: {planned_remaining}")
    print(f"State saved to: {LAST_BATCH.name}")


def main():
    parser = argparse.ArgumentParser(
        description="Print the next planned batch and one background to paste into ChatGPT."
    )
    parser.add_argument(
        "-n",
        "--number",
        type=int,
        default=10,
        help="Batch size, 1 to 10 (default: 10)",
    )
    args = parser.parse_args()

    if not 1 <= args.number <= 10:
        raise SystemExit("Batch size must be between 1 and 10.")

    rows = read_manifest()
    planned = [r for r in rows if r.get("status") == "planned"]

    if not planned:
        print("No planned images remaining.")
        return

    sources = garden_sources()
    last = load_last_batch()

    # If the previous batch is still entirely planned, simply reprint it with
    # the same background. This prevents repeated next_batch.py runs from
    # silently advancing the background.
    if last:
        by_id = {r.get("candidate_id"): r for r in rows}
        ids = last["candidate_ids"]
        statuses = [by_id[cid].get("status") for cid in ids if cid in by_id]

        if len(statuses) == len(ids) and statuses and all(s == "planned" for s in statuses):
            batch = [by_id[cid] for cid in ids]
            background = last["background"]
            remaining = len(planned) - len(batch)
            print_prompt(background, batch, remaining)
            return

        if statuses and any(s == "planned" for s in statuses) and any(s != "planned" for s in statuses):
            raise SystemExit(
                "last_batch.json refers to a partially synced batch. "
                "Resolve that batch before requesting the next one."
            )

    batch = planned[: args.number]

    if last:
        background = next_background(last["background"], sources)
    else:
        background = choose_first_background(batch, sources)

    save_last_batch(background, batch)
    print_prompt(background, batch, len(planned) - len(batch))


if __name__ == "__main__":
    main()
