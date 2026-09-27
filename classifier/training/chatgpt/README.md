# CAT / NO_CAT garden dataset script toolkit

This replaces the unreliable chat-only image-generation workflow with a controlled,
inspectable process.

## New target

- CAT training: **100**
- NO_CAT training: **120**
- CAT withheld test: **20**
- NO_CAT withheld test: **20**

The existing 20+20 test set is locked so the next training run can be compared honestly
with the previous result.

The previous approved 40 CAT + 40 NO_CAT training images are retained. Therefore the
new approval target is approximately:

- **60 additional CAT training masters**
- **80 additional NO_CAT training masters**

Rejected candidates never enter the build.

## Files

- `dataset_spec.yaml` — machine-readable governing rules.
- `unpack_garden_sources.py` — extracts/verifies the 16 supplied garden photographs.
- `generate_candidates.py` — plans or generates individual full-resolution candidates.
- `approve_candidates.py` — marks candidates approved/rejected.
- `build_dataset.py` — expands the old dataset while keeping the existing test set locked.
- `validate_dataset.py` — checks final counts, 192x192 grayscale format and exact duplicates.
- `candidate_manifest.csv` — created automatically when generation starts.
- `requirements.txt` — Python dependencies.

## 1. Setup on Windows / PowerShell

From this folder:

```powershell
py -m venv .venv
.\.venv\Scripts\Activate.ps1
py -m pip install -r requirements.txt
```

For API generation, set your OpenAI API key in the environment. Do not put the key
inside these scripts:

```powershell
$env:OPENAI_API_KEY="YOUR_KEY"
```

The generator uses the Image API's image-edit operation with `gpt-image-2`, so the
supplied real garden photograph is sent as the image input rather than asking the
model to invent a garden from text.

## 2. Put the real garden photographs in place

If you still have `Photos-1-001.zip`:

```powershell
py unpack_garden_sources.py "C:\path\to\Photos-1-001.zip"
```

The script verifies all 16 expected photographs.

## 3. Plan a small batch BEFORE spending anything

Example:

```powershell
py generate_candidates.py --cat 4 --no-cat 4 --seed 10
```

This prints the planned background/subject combinations. It does **not** call the API
and does not change the approval bank.

If the plan looks sensible, run exactly the same command with `--run`:

```powershell
py generate_candidates.py --cat 4 --no-cat 4 --seed 10 --run
```

Every API request produces one individual candidate image. The script never creates
contact sheets.

Generated masters appear under:

```text
candidates/
  cat/
  no_cat/
```

and are recorded as `pending` in `candidate_manifest.csv`.

## 4. Review and approve/reject

List pending images:

```powershell
py approve_candidates.py --list --status pending
```

Approve examples:

```powershell
py approve_candidates.py CAT_CAND_0001 CAT_CAND_0003 --status approved
```

Reject an example:

```powershell
py approve_candidates.py CAT_CAND_0002 --status rejected --note "cat scale too large"
```

The manifest is the authority. Rejected images stay excluded.

## 5. Repeat in manageable batches

Do not attempt all 140 new candidates at once. A sensible workflow is 8-16 candidates
per batch, review them, then continue.

The planner deliberately rotates use of the supplied garden photographs and varies
CAT coat/pose/direction/distance and NO_CAT distractor categories.

## 6. Build the expanded dataset

Keep the previous approved dataset ZIP available:

`EI192_APPROVED_40_40_TRAIN_20_20_TEST_UPRIGHT_20260927.zip`

Its expected SHA256 is stored in `dataset_spec.yaml`. The build stops if a different
ZIP is supplied unless you deliberately override the check.

Once at least 60 new CAT and 80 new NO_CAT candidates are approved:

```powershell
py build_dataset.py --existing "C:\path\to\EI192_APPROVED_40_40_TRAIN_20_20_TEST_UPRIGHT_20260927.zip"
```

This creates:

`EI192_EXPANDED_100CAT_120NOCAT_LOCKED_TEST.zip`

with exactly:

```text
cat_train/      100
no_cat_train/   120
cat_test/        20
no_cat_test/     20
```

The 20+20 test images come directly from the previous bundle and are not regenerated.

## 7. Validate before Edge Impulse

```powershell
py validate_dataset.py EI192_EXPANDED_100CAT_120NOCAT_LOCKED_TEST.zip
```

The validator checks:

- exact folder counts
- exactly 192x192
- grayscale mode
- readable images
- no exact duplicate hashes

## Important generation rules encoded in the YAML

CAT:
- 1-3 m
- grass only
- realistic scale/perspective/grounding
- multiple poses, directions and coat types
- no text or labels in pixels

NO_CAT:
- no cat anywhere
- birds, people/gardener, garden objects, natural shadows and empty garden
- no rabbit, dog, fox or hedgehog
- no isolated boots
- robin on grass is allowed; robin on birdbath is not
- daytime only

Most importantly: each generation is an **edit of one of the supplied garden
photographs**, not a text-only request to invent a generic garden.
