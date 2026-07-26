#include <Arduino.h>
#include <rpcWiFi.h>

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); } 
    delay(2000); 

    Serial.println("\n=========================================");
    Serial.println("     WIO TERMINAL LOCAL AIRWAVE SCAN     ");
    Serial.println("=========================================");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("[SCANNING] Hunting for available 2.4GHz networks...");
    int totalNetworks = WiFi.scanNetworks();
    
    if (totalNetworks == 0) {
        Serial.println("[WARNING] No 2.4GHz networks discovered in range.");
    } else {
        Serial.print("[SUCCESS] Discovered ");
        Serial.print(totalNetworks);
        Serial.println(" network(s) nearby:\n");

        for (int i = 0; i < totalNetworks; ++i) {
            // Print the SSID, Signal Strength (RSSI), and Encryption Type
            Serial.print("   "); Serial.print(i + 1); Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" ("); Serial.print(WiFi.RSSI(i)); Serial.print(" dBm)");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " [OPEN]" : " [SECURE]");
            delay(10);
        }
    }
    Serial.println("=========================================");
}

void loop() {
    delay(1000);
}
