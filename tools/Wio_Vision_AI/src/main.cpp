#include <Arduino.h>
#include <Wire.h>
#include <Seeed_Arduino_SSCMA.h>

SSCMA AI;
unsigned long lastCheckTime = 0;
// 🌟 TWEAK: Shifting to a fast 200ms sampling rate for instant gesture capture.
// Because the filter cuts repetitive traffic, this fast rate won't overwhelm the bus.
const unsigned long checkInterval = 200; 

void setup() {
    Serial.begin(115200);
    while(!Serial); 
    
    Serial.println("\n================================================");
    Serial.println("[🎮 FILTER MODE] Rock, Paper, Scissors Pro");
    Serial.println("================================================");

    Wire.begin(6, 7); 
    Wire.setClock(400000); 
    
    if (!AI.begin(&Wire)) {
        Serial.println("[❌ ERROR] Handshake failed over I2C layout.");
        while (1) { delay(1000); }
    }
    Serial.println("[SUCCESS] Filter engine armed. Show a gesture and hold it still!");
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastCheckTime >= checkInterval) {
        lastCheckTime = currentMillis;

        // 🌟 THE FILTER FIX:
        // Param 1: loops = 1
        // Param 2: filter = true  -> ONLY trigger a response if the gesture changes!
        // Param 3: show = false   -> Bypasses large image string loads
        int status = AI.invoke(1, true, false);

        // If the current frame matches the previous frame, status returns a non-success flag 
        // or the library skips loading new boxes, keeping the console completely clean.
        if (AI.boxes().size() > 0) {
            
            int targetID = AI.boxes()[0].target;
            int confidence = AI.boxes()[0].score;

            if (confidence > 55) {
                // Log the timestamp to prove it only updates when the hand moves
                Serial.print("[NEW EVENT @ ");
                Serial.print(millis() / 1000);
                Serial.print("s] -> ");

                if (targetID == 0) {
                    Serial.print("✋ PAPER! ");
                } else if (targetID == 1) {
                    Serial.print("✊ ROCK! ");
                } else if (targetID == 2) {
                    Serial.print("✌️ SCISSORS! ");
                } else {
                    Serial.print("Unknown Class (ID: ");
                    Serial.print(targetID);
                    Serial.print(") ");
                }
                
                Serial.print("(Certainty: ");
                Serial.print(confidence);
                Serial.println("%)");
            }
        }
    }
}
