CAT CLASSIFY - GRASS DOMAIN-RANDOMISATION WORKFLOW
================================================

PURPOSE
-------

This is the current workflow for the CAT / NO_CAT classifier for the
Seeed Grove Vision AI Module V2.

The previous approach used a small number of real garden photographs as
repeated backgrounds. Edge Impulse Feature Explorer clustered strongly by
those background photographs, while CAT and NO_CAT were not well separated.

The current experiment therefore uses MANY different procedural grass
backgrounds and pairs each background across both classes.

The aim is to make the background a poor predictor of the label and force the
model to pay more attention to the foreground subject.

Important:

    Training:
        synthetic CAT / NO_CAT images on varied grass backgrounds

    Initial testing:
        real garden images and selected images from the archived old dataset

Do NOT mix the archived old garden dataset into this training set yet.


CURRENT DIRECTORY LAYOUT
------------------------

Recommended layout:

    cat_classify_toolkit_png/
        grass_backgrounds/
            GRASS_BG_0001.png
            GRASS_BG_0002.png
            ...
            GRASS_BG_0100.png
            grass_backgrounds.csv
            preview.jpg

        image_bank/
            CAT_0001.png
            NO_CAT_0001.png
            ...

        ei_192/

        manifest.csv
        common.py
        next_batch.py
        sync_manifest.py
        convert_to_192.py
        status.py

        generate_grass_backgrounds_v3_fixed.py

        old_image_bank.zip             optional archive / later test data

There is NO dataset_spec.yaml in the current workflow.

common.py has been changed so that the source-background directory is:

    grass_backgrounds/

The old garden_photos/ and garden_photos_square/ workflow is not used for this
grass-domain-randomisation training experiment.


1. GENERATE PROCEDURAL GRASS BACKGROUNDS
----------------------------------------

The current generator is:

    generate_grass_backgrounds_v3_fixed.py

Run:

    py .\generate_grass_backgrounds_v3_fixed.py

Default output:

    100 backgrounds
    512 x 512 colour PNG
    grass_backgrounds/

The backgrounds deliberately vary in:

    grass colour
    brightness
    patchiness
    damp / dry appearance
    mottling
    fine texture
    broad shadows
    sparse small debris

The randomisation is intentional.

The objective is NOT to create one perfect-looking lawn.

The objective is to create enough different grass appearances that the
classifier cannot rely on the exact background texture.

The generator uses a fixed seed by default, so the same numbered backgrounds
can be recreated if necessary.


2. PAIRED BACKGROUND DESIGN
---------------------------

The manifest contains 100 CAT rows and 100 NO_CAT rows.

Each procedural background is used once for CAT and once for NO_CAT:

    GRASS_BG_0001 -> CAT_0001
    GRASS_BG_0001 -> NO_CAT_0001

    GRASS_BG_0002 -> CAT_0002
    GRASS_BG_0002 -> NO_CAT_0002

    ...

    GRASS_BG_0100 -> CAT_0100
    GRASS_BG_0100 -> NO_CAT_0100

This pairing is important.

Because the exact same background appears in both classes, the background
itself should have little or no correlation with CAT versus NO_CAT.

The manifest format remains:

    candidate_id,class,status,source_background,subject_type,pose,direction,distance_m,master_path

Example:

    CAT_0001,cat,planned,GRASS_BG_0001.png,ginger,crouching,left,2.4,
    NO_CAT_0001,no_cat,planned,GRASS_BG_0001.png,branch_shadow,,,,

The existing distance_m column is currently used as an approximate subject
distance / apparent-size cue for CAT generation. It does not need to be treated
as physically exact geometry on the procedural background.


3. IMAGE BANK
-------------

The current image_bank/ should contain ONLY the new grass-experiment masters.

The previous image_bank was archived before starting this experiment.

That prevents sync_manifest.py from seeing old CAT_#### / NO_CAT_#### files and
incorrectly marking new manifest rows as generated.

Generated masters go into:

    image_bank/

Examples:

    image_bank/CAT_0001.png
    image_bank/NO_CAT_0001.png

