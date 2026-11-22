import base64
import json
import os
import time
from tkinter import messagebox
import cv2
import numpy as np
import serial
import serial.tools.list_ports

BAUD_RATE = 921600
SAVE_DIR = "dataset_images"


def find_grove_port():
    for port in serial.tools.list_ports.comports():
        # CH343 USB-Serial VID/PID used by Grove Vision AI V2
        if port.vid == 0x1A86 and port.pid == 0x55D3:
            return port.device
        if "CH343" in (port.description or ""):
            return port.device
    return None


def get_frame(ser):
    ser.write(b"AT+SAMPLE=1\r\n")
    deadline = time.time() + 2.0
    while time.time() < deadline:
        line = ser.readline().decode("utf-8", errors="ignore").strip()
        if not line:
            continue
        try:
            payload = json.loads(line)
            if payload.get("name") == "SAMPLE" and "data" in payload:
                img_b64 = payload["data"].get("image")
                if img_b64:
                    img_bytes = base64.b64decode(img_b64)
                    np_arr = np.frombuffer(img_bytes, np.uint8)
                    return cv2.imdecode(np_arr, cv2.IMREAD_COLOR)
        except json.JSONDecodeError:
            continue
    return None


def main():
    port_name = find_grove_port()
    if not port_name:
        messagebox.showerror(
            "Device Not Found",
            "Grove Vision AI V2 not detected.\nCheck USB cable and ensure SenseCraft is closed.",
        )
        return

    os.makedirs(SAVE_DIR, exist_ok=True)

    try:
        ser = serial.Serial(port_name, BAUD_RATE, timeout=0.5)
    except serial.SerialException as e:
        messagebox.showerror("Port Busy", f"Could not open {port_name}:\n{e}")
        return

    with ser:
        time.sleep(0.5)
        ser.reset_input_buffer()
        ser.write(b"AT+BREAK\r\n")
        time.sleep(0.2)
        ser.reset_input_buffer()

        count = 0
        while True:
            frame = get_frame(ser)
            if frame is None:
                continue

            display = cv2.resize(frame, (480, 480), interpolation=cv2.INTER_NEAREST)
            cv2.putText(
                display,
                f"{port_name} | Saved: {count} | [SPACE] Snap [Q] Quit",
                (10, 25),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                (0, 255, 0),
                2,
            )
            cv2.imshow("Grove Vision AI V2 - Dataset Collector", display)

            key = cv2.waitKey(1) & 0xFF
            if key == ord(" "):
                filepath = os.path.join(SAVE_DIR, f"img_{int(time.time())}_{count:04d}.jpg")
                cv2.imwrite(filepath, frame)
                count += 1
            elif key in (ord("q"), 27):
                break

    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
