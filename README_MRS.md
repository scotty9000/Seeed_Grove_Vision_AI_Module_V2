Steps to compile and flash for the tflm_mb_cls example

cd .\EPII_CM55M_APP_S\
in makefile change
 APP_TYPE = tflm_mb_cls

cd C:\Repos\Seeed_Grove_Vision_AI_Module_V2\EPII_CM55M_APP_S
make clean
make

should generate an elf, copy it to
..\EPII_CM55M_APP_S\obj_epii_evb_icv30_bdv10\gnu_epii_evb_WLCSP65\EPII_CM55M_gnu_epii_evb_WLCSP65_s.elf input_case1_secboot\
then 
cd ..\we2_image_gen_local\
run this to gen an image
./we2_local_image_gen project_case1_blp_wlcsp.json
flash the image + (example) tflite to the we2
PS C:\Repos\Seeed_Grove_Vision_AI_Module_V2> python xmodem\xmodem_send.py --port=COM9 --baudrate=921600 --protocol=xmodem --file=we2_image_gen_local\output_case1_sec_wlcsp\output.img --model="model_zoo\tflm_mb_cls\qat_pruning_model_vela.tflite 0xB7B000 0x0000

b''
b'!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'
b'!!  Please keep the power on during the program upgrade process  !!'
b'!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!'
b''
b''
b'Send data using the xmodem protocol from your terminal'
xmodem_sending >> we2_image_gen_local\output_case1_sec_wlcsp\output.img
[██████████████████████████████] 100.00% 4480/4480 error: 0

xmodem_send bin file done!!
generate _temp_model_0_preamble_data.bin for model_zoo\tflm_mb_cls\qat_pruning_model_vela.tflite preamble data
xmodem_sending >> _temp_model_0_preamble_data.bin
[██████████████████████████████] 100.00% 1/1 error: 0

xmodem_send bin file done!!
xmodem_sending >> model_zoo\tflm_mb_cls\qat_pruning_model_vela.tflite
[██████████████████████████████] 100.00% 13318/13318 error: 0

xmodem_send bin file done!!
xmodem_send bin file result =  True
b'\x06'
b''
b'Do you want to end file transmission and reboot system? (y)'

Firmware upgrade completed, restart WE2 ...
