from __future__ import annotations
from pathlib import Path
import argparse
import base64
import json
import os
import random
import shutil

from common import ROOT, append_manifest, load_spec, read_manifest

def weighted_choice(rng, mapping):
    keys = list(mapping)
    weights = [float(mapping[k]) for k in keys]
    return rng.choices(keys, weights=weights, k=1)[0]

def least_used_source(rows, sources, cls, rng):
    counts = {s: 0 for s in sources}
    for row in rows:
        if row.get("class") == cls and row.get("source_background") in counts:
            counts[row["source_background"]] += 1
    low = min(counts.values())
    choices = [s for s, n in counts.items() if n == low]
    return rng.choice(choices)

def next_id(rows, cls):
    prefix = "CAT_CAND_" if cls == "cat" else "NO_CAT_CAND_"
    nums = []
    for r in rows:
        cid = r.get("candidate_id", "")
        if cid.startswith(prefix):
            try:
                nums.append(int(cid.split("_")[-1]))
            except ValueError:
                pass
    return f"{prefix}{(max(nums, default=0)+1):04d}"

def choose_distance(rng, spec):
    d = spec["cat"]["distance_m"]
    # Deliberately include a useful share of 2.5-3 m examples.
    if rng.random() < float(d.get("preferred_far_fraction", 0.30)):
        return round(rng.uniform(2.5, float(d["max"])), 1)
    return round(rng.uniform(float(d["min"]), 2.5), 1)

def cat_plan(rng, spec, rows):
    sources = spec["garden_sources"]["filenames"]
    source = least_used_source(rows, sources, "cat", rng)
    coat = rng.choice(spec["cat"]["coats"])
    pose = rng.choice(spec["cat"]["poses"])
    direction = rng.choice(spec["cat"]["directions"])
    distance = choose_distance(rng, spec)

    prompt = f"""IMAGE EDIT. Use the supplied garden photograph as the base image and preserve it as closely as possible.
Do not redesign, replace or invent a different garden. Preserve the camera viewpoint, framing, lawn, paving, walls,
bench, drainpipe, raised beds, plants and other existing structures. Preserve the source daylight and make the added
subject match that lighting.

Add exactly one realistic {coat.replace('_',' ')} domestic cat on the GRASS ONLY, approximately {distance:.1f} metres
from the camera. Pose: {pose.replace('_',' ')}. Direction: {direction.replace('_',' ')}.
Make the cat scale follow the lawn perspective and grass-blade scale. Paws/body must make believable contact with the
grass. No floating, sinking, halos, rectangular cut-out remnants or obvious compositing. No other animals or people.
Do not add text, labels, watermarks, borders or a contact-sheet layout. Produce one individual approval image."""
    return dict(source_background=source, subject_type=coat, pose=pose,
                direction=direction, distance_m=f"{distance:.1f}",
                lighting="match_source", prompt=prompt)

def no_cat_plan(rng, spec, rows):
    sources = spec["garden_sources"]["filenames"]
    source = least_used_source(rows, sources, "no_cat", rng)
    group = weighted_choice(rng, spec["no_cat"]["mix_weights"])

    pose = direction = distance = ""
    if group == "empty_garden":
        subtype = "empty_garden"
        prompt = ""
    elif group == "birds":
        subtype = rng.choice(spec["no_cat"]["birds"])
        prompt = f"""IMAGE EDIT. Use the supplied garden photograph as the base image and preserve it as closely as possible.
Do not redesign or substitute another garden. Preserve the original camera viewpoint, framing, structures and daylight.
Add a realistic {subtype.replace('_',' ')} naturally on the lawn/ordinary garden ground at a plausible garden distance.
Match perspective, scale, lighting and ground contact. There must be NO CAT anywhere in the image. No pasted rectangles,
halos or extraction artefacts. Do not add text, labels, watermarks, borders or a contact-sheet layout. Produce one
individual approval image."""
    elif group == "people":
        subtype = rng.choice(spec["no_cat"]["people"])
        prompt = f"""IMAGE EDIT. Use the supplied garden photograph as the base image and preserve it as closely as possible.
Do not redesign or substitute another garden. Preserve the original camera viewpoint, framing, structures and daylight.
Add one realistic {subtype.replace('_',' ')} naturally interacting with the garden. Any visible legs/feet must be attached
naturally to the person; no isolated boots. Match perspective, scale, lighting and ground contact. There must be NO CAT
anywhere in the image. No pasted rectangles, halos or extraction artefacts. Do not add text, labels, watermarks, borders
or a contact-sheet layout. Produce one individual approval image."""
    elif group == "objects":
        subtype = rng.choice(spec["no_cat"]["objects"])
        prompt = f"""IMAGE EDIT. Use the supplied garden photograph as the base image and preserve it as closely as possible.
Do not redesign or substitute another garden. Preserve the original camera viewpoint, framing, structures and daylight.
Add one realistic {subtype.replace('_',' ')} naturally placed in the garden at plausible scale and perspective. It must
be correctly grounded and lit. There must be NO CAT anywhere in the image. No pasted rectangles, halos or extraction
artefacts. Do not add text, labels, watermarks, borders or a contact-sheet layout. Produce one individual approval image."""
    else:
        subtype = rng.choice(spec["no_cat"]["shadows"])
        prompt = f"""IMAGE EDIT. Use the supplied garden photograph as the base image and preserve it as closely as possible.
Do not redesign or substitute another garden. Preserve the camera viewpoint and all existing structures. Add only a
realistic natural daytime {subtype.replace('_',' ')} consistent with the existing light. There must be NO CAT anywhere
in the image. Do not add animals, people, text, labels, watermarks, borders or a contact-sheet layout. Produce one
individual approval image."""

    return dict(source_background=source, subject_type=subtype, pose=pose,
                direction=direction, distance_m=distance, lighting="match_source",
                prompt=prompt)

