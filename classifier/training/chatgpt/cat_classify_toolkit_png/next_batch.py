import argparse

from common import read_manifest


def format_row(row):
    return (
        f'{row["candidate_id"]} | '
        f'{row["source_background"]} | '
        f'{row["subject_type"]} | '
        f'{row["pose"]} | '
        f'{row["direction"]} | '
        f'{row["distance_m"]}'
    )


def main():
    parser = argparse.ArgumentParser(
        description="Print the next planned instructions to paste into ChatGPT."
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

    batch = planned[: args.number]

    for row in batch:
        print(format_row(row))

    print()
    print(f"Batch: {len(batch)}")
    print(f"Planned remaining after this batch: {len(planned) - len(batch)}")


if __name__ == "__main__":
    main()
