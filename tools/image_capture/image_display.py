import base64
import json
import os
import time
import cv2
import numpy as np
import serial

COM_PORT = "COM9"  # Update to match your Device Manager COM port
BAUD_RATE = 921600
SAVE_DIR = "dataset_images"

os.makedirs(SAVE_DIR, exist_ok=True)


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
    with serial.Serial(COM_PORT, BAUD_RATE, timeout=0.5) as ser:
        time.sleep(0.5)
        ser.reset_input_buffer()
        # Pause any running inference loop so sampling is responsive
        ser.write(b"AT+BREAK\r\n")
        time.sleep(0.2)
        ser.reset_input_buffer()

        print("Preview started. Press SPACE to save image, or Q / ESC to quit.")
        count = 0

        while True:
            frame = get_frame(ser)
            if frame is None:
                continue

            # Scale up 2x for easier viewing on a high-res monitor (raw save stays original size)
            display = cv2.resize(frame, (480, 480), interpolation=cv2.INTER_NEAREST)
            cv2.putText(
                display,
                f"Saved: {count} | [SPACE] Snap  [Q] Quit",
                (10, 25),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.55,
                (0, 255, 0),
                2,
            )
            cv2.imshow("Grove Vision AI V2 - Dataset Collector", display)

            key = cv2.waitKey(1) & 0xFF
            if key == ord(" "):
                filepath = os.path.join(SAVE_DIR, f"img_{int(time.time())}_{count:04d}.jpg")
                cv2.imwrite(filepath, frame)
                print(f"Saved: {filepath}")
                count += 1
            elif key in (ord("q"), 27):  # 'q' or ESC
                break

    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
