#include <Arduino.h>
#include <Wire.h>
#include <Seeed_Arduino_SSCMA.h>

SSCMA AI;
unsigned long lastCheckTime = 0;
// We can now safely poll every 200ms for fast, snappy gesture detection
const unsigned long checkInterval = 200; 

int lastDetectedID = -1; // Tracks the previous gesture state to handle changes

void setup() {
    Serial.begin(115200);
    while(!Serial); 
    
    Serial.println("\n================================================");
    Serial.println("[⚡ HIGH-SPEED ENGINE] Optimized Dual-Speed Loop");
    Serial.println("================================================");

    Wire.begin(6, 7); 
    Wire.setClock(400000); 
    
    if (!AI.begin(&Wire)) {
        Serial.println("[❌ ERROR] Handshake failed.");
        while (1) { delay(1000); }
    }
    Serial.println("[SUCCESS] High-speed monitoring running...");
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastCheckTime >= checkInterval) {
        lastCheckTime = currentMillis;

        // 🌟 SPEED FIX 1: Send show = false for rapid real-time gesture tracking
        // This transaction is incredibly lightweight and clears the bus instantly
        int status = AI.invoke(1, false, false);

        if (status >= 0) {
            if (AI.boxes().size() > 0) {
                // 🌟 FIXED ARRAYS: Access elements out of index position 0 of the vector array
                int currentID = AI.boxes()[0].target;
                int confidence = AI.boxes()[0].score;

                if (confidence > 55) {
                    // 🌟 SPEED FIX 2: Only fetch image text when a NEW gesture event drops in
                    if (currentID != lastDetectedID) {
                        lastDetectedID = currentID; // Update state machine

                        Serial.println("\n------------------------------------------------");
                        Serial.print("[🎯 NEW GESTURE] Result: ");
                        if (currentID == 0) Serial.print("✋ PAPER! ");
                        else if (currentID == 1) Serial.print("✊ ROCK! ");
                        else if (currentID == 2) Serial.print("✌️ SCISSORS! ");
                        Serial.print("(Certainty: ");
                        Serial.print(confidence);
                        Serial.println("%)");

                        // Now that a change event is confirmed, execute a targeted image pull
                        Serial.println(" ➔ Fetching snapshot bytes via I2C...");
                        AI.invoke(1, false, true); 
                        
                        size_t payloadSize = AI.last_image().length();
                        Serial.print(" ➔ Image Payload Size: ");
                        Serial.print(payloadSize);
                        Serial.println(" characters.");
                        
                        Serial.print(" ➔ Data Preview: ");
                        Serial.print(AI.last_image().substring(0, 40));
                        Serial.println("...");
                    }
                }
            } else {
                // If no hands are visible, reset our tracker state
                if (lastDetectedID != -1) {
                    Serial.println("\n[-] Hand removed. Re-arming high-speed scanner.");
                    lastDetectedID = -1;
                }
            }
        }
    }
}
