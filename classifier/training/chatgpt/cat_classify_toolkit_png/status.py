from collections import Counter

from common import load_spec, read_manifest


def main():
    spec = load_spec()
    rows = read_manifest()

    targets = spec["targets"]
    baseline = spec["baseline_expected"]

    generated_target_cat = max(
        0,
        int(targets["cat_train"]) - int(baseline["cat_train"])
    )
    generated_target_no_cat = max(
        0,
        int(targets["no_cat_train"]) - int(baseline["no_cat_train"])
    )

    by_class = Counter(r.get("class", "") for r in rows)
    by_status = Counter(r.get("status", "") for r in rows)

    print(f"Generated-training target CAT:    {generated_target_cat}")
    print(f"Generated-training target NO_CAT: {generated_target_no_cat}")
    print()
    print(f"Manifest CAT rows:                {by_class['cat']}")
    print(f"Manifest NO_CAT rows:             {by_class['no_cat']}")
    print(f"Planned:                          {by_status['planned']}")
    print(f"Generated:                        {by_status['generated']}")


if __name__ == "__main__":
    main()
