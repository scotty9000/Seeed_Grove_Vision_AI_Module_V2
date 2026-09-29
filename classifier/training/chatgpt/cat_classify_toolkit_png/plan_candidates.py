import argparse
import random

from common import (
    garden_sources,
    load_spec,
    read_manifest,
    write_manifest,
)


def next_id(rows, cls):
    prefix = "CAT_" if cls == "cat" else "NO_CAT_"
    nums = []

    for row in rows:
        cid = row.get("candidate_id", "")
        if cid.startswith(prefix):
            try:
                nums.append(int(cid.rsplit("_", 1)[1]))
            except ValueError:
                pass

    return f"{prefix}{max(nums, default=0) + 1:04d}"


def least_used(rows, cls, sources, rng):
    counts = {src: 0 for src in sources}

    for row in rows:
        if (
            row.get("class") == cls
            and row.get("source_background") in counts
        ):
            counts[row["source_background"]] += 1

    minimum = min(counts.values())
    choices = [src for src, count in counts.items() if count == minimum]
    return rng.choice(choices)


def weighted_choice(rng, weights):
    keys = list(weights)
    return rng.choices(
        keys,
        weights=[float(weights[k]) for k in keys],
        k=1,
    )[0]


def cat_row(rng, spec, rows, sources):
    src = least_used(rows, "cat", sources, rng)
    cat = spec["cat"]

    coat = rng.choice(cat["coats"])
    pose = rng.choice(cat["poses"])
    direction = rng.choice(cat["directions"])

    d = cat["distance_m"]
    min_d = float(d["min"])
    max_d = float(d["max"])
    far_fraction = float(d["far_fraction"])

    far_start = min(2.5, max_d)

    if rng.random() < far_fraction and far_start < max_d:
        distance = rng.uniform(far_start, max_d)
    else:
        distance = rng.uniform(min_d, far_start)

    return {
        "candidate_id": next_id(rows, "cat"),
        "class": "cat",
        "status": "planned",
        "source_background": src,
        "subject_type": coat,
        "pose": pose,
        "direction": direction,
        "distance_m": f"{distance:.1f}",
        "master_path": "",
    }


def no_cat_row(rng, spec, rows, sources):
    src = least_used(rows, "no_cat", sources, rng)
    no_cat = spec["no_cat"]

    group = weighted_choice(rng, no_cat["mix_weights"])

    if group == "empty_garden":
        subject = "empty_garden"
    else:
        subject = rng.choice(no_cat[group])

    return {
        "candidate_id": next_id(rows, "no_cat"),
        "class": "no_cat",
        "status": "planned",
        "source_background": src,
        "subject_type": subject,
        "pose": "",
        "direction": "",
        "distance_m": "",
        "master_path": "",
    }


def count_class(rows, cls):
    return sum(1 for row in rows if row.get("class") == cls)


def required_generated_train(spec):
    targets = spec["targets"]
    baseline = spec["baseline_expected"]

    if not baseline.get("lock_test_set", False):
        raise RuntimeError(
            "This planner assumes baseline_expected.lock_test_set is true."
        )

    cat_needed = max(
        0,
        int(targets["cat_train"]) - int(baseline["cat_train"])
    )
    no_cat_needed = max(
        0,
        int(targets["no_cat_train"]) - int(baseline["no_cat_train"])
    )

    return cat_needed, no_cat_needed


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Plan all ChatGPT-generated training candidates required by "
            "dataset_spec.yaml. Re-running is safe."
        )
    )
    parser.add_argument("--seed", type=int, default=1)
    args = parser.parse_args()

    spec = load_spec()
    sources = garden_sources()
    rows = read_manifest()
    rng = random.Random(args.seed)

    cat_target, no_cat_target = required_generated_train(spec)

    current_cat = count_class(rows, "cat")
    current_no_cat = count_class(rows, "no_cat")

    add_cat = max(0, cat_target - current_cat)
    add_no_cat = max(0, no_cat_target - current_no_cat)

    new_rows = []
    remaining_cat = add_cat
    remaining_no_cat = add_no_cat

    while remaining_cat or remaining_no_cat:
        if remaining_cat:
            new_rows.append(
                cat_row(rng, spec, rows + new_rows, sources)
            )
            remaining_cat -= 1

        if remaining_no_cat:
            new_rows.append(
                no_cat_row(rng, spec, rows + new_rows, sources)
            )
            remaining_no_cat -= 1

    if new_rows:
        write_manifest(rows + new_rows)

    print(f"Required generated CAT train images:    {cat_target}")
    print(f"Required generated NO_CAT train images: {no_cat_target}")
    print(f"Already in manifest - CAT:              {current_cat}")
    print(f"Already in manifest - NO_CAT:           {current_no_cat}")
    print(f"Added this run - CAT:                    {add_cat}")
    print(f"Added this run - NO_CAT:                 {add_no_cat}")
    print(f"Manifest rows now:                       {len(rows) + len(new_rows)}")


if __name__ == "__main__":
    main()
