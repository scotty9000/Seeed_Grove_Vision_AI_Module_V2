WE2 / Grove Vision AI V2 - Cat Classifier Build and Deployment
===============================================================

Project location
----------------
C:\Repos\Seeed_Grove_Vision_AI_Module_V2\classifier\firmware

This folder already contains the previous working .tflite model and
himax_vela.ini.

Current model
-------------
Edge Impulse image-classification model:

- MobileNetV2
- 160 x 160 input
- RGB input
- alpha 0.35
- 2 classes: cat / no_cat
- quantized int8 deployment model

Current Edge Impulse results used as a reference:

- Validation accuracy: 100%
- Held-out synthetic test accuracy: 94.74%
- Real-garden generated test accuracy: 97.48%

The final test is still the model running on the real WE2 camera.


IMPORTANT - USE THE VELA BUILD
------------------------------
Previously, a model uploaded directly from Edge Impulse / SenseCraft could run
for a few inferences and then crash.

The stable route was:

    Edge Impulse int8 .tflite
        -> compile with Arm Vela for the WE2 Ethos-U55
        -> *_vela.tflite
        -> upload the Vela model to the Grove Vision AI V2 / SenseCraft

Use the same known-good Vela environment and himax_vela.ini that worked before.
Do not upgrade or reinstall tools unless there is a reason to do so.


1. DLOAD THE MODEL FROM EDGE IMPULSE
-------------------------------------
In Edge Impulse Studio:

1. Open the cat / no_cat project.
2. Once Trained the model is available
3. Download the tensorflow lite (int8 quantized) model from the dashboard

Rename the file to something clear, for example:

    cat_mobilenetv2_160_rgb_int8.tflite

Copy it into:

    C:\Repos\Seeed_Grove_Vision_AI_Module_V2\classifier\firmware

Keep the original Edge Impulse model. Do not overwrite the previous known-good
model until the new one has been tested on hardware.


2. OPEN POWERSHELL IN THE FIRMWARE DIRECTORY
--------------------------------------------

    cd C:\Repos\Seeed_Grove_Vision_AI_Module_V2\classifier\firmware

Activate the same Python / Vela environment that was used for the previous
successful build, if required.

Check Vela is available:

    vela --version

If the previous working environment used a different executable name, use that
same executable rather than changing the environment.


3. COMPILE THE INT8 MODEL WITH VELA
-----------------------------------
Use the existing:

    himax_vela.ini

