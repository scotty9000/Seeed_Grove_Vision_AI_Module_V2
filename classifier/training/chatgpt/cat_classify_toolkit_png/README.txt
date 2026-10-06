CAT CLASSIFY - CLEAN TOOLKIT (PNG WORKFLOW)
===========================================

Keep from the old project:

    garden_photos/
    garden_photos_square/

Place these toolkit files beside those folders.

Final layout:

    cat_classify/
        garden_photos/
        garden_photos_square/
        image_bank/
        ei_192/

        dataset_spec.yaml
        manifest.csv              created automatically
        common.py
        plan_candidates.py
        next_batch.py
        sync_manifest.py
        convert_to_192.py
        crop_garden_square.py
        status.py
        requirements.txt


INSTALL
-------

PowerShell:

    py -m pip install -r requirements.txt


DATASET SPEC
------------

dataset_spec.yaml is the authoritative configuration.

The supplied copy is your previous spec with these agreed changes:

    manual_approval_required: false
    garden_sources.directory: "garden_photos_square"
    dataset.final_format: png

The final EI images are now:

    192 x 192
    grayscale
    PNG


1. SQUARE GARDEN REFERENCES
---------------------------

If garden_photos_square/ is already correct, do nothing.

To rebuild the lower-square crops from garden_photos/:

    py crop_garden_square.py


2. PLAN THE COMPLETE GENERATED SET
----------------------------------

Run:

    py plan_candidates.py --seed 1

The script reads dataset_spec.yaml.

Because the test set is locked in baseline_expected, it plans only the
extra ChatGPT-generated training images needed to reach the training targets.

With the current spec:

    CAT train target       100
    baseline CAT train      40
    generated CAT needed    60

    NO_CAT train target    120
    baseline NO_CAT train   40
    generated NO_CAT needed 80

So a fresh manifest will contain 140 planned rows.

Re-running plan_candidates.py is safe: it fills only missing rows.


3. GET THE NEXT CHATGPT BATCH
-----------------------------

    py next_batch.py

Default batch size is 10.

Or:

    py next_batch.py -n 6

It prints lines such as:

    CAT_0001 | IMG_20260912_151316.jpg | fluffy_tabby | walking | right | 2.3
    NO_CAT_0001 | IMG_20260912_151313.jpg | blackbird |  |  |

Paste those lines into ChatGPT.

The referenced square garden photos must be available in the conversation.

Ask ChatGPT to generate:

    one square colour master per line
    using the named square garden photo as the actual reference
    named by candidate ID

    When sending a batch, use this exact pattern:
    1. upload the relevant square garden photos
    2. paste:
    Use the square garden photos (IMG_20260912_*) previously uploaded as the actual reference backgrounds.
    Generate one separate square colour master image per line.
    Do not make a collage or contact sheet.
    Preserve the background composition of the named square garden photo.
    Return separate images named by candidate ID in a zip file for download link.

    3. then paste the candidate lines


4. IMAGE BANK
-------------

Put the returned square colour masters into:

    image_bank/

Examples:

    image_bank/CAT_0001.png
    image_bank/NO_CAT_0001.png

The master can be PNG, JPG, JPEG or WEBP.
Do not resize it to 512 first.


5. UPDATE MANIFEST
------------------

    py sync_manifest.py

If image_bank/CAT_0001.png exists, CAT_0001 moves automatically from:

    planned

to:

    generated

and master_path is populated.

There are no review states.


6. MAKE EDGE IMPULSE COPIES
---------------------------

    py convert_to_192.py

This reads dataset_spec.yaml and creates:

    192 x 192
    8-bit grayscale
    PNG

in:

    ei_192/

Use:

    py convert_to_192.py --force

to rebuild existing derived images.


7. REPEAT
---------

    py next_batch.py
    generate batch in ChatGPT
    put masters into image_bank/
    py sync_manifest.py
    py convert_to_192.py

until:

    py next_batch.py

reports no planned images remaining.


8. STATUS
---------

    py status.py
