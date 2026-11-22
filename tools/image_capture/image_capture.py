import base64
import json
import os
import time
import serial

COM_PORT = "COM9"  # Change this to your Grove Vision AI V2 COM port
BAUD_RATE = 921600
SAVE_DIR = "dataset_images"

os.makedirs(SAVE_DIR, exist_ok=True)


def capture_frame(ser, filename):
    ser.reset_input_buffer()
    ser.write(b"AT+SAMPLE=1\r\n")

    deadline = time.time() + 5.0
    while time.time() < deadline:
        line = ser.readline().decode("utf-8", errors="ignore").strip()
        if not line:
            continue
        try:
            payload = json.loads(line)
            if payload.get("name") == "SAMPLE" and "data" in payload:
                img_b64 = payload["data"].get("image")
                if img_b64:
                    with open(filename, "wb") as f:
                        f.write(base64.b64decode(img_b64))
                    return True
        except json.JSONDecodeError:
            continue
    return False


def main():
    with serial.Serial(COM_PORT, BAUD_RATE, timeout=1) as ser:
        time.sleep(0.5)
        print(f"Connected to {COM_PORT}. Press Enter to snap a photo, or 'q' + Enter to quit.")

        count = 0
        while True:
            cmd = input(f"[{count} saved] > ").strip().lower()
            if cmd == "q":
                break

            filepath = os.path.join(SAVE_DIR, f"img_{int(time.time())}_{count:04d}.jpg")
            if capture_frame(ser, filepath):
                print(f"Saved: {filepath}")
                count += 1
            else:
                print("Timeout or no image received. Check camera ribbon cable.")


if __name__ == "__main__":
    main()
