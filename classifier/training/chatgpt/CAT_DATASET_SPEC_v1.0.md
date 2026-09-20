# Cat / No-Cat Dataset Specification
**Version:** 1.0  
**Purpose:** Master specification for building and validating the garden cat-detection dataset.

---

## 1. Governing rule

This document is the **master specification** for all future dataset generation.

Before generating, modifying, or packaging a dataset:

1. Read this specification.
2. Follow it exactly.
3. Do not silently change the method, image style, counts, exclusions, or quality standard.
4. If a requested change conflicts with this specification, update the specification first or explicitly note the agreed exception.
5. Do not substitute lower-quality images simply to reach the requested image count.

Quality takes priority over quantity.

---

# 2. Final dataset format

## Image format

- Resolution: **192 × 192 pixels**
- Colour: **grayscale**
- Final images should be clean and sharp enough for the camera use case.
- Do not add artificial blur, grain, noise, infrared effects, or night-camera effects.
- Images remain **upright**. Any 180° rotation required for the physical camera will be handled separately.

## Dataset size

### Training
- `cat_train/` — **40 images**
- `no_cat_train/` — **40 images**

### Test
- `cat_test/` — **20 images**
- `no_cat_test/` — **20 images**

Total: **120 images**

## Delivery format

Supply one ZIP bundle containing exactly:

```text
cat_train/
no_cat_train/
cat_test/
no_cat_test/
```

Do not place README files, manifests, preview sheets, or other files inside these four Edge Impulse image folders.

Supporting documentation may be included separately in the outer ZIP if useful.

---

# 3. Garden background source

Use the **supplied real garden photographs** as the background source.

The generated scenes should remain visually representative of the actual deployment environment.

Do not replace the supplied garden with unrelated generic gardens.

---

# 4. Image creation method

The normal production method is:

1. Start with one of the supplied garden photographs.
2. Add a cat or NO_CAT distractor to the garden scene.
3. Make the subject look naturally present in the scene.
4. Check scale, perspective, lighting, ground contact, and extraction quality.
5. Only after the full-resolution image has been approved, convert/crop it to **192 × 192 grayscale** for the dataset.

## Important

The **full-resolution source image is the quality-control master**.

Do not first reduce to 192 × 192 and then attempt to judge compositing quality.

---

# 5. Approved image bank workflow

Dataset images should normally come from an **approved source-image bank**.

## Candidate stage

Generate candidate images at a resolution **larger than 192 × 192**.

Each candidate should represent one genuinely distinct scene.

Candidates are reviewed before being allowed into the approved bank.

## Candidate decisions

Each candidate is classified as:

- **APPROVED**
- **REJECTED**
- **APPROVED WITH NOTE**

Examples of notes:

- good cat scale
- useful difficult lighting
- good crouching-person distractor
- subject slightly close but acceptable
- particularly useful bird example

## Approved-bank rule

Once an image is approved, it can be used as source material for dataset assembly.

Images that have not been approved should not be used simply to make up the required image count.

## No silent replacements

If there are not enough approved images to satisfy the requested dataset size, stop and create more candidate images for approval rather than filling the dataset with lower-quality material.

---

# 6. Independence and duplication rules

- Each dataset image should come from a **genuinely distinct source scene**.
- Do not create multiple dataset samples by slightly cropping, rotating, resizing, mirroring, or otherwise varying the same source image.
- Do not use near-identical source scenes merely to increase the image count.
- Training and test images must be **source-disjoint**.
- A source image used for training must not also be used to create a test image.
- No routine data augmentation is to be applied at this stage.

---

# 7. Lighting variation

Use realistic **daytime** lighting variation.

Include examples such as:

- sunny
- overcast
- partial shade
- strong sunlight
- strong natural shadows
- tree shadows
- mixed light and shade

Do not include:

- nighttime
- infrared
- artificial night-vision appearance
- unrealistic extreme image degradation

Lighting should remain believable for the supplied garden.

---