def generate_one(plan, cls, cid, spec):
    src = ROOT / spec["garden_sources"]["directory"] / plan["source_background"]
    if not src.exists():
        raise FileNotFoundError(f"Missing garden source: {src}")

    out_dir = ROOT / "candidates" / cls
    out_dir.mkdir(parents=True, exist_ok=True)

    # Empty garden scenes are genuine source photographs: no model call needed.
    if plan["subject_type"] == "empty_garden":
        suffix = src.suffix.lower() if src.suffix else ".jpg"
        out = out_dir / f"{cid}_master{suffix}"
        shutil.copy2(src, out)
        return out

    if not os.getenv("OPENAI_API_KEY"):
        raise RuntimeError("OPENAI_API_KEY is not set. Run without --run to preview the plan.")

    from openai import OpenAI
    client = OpenAI()
    with open(src, "rb") as image_file:
        result = client.images.edit(
            model=spec["generation"]["model"],
            image=image_file,
            prompt=plan["prompt"],
            size=spec["generation"]["preview_size"],
            quality=spec["generation"]["preview_quality"],
            output_format=spec["generation"]["output_format"],
        )

    image_bytes = base64.b64decode(result.data[0].b64_json)
    out = out_dir / f"{cid}_master.png"
    out.write_bytes(image_bytes)
    return out

def main():
    ap = argparse.ArgumentParser(
        description="Plan or generate individual CAT / NO_CAT approval candidates from supplied garden photos."
    )
    ap.add_argument("--cat", type=int, default=0, help="Number of CAT candidates.")
    ap.add_argument("--no-cat", dest="no_cat", type=int, default=0, help="Number of NO_CAT candidates.")
    ap.add_argument("--seed", type=int, default=1, help="Deterministic planning seed.")
    ap.add_argument("--run", action="store_true", help="Actually call the image API. Without this, only print the plan.")
    args = ap.parse_args()
    if args.cat < 0 or args.no_cat < 0 or (args.cat + args.no_cat) == 0:
        raise SystemExit("Request at least one candidate with --cat N and/or --no-cat N.")

    spec = load_spec()
    rows = read_manifest()
    rng = random.Random(args.seed)

    planned = []
    for cls, count in (("cat", args.cat), ("no_cat", args.no_cat)):
        for _ in range(count):
            cid = next_id(rows + planned, cls)
            plan = cat_plan(rng, spec, rows + planned) if cls == "cat" else no_cat_plan(rng, spec, rows + planned)
            row = {
                "candidate_id": cid,
                "class": cls,
                "status": "pending",
                **plan,
                "master_path": "",
                "notes": "",
            }
            planned.append(row)

    if not args.run:
        print(json.dumps([
            {k: v for k, v in row.items() if k != "prompt"} for row in planned
        ], indent=2))
        print("\nPLAN ONLY: no images generated and manifest unchanged.")
        print("Re-run the same command with --run after reviewing the plan.")
        return

    for row in planned:
        out = generate_one(row, row["class"], row["candidate_id"], spec)
        row["master_path"] = str(out.relative_to(ROOT))
        append_manifest(row)
        print(f"generated {row['candidate_id']} -> {row['master_path']}")

if __name__ == "__main__":
    main()
