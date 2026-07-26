#include <Arduino.h>
#include <rpcWiFi.h>  

// SECURE DEFINITIONS: These automatically pull from your secrets.ini file during compilation!
const char* ssid     = SECRET_SSID;     
const char* password = SECRET_PASS; 

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); } 
    delay(2000); 

    Serial.println("\n=========================================");
    Serial.println("  SECURE WI-FI DIAGNOSTIC CONNECTION    ");
    Serial.println("=========================================");
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.print("Connecting to secure network target...");
    WiFi.begin(ssid, password);

    int connectionTimer = 0;
    while (WiFi.status() != WL_CONNECTED && connectionTimer < 60) {
        delay(500);
        Serial.print(".");
        connectionTimer++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[SUCCESS] Connected cleanly to your hidden route!");
        Serial.print("  -> IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n[FAILURE] Connection failed. Check your secrets.ini settings.");
    }
}

void loop() {
    delay(1000);
}
