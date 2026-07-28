#include <Arduino.h>
#include <Wire.h>
#include <Seeed_Arduino_SSCMA.h>

SSCMA AI;
unsigned long lastHeartbeatTime = 0;
const unsigned long heartbeatInterval = 2000; 

void setup() {
    Serial.begin(115200);
    delay(3000); // USB sync window
    Serial.println("\n[HARDWARE CHECK] Starting hardware link test...");

    // Start I2C Communication on the XIAO's default trace lines
    Wire.begin(); 
    Wire.setClock(400000); // 400kHz matches the high-speed data stream capabilities
    
    // Attempt to open dialogue with the camera board
    if (!AI.begin(&Wire)) {
        Serial.println("\n[❌ CRITICAL ERROR] Camera board not found on I2C bus!");
        Serial.println("[HELP] Verify the XIAO is pressed completely into the underside socket header.");
        while (1) { 
            delay(1000); 
        }
    }

    Serial.println("\n[I2C SUCCESS] Grove Vision AI V2 camera successfully detected!");
    Serial.println("System Ready. Listening for filtered hardware stream events...");
}

void loop() {
    // Fast evaluate current frame capture structure (no network payload yet)
    int status = AI.invoke(1, true, false);
    
    if (status == 0) { 
        int targetCount = AI.boxes().size();
        if (targetCount > 0) {
            int score = AI.boxes()[0].score; 
            Serial.print("[STREAM MATCH] Person spotted with confidence: ");
            Serial.print(score);
            Serial.println("%");
        }
    }

    // Steady background serial heartbeat
    unsigned long currentMillis = millis();
    if (currentMillis - lastHeartbeatTime >= heartbeatInterval) {
        Serial.print("[HEARTBEAT] Core running. Uptime: ");
        Serial.print(currentMillis / 1000);
        Serial.println("s");
        lastHeartbeatTime = currentMillis;
    }
    
    delay(60); 
}
