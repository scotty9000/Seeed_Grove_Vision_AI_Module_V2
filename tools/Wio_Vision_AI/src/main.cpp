#include <Arduino.h>
#include <Wire.h>
#include <Seeed_Arduino_SSCMA.h>

SSCMA AI;
unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 500; // Faster 500ms sampling for smooth gameplay

void setup() {
    Serial.begin(115200);
    while(!Serial); 
    
    Serial.println("\n================================================");
    Serial.println("[🎮 GAME INTERCEPT] Rock, Paper, Scissors Pro");
    Serial.println("================================================");

    Wire.begin(6, 7); 
    Wire.setClock(400000); 
    
    if (!AI.begin(&Wire)) {
        Serial.println("[❌ ERROR] Handshake failed over I2C layout.");
        while (1) { delay(1000); }
    }
    Serial.println("[SUCCESS] Live tracking ready. Show your move!");
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastCheckTime >= checkInterval) {
        lastCheckTime = currentMillis;

        // Execute inference pass (loops=1, filter=false, request_image=false)
        int status = AI.invoke(1, false, false);

        // 🌟 THE FIX: If status is 3 or there are no boxes, the frame is empty/covered.
        // We force a clear state immediately instead of letting old values linger.
        if (status == 3 || AI.boxes().size() == 0) {
            Serial.println("❌ No gesture detected (Empty or Covered Frame)");
        } 
        else if (AI.boxes().size() > 0) {
            
            // Look directly at the primary bounding box tracking array
            int targetID = AI.boxes()[0].target;
            int confidence = AI.boxes()[0].score;

            // Enforce a strict minimum certainty filter to prevent false detections
            if (confidence > 55) {
                Serial.print("[DETECTED] ");
                
                // 🌟 OFFICIAL SEEED GESTURE INDEX MAPPINGS:
                if (targetID == 0) {
                    Serial.print("✋ PAPER! ");
                } else if (targetID == 1) {
                    Serial.print("✊ ROCK! ");
                } else if (targetID == 2) {
                    Serial.print("✌️ SCISSORS! ");
                } else {
                    Serial.print("Unknown Item (ID: ");
                    Serial.print(targetID);
                    Serial.print(") ");
                }
                
                Serial.print("(Certainty: ");
                Serial.print(confidence);
                Serial.println("%)");
            } else {
                Serial.println("🤔 Reading unstable... hold your hand still.");
            }
        }
    }
}