Typical PowerShell command:

    vela `
      --accelerator-config ethos-u55-64 `
      --config .\himax_vela.ini `
      --system-config My_Sys_Cfg `
      --memory-mode My_Mem_Mode_Parent `
      --optimise Size `
      --output-dir .\vela_output `
      .\cat_mobilenetv2_160_rgb_int8.tflite

The expected output file should be similar to:

    .\vela_output\cat_mobilenetv2_160_rgb_int8_vela.tflite

If the previous successful command differs from the example above, prefer the
previous known-good command and configuration.


4. CHECK THE VELA REPORT
------------------------
Before deploying the model, read the Vela summary.

Check that:

- compilation completed successfully
- the target is Ethos-U55-64
- all, or nearly all, operators are assigned to the NPU
- there are no unexpected unsupported operators
- there are no unexpected CPU fallbacks
- SRAM requirement is reasonable
- flash / model size is reasonable

The previous successful model had all operators on the NPU. Treat that as the
ideal result for this model as well.

If Vela reports unsupported operators or an unexpectedly high memory
requirement, stop and investigate before uploading the model.


5. RGB INPUT
------------
This model expects:

    160 x 160 x 3

That is RGB input.

This is different from the earlier MobileNetV1 grayscale model.

Compiling with Vela does not intentionally convert a 3-channel RGB model into a
1-channel grayscale model. The compiled model should still expect RGB input.

Do not convert the new MobileNetV2 model back to grayscale just because the old
model used grayscale.


6. UPLOAD THE VELA MODEL TO THE WE2
-----------------------------------
Upload the Vela-generated model, not the raw Edge Impulse model:

    cat_mobilenetv2_160_rgb_int8_vela.tflite

Use the same SenseCraft / Grove Vision AI V2 upload procedure that was stable
with the previous Vela-built model.

If SenseCraft asks for class labels, use the actual output order from Edge
Impulse.

Expected labels are:

    cat
    no_cat

Verify their exact order in Edge Impulse before entering them.


7. FIRST HARDWARE TEST - STABILITY
----------------------------------
Before judging classification accuracy, check that the new model is stable.

Run repeated inference continuously for several minutes.

Look specifically for the failure seen with the old raw EI deployment:

- no crash after a few inferences
- no tensor arena / allocation failure
- no unexpected reset
- no lock-up

If the model remains stable, move on to classification testing.


8. SECOND HARDWARE TEST - REAL CAMERA ACCURACY
----------------------------------------------
Test from the intended low garden camera position.

CAT examples should include:

- close cat
- medium-distance cat
- distant cat
- ginger / tabby / black / black-and-white / white where possible
- sitting
- walking
- crouching
- lying down
- partial grass occlusion

NO_CAT examples should include:

- empty grass
- birds
- crouching / kneeling gardener
- walking person / lower legs
- watering can
- rake
- wheelie bin
- shadows
- normal garden clutter

Crouching / kneeling people are especially important hard negatives because a
previous NO_CAT gardener image was the one error in a held-out test.


9. IF HARDWARE ACCURACY IS LOWER THAN EDGE IMPULSE
--------------------------------------------------
Do not immediately retrain the model.

First check:

- whether the camera feed is actually colour / RGB
- crop behaviour
- resize behaviour
- camera height
- subject scale
- lighting
- subject distance
- confidence threshold
- model stability

The real camera result is more important than the Edge Impulse validation
score.


10. KEEP BOTH ORIGINAL AND VELA MODELS
--------------------------------------
Keep both files:

    cat_mobilenetv2_160_rgb_int8.tflite
    cat_mobilenetv2_160_rgb_int8_vela.tflite

This allows comparison and makes it easy to rebuild with different Vela
settings later.

Also keep the previous known-good Vela model until the new MobileNetV2 model has
passed a longer stability test.


11. SUGGESTED FOLDER LAYOUT
---------------------------

C:\Repos\Seeed_Grove_Vision_AI_Module_V2\classifier\firmware
|
|-- himax_vela.ini
|-- cat_mobilenetv2_160_rgb_int8.tflite
|-- previous_known_good_model.tflite
|-- vela_output\
|   `-- cat_mobilenetv2_160_rgb_int8_vela.tflite
`-- README_WE2_BUILD.txt


12. IF VELA FAILS
-----------------
Before changing the toolchain, save:

- the exact Vela command used
- the full Vela console output
- Vela version
- the input .tflite filename
- the relevant himax_vela.ini settings

Compare those with the previous known-good build before reinstalling or
upgrading anything.


QUICK REFERENCE
---------------

PowerShell:

    cd C:\Repos\Seeed_Grove_Vision_AI_Module_V2\classifier\firmware

    vela `
      --accelerator-config ethos-u55-64 `
      --config .\himax_vela.ini `
      --system-config My_Sys_Cfg `
      --memory-mode My_Mem_Mode_Parent `
      --optimise Size `
      --output-dir .\vela_output `
      .\cat_mobilenetv2_160_rgb_int8.tflite

Then upload:

    .\vela_output\cat_mobilenetv2_160_rgb_int8_vela.tflite

using the same SenseCraft / Grove Vision AI V2 upload process that was stable
with the earlier Vela model.


FLASH TFLITE WITHOUT SenseCraft
====================================
python .\xmodem\xmodem_send.py `
  --port=COM9 `
  --baudrate=921600 `
  --protocol=xmodem `
  --model="C:\Repos\Seeed_Grove_Vision_AI_Module_V2\classifier\firmware\vela_output\cat_mobilenetv2_160_rgb_int8_vela.tflite 0x400000 0x00000"