# 8. CAT class specification

## Distance

Cats should appear approximately **1 m to 3 m from the camera**.

## Location

Cats must be **on grass only**.

## Scale and perspective

Use the grass and scene perspective as the primary guide to cat size.

A cat farther away must:

- appear smaller,
- sit naturally within the scene perspective,
- be surrounded by grass whose apparent size is consistent with that distance.

A cat closer to the camera should appear correspondingly larger.

Do not create a small cat in foreground-scale grass or a large cat in a distant part of the garden.

## Ground contact

Cats must look physically grounded.

Reject images where the cat appears:

- floating,
- suspended,
- hovering above grass,
- sunk incorrectly into the ground,
- standing on an invisible platform,
- disconnected from the garden surface.

## Required pose variation

Include a useful spread of:

- sitting
- walking
- defecating
- lying down

## Direction variation

Include cats facing and/or moving in different directions:

- left
- right
- toward camera
- away from camera
- diagonal directions where natural

## Appearance variation

Use a range of cat appearances, including:

- tabby
- white
- black
- black and white
- fluffy / long-haired
- short-haired / non-fluffy
- other realistic coat patterns where useful

Avoid over-representing one cat appearance.

## CAT exclusions

Do not include:

- cats outside the intended 1–3 m range
- extremely tiny/distant cats
- airborne or mid-jump cats
- cats on fences, walls, roofs, furniture, paths, or other non-grass surfaces
- obviously pasted cats
- cats with bad masking/extraction edges
- cats with inconsistent lighting
- cats whose size does not match the scene perspective

---

# 9. NO_CAT class specification

The NO_CAT set should contain both:

- empty garden scenes
- garden scenes containing realistic distractors

The aim is to teach the classifier what can legitimately appear in the garden without being a cat.

---

# 10. NO_CAT distractor list

Include a useful spread of the following.

## People

- legs belonging to a person naturally present in the scene
- walking person / visible lower body
- crouching gardener

### Important

Do **not** include isolated boots or footwear on their own simply as a subject.

Footwear is acceptable only when naturally part of a person/gardener scene.

## Birds

Include:

- pigeon — single
- pigeons — pair
- magpie — single
- magpies — pair
- crow — single
- crows — pair
- blackbird
- robin
- thrush

Use different realistic positions and distances.

## Garden objects

Include:

- watering can
- wheelie bin
- rake

Objects must look naturally present in the garden.

## Shadows

Include difficult natural shadows, such as:

- tree shadow
- branch shadow
- strong sun/shade boundary

## Hedgehogs

Include:

- single hedgehog
- hedgehog with babies

## Distance variation

NO_CAT subjects should appear at different realistic distances within the garden.

Use perspective and surrounding grass/objects to determine believable scale.

---

# 11. NO_CAT exclusions

Do not include unless the specification is explicitly changed:

- rabbit
- dog
- fox
- nighttime scenes
- infrared scenes
- isolated boots
- badly extracted foreground objects
- subjects with rectangular or halo-like cut-out artefacts
- obviously pasted objects
- unrealistic floating objects
- unrelated generic garden backgrounds

---

# 12. Compositing quality requirements

Every composite must pass a visual realism check.

## Extraction quality

Reject any subject where:

- background remnants remain around the extracted object,
- a white/black/grey halo is visible,
- a rectangular source-image area is visible,
- edges are jagged or obviously artificial,
- transparency/masking is incomplete.

## Lighting

The subject lighting should broadly match the garden.

Check:

- light direction
- brightness
- contrast
- shadow strength
- subject exposure

## Perspective

The subject must match the camera viewpoint.

Reject subjects that look photographed from an incompatible angle.

## Grounding

Feet, paws, wheels, object bases, or bodies touching the ground must make believable contact with the grass/ground.

## Scale

Subject size must be consistent with its apparent location in the garden.

---

# 13. Full-resolution approval previews

The preview/approval image should be **larger than 192 × 192**.

The purpose of the preview is to judge:

- extraction quality
- scale
- grounding
- perspective
- lighting
- subject placement
- realism

An approved preview becomes part of the approved source bank.

The final 192 × 192 dataset image is derived from the approved high-resolution source.

Do not assume that a whole generated batch is approved because one or two examples were good.

Approval applies to individual images or explicitly approved groups.

---

# 14. Dataset balance guidance

The requested counts are fixed at:

| Class | Training | Test |
|---|---:|---:|
| CAT | 40 | 20 |
| NO_CAT | 40 | 20 |

Within those numbers, maintain useful variety rather than exact equal subcategory counts.

For example, the CAT set should contain multiple poses, distances, coat types, directions, and lighting conditions.

The NO_CAT set should contain a mixture of:

- empty garden
- people/gardener
- birds
- garden objects
- shadows
- hedgehogs

Do not allow one easy category, such as empty garden, to dominate the NO_CAT set.

---

# 15. Train/test split rules

Test images are **withheld evaluation images**.

Therefore:

- do not use the same source scene in train and test,
- do not use alternate crops of the same source scene across train/test,
- do not use a nearly identical recreation of a training image as a test image,
- test images should include genuinely difficult examples as well as straightforward ones.

---

# 16. File naming

Use simple sequential filenames.

Recommended:

```text
cat_train_0001.jpg
cat_train_0002.jpg
...

no_cat_train_0001.jpg
...

cat_test_0001.jpg
...

no_cat_test_0001.jpg
...
```

Filenames should not be used by Edge Impulse to infer labels.

The folders/classes are labelled manually as:

- `cat`
- `no_cat`

---

# 17. Final quality-control checklist

Before packaging, verify every image against the following checklist.

## Technical

- [ ] 192 × 192 pixels
- [ ] grayscale
- [ ] readable image file
- [ ] correct folder
- [ ] correct number of images
- [ ] no duplicate file hashes

## CAT

- [ ] cat present
- [ ] cat is approximately 1–3 m away
- [ ] cat is on grass
- [ ] realistic size for location
- [ ] believable ground contact
- [ ] no extraction artefacts
- [ ] no floating
- [ ] acceptable lighting
- [ ] useful pose/appearance diversity

## NO_CAT

- [ ] no cat present
- [ ] distractor is allowed by this specification
- [ ] no isolated boots
- [ ] no rabbit/dog/fox
- [ ] no nighttime/IR
- [ ] no bad cut-out artefacts
- [ ] realistic subject scale
- [ ] believable placement
- [ ] useful distractor variety

## Dataset-level

- [ ] 40 CAT training
- [ ] 40 NO_CAT training
- [ ] 20 CAT test
- [ ] 20 NO_CAT test
- [ ] train/test source-disjoint
- [ ] no near-duplicate source scenes
- [ ] varied lighting
- [ ] varied distance
- [ ] varied CAT poses/types/directions
- [ ] varied NO_CAT distractors

---

# 18. Packaging

Final ZIP structure:

```text
dataset.zip
├── cat_train/
│   ├── cat_train_0001.jpg
│   └── ...
├── no_cat_train/
│   ├── no_cat_train_0001.jpg
│   └── ...
├── cat_test/
│   ├── cat_test_0001.jpg
│   └── ...
└── no_cat_test/
    ├── no_cat_test_0001.jpg
    └── ...
```

No preview/contact-sheet images should be placed inside these four folders.

---

# 19. Change control

When this specification is changed:

- update the version number,
- record the change,
- use the newest agreed version for subsequent builds.

Suggested versioning:

- `v1.0` — initial agreed process
- `v1.1` — minor rule or distractor changes
- `v2.0` — major dataset/process change

---

# 20. Working principle

**Never lower the visual quality standard merely to reach the target image count.**

If there are not enough approved images:

1. state how many approved images are available,
2. generate additional full-resolution candidates,
3. obtain approval,
4. then assemble the dataset.

This approved-image-bank process is the default workflow for future iterations.