Keep the generated masters as square colour PNG files.

Do not convert to 192 x 192 grayscale yet.

Keep the colour masters until the complete training set has been generated.


4. GET THE NEXT BATCH
---------------------

Run:

    py .\next_batch.py

Default batch size is 10 rows.

Because each background is paired CAT + NO_CAT, a 10-row batch normally uses
5 grass background files.

Example:

    CAT_0006 | GRASS_BG_0006.png | fluffy_black_and_white | squatting_defecating | diagonal_left | 1.1
    NO_CAT_0006 | GRASS_BG_0006.png | crow_single |  |  |
    CAT_0007 | GRASS_BG_0007.png | black | crouching | right | 1.6
    NO_CAT_0007 | GRASS_BG_0007.png | crow_pair |  |  |

Paste the output into ChatGPT.


5. CHATGPT IMAGE-GENERATION PROMPT
----------------------------------

For a new chat, upload the GRASS_BG_####.png files referenced by the batch
before pasting the prompt.

For a 10-row batch this is normally only 5 source backgrounds.

If the relevant grass backgrounds are already available in the current chat,
they do not need to be uploaded again.

Use the following prompt text before the candidate lines:


----- COPY PROMPT BELOW -----

Use the named procedural grass images (GRASS_BG_####.png) as the ACTUAL
reference backgrounds for these training images.

Generate ONE SEPARATE square colour master image for EACH candidate line.

IMPORTANT BACKGROUND RULES:
- Edit the named GRASS_BG_####.png background for that line.
- Do not replace it with a generic lawn.
- Preserve the individual grass colour, texture, mottling, brightness and
  shadow pattern of the named background as much as practical.
- The same numbered grass background is deliberately used for one CAT and one
  NO_CAT image, so background identity must not become associated with class.
- Do not make a collage or contact sheet.

STRICT PERSPECTIVE / DEPTH RULES:
- The camera is very low, close to grass height, looking across a lawn that recedes into the distance.
- Treat the grass as a ground plane with depth.
- IMPORTANT: vertical image position MUST control apparent distance and scale.
- Subjects placed higher in the image are farther away and must appear smaller.
- Subjects placed lower in the image are closer and must appear larger.
- Do not place large distant subjects near the top of the frame.
- Keep substantial foreground grass in the lower part of the image.
- Most subjects should sit in the upper-middle part of the frame unless they are intentionally near.
- Approximate distance cue from the prompt must affect both subject size and placement:
  - around 1.0–1.5 m: larger, somewhat lower in frame
  - around 1.6–2.2 m: medium size, mid to upper-middle
  - around 2.3–3.0 m: smaller, higher in frame

CAT LINES:
- Add the specified cat type.
- Follow the specified pose and direction.
- Use the final numeric field as an approximate distance / apparent-size cue.
- Vary subject position naturally; do not always place cats in the centre.
- Keep the full cat visible unless realistic partial occlusion by grass occurs.
- Do not add extra animals or objects.

NO_CAT LINES:
- Add only the specified non-cat subject or condition.
- For empty_grass, leave the named grass background unchanged.
- For bird/tool/person/shadow entries, keep them plausible for a garden lawn.
- Do not add a cat.

OUTPUT:
- Return each image separately.
- Name each file exactly by candidate ID, for example:
      CAT_0011.png
      NO_CAT_0011.png
- Put all generated images for the batch into one ZIP file and provide a
  download link.

----- COPY PROMPT ABOVE -----

Then paste the candidate lines immediately below it.


6. IMPORTANT GENERATION PRINCIPLES
----------------------------------

The new training set should vary in BOTH foreground subject and background.

CAT variation includes:

    coat colour / pattern
    pose
    direction
    apparent size / distance
    position in frame

NO_CAT hard negatives include examples such as:

    empty grass
    blackbird
    pigeon
    magpie
    crow
    robin
    thrush
    rake
    watering can
    wheelie bin
    walking person's lower body
    gardener
    crouching gardener
    strong sun / shade boundary
    branch shadow
    tree shadow

CAT and NO_CAT should both use the same overall grass-background distribution.

Do NOT create a situation where:

    CAT    = one kind of grass
    NO_CAT = another kind of grass

The paired manifest is specifically designed to prevent that.


7. ADD THE DOWNLOADED BATCH TO IMAGE_BANK
-----------------------------------------

Download the ZIP returned by ChatGPT.

Extract the PNG files into:

    image_bank/

For example:

    image_bank/CAT_0006.png
    image_bank/NO_CAT_0006.png
    ...
    image_bank/CAT_0010.png
    image_bank/NO_CAT_0010.png

Check that the filenames exactly match the candidate IDs.


8. SYNC THE MANIFEST
--------------------

Run:

    py .\sync_manifest.py

If the corresponding image exists in image_bank/, sync_manifest.py changes:

    planned

to:

    generated

and fills master_path.

Example:

    CAT_0006: planned -> generated (CAT_0006.png)
    NO_CAT_0006: planned -> generated (NO_CAT_0006.png)

There are no manual review states.

The current common.py does NOT require dataset_spec.yaml.


9. REPEAT UNTIL GENERATION IS COMPLETE
--------------------------------------

Repeat:

    py .\next_batch.py

    paste the batch into ChatGPT using the prompt above

    download the ZIP

    extract into image_bank/

    py .\sync_manifest.py

Continue until:

    py .\next_batch.py

reports:

    No planned images remaining.


10. CONVERT FOR EDGE IMPULSE
----------------------------

Do the final conversion AFTER the colour master dataset is complete.

Target Edge Impulse input format:

    192 x 192
    8-bit grayscale
    PNG

Derived images go into:

    ei_192/

Use:

    py .\convert_to_192.py

If the current copy of convert_to_192.py still expects the old
dataset_spec.yaml, update that script before the final conversion. The current
grass workflow itself no longer uses dataset_spec.yaml.

Do not delete the square colour masters after conversion.


11. EDGE IMPULSE TRAINING / FEATURE EXPLORER
--------------------------------------------

Upload the 192 x 192 grayscale derived images to Edge Impulse.

The important diagnostic is Feature Explorer.

The previous dataset showed strong clustering by the repeated garden source
photograph.

With this new dataset we want:

    much weaker clustering by individual background
    better CAT versus NO_CAT separation
    no obvious CAT / NO_CAT split caused only by lawn appearance

If the embeddings still cluster strongly by GRASS_BG number or lawn style,
stop and investigate before generating substantially more data.


12. TEST DATA - KEEP SEPARATE FROM TRAINING
-------------------------------------------

Initially keep the following OUT of the new training set:

    real empty garden photographs
    archived old image_bank images
    full real garden scenes
    other genuine camera frames

These are valuable because they are different from the synthetic procedural
grass training distribution.

Use them later as a generalisation / out-of-distribution test.

A useful progression is:

    1. Train on procedural grass CAT / NO_CAT.
    2. Test on real garden grass / empty garden images.
    3. Test selected archived hard negatives.
    4. Only then decide whether a small amount of real-garden fine-tuning is
       necessary.


13. STATUS
----------

Run:

    py .\status.py

Use status.py and manifest.csv to confirm the number of planned and generated
images before final conversion and Edge Impulse training.


14. CURRENT EXPERIMENT SUMMARY
------------------------------

Current intended training set:

    100 CAT
    100 NO_CAT
    100 different procedural grass backgrounds

Each GRASS_BG is paired across both classes.

The first batches were generated with a somewhat elevated viewpoint.

From subsequent batches onward, use the LOW, near-ground wildlife-camera
perspective in the copy prompt above.

Keeping a small amount of perspective variation is acceptable, but future
generation should better reflect the actual low-mounted deployment camera.


15. ARCHIVE / SAFETY
--------------------

Keep backups of:

    manifest.csv
    old_image_bank.zip
    grass_backgrounds/
    image_bank/

The old dataset should not be discarded.

It may be useful later for:

    hard-negative testing
    comparison with the new model
    out-of-distribution evaluation
    diagnosing false positives
