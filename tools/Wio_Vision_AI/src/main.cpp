#include <Arduino.h>
#include <rpcWiFi.h> // Library must be included to open communication lines

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); } 
    delay(2000); 

    Serial.println("\n=========================================");
    Serial.println("   WIO TERMINAL: CO-PROCESSOR REGISTER   ");
    Serial.println("=========================================");
    Serial.println("[SYSTEM] Querying Realtek RTL8720 core...");

    // Fetch the version string directly from the secondary chip's hardware layer
    const char* version = rpc_system_version();

    Serial.println("\n------------- HARDWARE INFO -------------");
    if (version != NULL && strlen(version) > 0) {
        Serial.print("  -> Active Firmware Version: ");
        Serial.println(version);
        Serial.println("-----------------------------------------");
        Serial.println("\n[STATUS] Hardware communication is open!");
    } else {
        Serial.println("  -> Active Firmware Version: UNREADABLE / CRASHED");
        Serial.println("-----------------------------------------");
        Serial.println("\n[STATUS] System mismatch. Core firmware requires an update.");
    }
    Serial.println("=========================================");
}

void loop() {
    delay(1000);
}
