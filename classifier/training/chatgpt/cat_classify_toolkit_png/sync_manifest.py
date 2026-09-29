from common import (
    IMAGE_BANK,
    IMAGE_EXTENSIONS,
    ROOT,
    read_manifest,
    write_manifest,
)


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


def main():
    IMAGE_BANK.mkdir(exist_ok=True)
    rows = read_manifest()
    changed = 0

    for row in rows:
        if row.get("status") != "planned":
            continue

        image = find_master(row["candidate_id"])
        if image is None:
            continue

        row["status"] = "generated"
        row["master_path"] = image.relative_to(ROOT).as_posix()

        print(
            f'{row["candidate_id"]}: planned -> generated '
            f'({image.name})'
        )
        changed += 1

    if changed:
        write_manifest(rows)

    print(f"Updated {changed} manifest row(s).")


if __name__ == "__main__":
    main()
