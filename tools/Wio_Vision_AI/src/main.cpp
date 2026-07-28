#include <Arduino.h>

unsigned long lastHeartbeatTime = 0;
const unsigned long heartbeatInterval = 1000; // Print every 1 second
uint32_t counter = 0;

void setup() {
    // Initialize the USB Serial port at 115200 baud
    Serial.begin(115200);
    
    // Safety delay to allow the PC to recognize the new XIAO COM port on boot
    delay(3000); 
    
    Serial.println("\n====================================");
    Serial.println("[SYSTEM] XIAO ESP32-C3 Boot Successful!");
    Serial.println("====================================");
}

void loop() {
    unsigned long currentMillis = millis();
    
    // Print a heartbeat every second
    if (currentMillis - lastHeartbeatTime >= heartbeatInterval) {
        counter++;
        Serial.print("[HEARTBEAT] XIAO C3 is running. Count: ");
        Serial.print(counter);
        Serial.print(" | Uptime: ");
        Serial.print(currentMillis / 1000);
        Serial.println("s");
        
        lastHeartbeatTime = currentMillis;
    }
}
