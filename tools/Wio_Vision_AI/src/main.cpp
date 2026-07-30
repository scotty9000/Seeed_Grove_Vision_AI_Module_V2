#include <Arduino.h>
#include <Wire.h>
#include <Seeed_Arduino_SSCMA.h>

SSCMA AI;
unsigned long lastCaptureTime = 0;
const unsigned long captureInterval = 1000; // Force capture exactly every 1 second
uint32_t captureCount = 0;

void setup() {
    Serial.begin(115200);
    delay(3000); // USB port sync window
    Serial.println("\n================================================");
    Serial.println("[DIAGNOSIS] Starting Isolated Step 2 Test Loop...");
    Serial.println("================================================");

    // Initialize I2C bus parameters
    Wire.begin(); 
    Wire.setClock(400000); // 400kHz high-speed configuration
    
    if (!AI.begin(&Wire)) {
        Serial.println("[❌ CRITICAL ERROR] Camera board not found on I2C bus!");
        while (1) { delay(1000); }
    }

    Serial.println("[SUCCESS] Hardware found. Beginning 1-second image stream...");
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastCaptureTime >= captureInterval) {
        lastCaptureTime = currentMillis;
        captureCount++;

        Serial.println("\n------------------------------------------------");
        Serial.print("[TEST] Dispatched Step 2 Capture #");
        Serial.println(captureCount);

        // Execute inference with filter=false and show=true (Request full image string)
        int status = AI.invoke(1, false, true);

        // Treat any return code >= 0 as a complete success
        if (status >= 0) {
            Serial.print("[SUCCESS] Step 2 request completed with code: ");
            Serial.println(status);
            
            // Extract and log tracking coordinates for verification
            int totalObjects = AI.boxes().size();
            Serial.print("[INFO] Vector confirmation size: ");
            Serial.print(totalObjects);
            Serial.println(" elements detected in frame.");

            for (int i = 0; i < totalObjects; i++) {
                int x      = AI.boxes()[i].x;      // X coordinate of box center
                int y      = AI.boxes()[i].y;      // Y coordinate of box center
                int w      = AI.boxes()[i].w;      // Width of bounding box
                int h      = AI.boxes()[i].h;      // Height of bounding box
                int score  = AI.boxes()[i].score;  // Confidence score (0-100%)
                int target = AI.boxes()[i].target; // Model class ID index

                Serial.print("  👉 Object #");
                Serial.print(i + 1);
                Serial.print(" [Class ID: ");
                Serial.print(target);
                Serial.print("] | Conf: ");
                Serial.print(score);
                Serial.print("% | Pos: (");
                Serial.print(x);
                Serial.print(", ");
                Serial.print(y);
                Serial.print(") Size: ");
                Serial.print(w);
                Serial.print("x");
                Serial.println(h);
            }

            // Image Payload Verification
            String imgData = AI.last_image();
            if (imgData.length() > 0) {
                Serial.print("[SUCCESS] Image fetched! Base64 String length: ");
                Serial.print(imgData.length());
                Serial.println(" characters.");
                
                // Print just the first 30 characters of the string to prove data exists without flooding serial
                Serial.print("[DATA PREVIEW] ");
                Serial.print(imgData.substring(0, 30));
                Serial.println("...");
            } else {
                Serial.print("[⚠️ WARNING] Invoke returned status ");
                Serial.print(status);
                Serial.println(", but last_image() data remains EMPTY over I2C.");
            }
        } else {
            Serial.print("[❌ BUS ERROR] Step 2 request failed with error code: ");
            Serial.println(status);
        }
    }
}
