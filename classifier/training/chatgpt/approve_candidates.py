from __future__ import annotations
import argparse
from common import read_manifest, write_manifest

def main():
    ap = argparse.ArgumentParser(description="Review/update candidate approval state.")
    ap.add_argument("ids", nargs="*", help="Candidate IDs to update.")
    ap.add_argument("--status", choices=["approved", "rejected", "pending"])
    ap.add_argument("--note", default="")
    ap.add_argument("--list", action="store_true", help="List candidates, optionally filtered by --status.")
    args = ap.parse_args()

    rows = read_manifest()

    if args.list:
        for r in rows:
            if args.status and r["status"] != args.status:
                continue
            print(f"{r['candidate_id']:16} {r['class']:7} {r['status']:9} {r['subject_type']:28} {r['master_path']}")
        return

    if not args.ids or not args.status:
        raise SystemExit("For an update, provide one or more IDs and --status approved|rejected|pending.")

    wanted = set(args.ids)
    found = set()
    for r in rows:
        if r["candidate_id"] in wanted:
            r["status"] = args.status
            if args.note:
                r["notes"] = args.note
            found.add(r["candidate_id"])

    missing = wanted - found
    if missing:
        raise SystemExit("Unknown candidate ID(s): " + ", ".join(sorted(missing)))

    write_manifest(rows)
    print(f"Updated {len(found)} candidate(s) -> {args.status}")

if __name__ == "__main__":
    main()
