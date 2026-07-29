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

        if (status == 0) {
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
                Serial.println("[⚠️ WARNING] Invoke returned 0, but last_image() is completely EMPTY.");
            }
        } else {
            Serial.print("[❌ BUS ERROR] Step 2 request failed with code: ");
            Serial.println(status);
        }
    }
}
