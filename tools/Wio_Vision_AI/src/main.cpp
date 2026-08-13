#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Seeed_Arduino_SSCMA.h>
#include "mbedtls/base64.h"
#include "secrets.h"

// CONFIGURATION PARAMETERS
const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;
const String botToken = SECRET_TOKEN;
const String chatID   = SECRET_ID;

// Static Buffer for Fragmentation Prevention
#define STATIC_BUFFER_SIZE 4096
unsigned char staticBinaryBuffer[STATIC_BUFFER_SIZE];

SSCMA AI;
unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 200; // Snappy 200ms camera scan rate

// ⏱️ ANTI-FLOOD LOCKOUT SYSTEM
const unsigned long ANTI_FLOOD_INTERVAL = 15000; 
unsigned long lockoutTimerStart = 0;             
bool isLockoutActive = false; 

// TIMEOUTS AND MONITORING
unsigned long lastHeartbeatTime = 0;

void sendTelegramJpgFile(const String& base64Str, const String& gestureName);

void setup() {
    Serial.begin(115200);
    
    // HEADLESS OPERATION WINDOW: Avoid freezes when running on garden battery
    unsigned long startWindow = millis();
    while (!Serial && (millis() - startWindow < 3000)) {
        delay(10);
    }
    
    if (Serial) {
        Serial.println("\n================================================");
        Serial.println("[🔋 STANDALONE PRODUCTION] Booting Stable Radio Rails...");
        Serial.println("================================================");
    }

    // Initialize I2C layers first
    Wire.begin(6, 7); 
    Wire.setClock(400000); 
    
    if (!AI.begin(&Wire)) {
        if (Serial) Serial.println("[❌ ERROR] Camera board not found over I2C.");
        while (1) { delay(1000); }
    }
    if (Serial) Serial.println("[SUCCESS] Camera board online over I2C.");
    delay(1000); // Allow rails to settle before turning on wireless engine

    // CLEAN HARDWARE WIRELESS RESET
    WiFi.disconnect(true); 
    delay(500);
    WiFi.mode(WIFI_STA);
    
    // 🌟 OPTIMIZATION: Throttling TX Power prevents internal C3 silicon reflections out in the garden
    WiFi.setTxPower(WIFI_POWER_11dBm); 
    
    WiFi.begin(ssid, password);
    
    int connectionTimeoutCounter = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (Serial) Serial.print(".");
        connectionTimeoutCounter++;
        
        if (connectionTimeoutCounter > 30) { 
            if (Serial) Serial.println("\n[⚠️ TIMEOUT] Resetting Wi-Fi adapter adapter...");
            WiFi.disconnect();
            delay(1000);
            WiFi.begin(ssid, password);
            connectionTimeoutCounter = 0;
        }
    }
    
    if (Serial) {
        Serial.println("\n[🎉 CONNECTED] Wi-Fi Link Established!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("RSSI Connection Strength: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    }
}

void loop() {
    unsigned long currentMillis = millis();

    // ⏱️ AUTOMATIC ANTI-FLOOD LOCKOUT RELEASE CHECKER
    if (isLockoutActive && (currentMillis - lockoutTimerStart >= ANTI_FLOOD_INTERVAL)) {
        Serial.println("\n[⏱️ LOCKOUT SYSTEM] Cooldown window expired. Alerts unlocked and ready.");
        isLockoutActive = false;
    }

    // IDLE HEARTBEAT: Only prints dots if the system is unlocked and waiting
    if (!isLockoutActive && (currentMillis - lastHeartbeatTime >= 3000)) {
        lastHeartbeatTime = currentMillis;
        Serial.print("."); 
    }

    if (currentMillis - lastCheckTime >= checkInterval) {
        lastCheckTime = currentMillis;

        // Run AI evaluation pass (Light metadata pass)
        int status = AI.invoke(1, true, false);

        if (status < 0) {
            Serial.print("\n[❌ I2C ERROR] Camera bus read failed. Status: ");
            Serial.println(status);
            return; 
        }

        // Check if the AI model found any matching target objects
        if (AI.boxes().size() > 0) {
            int currentID = AI.boxes()[0].target;
            int confidence = AI.boxes()[0].score;

            if (confidence > 45) {
                
                // 🛑 CASE 1: MATCH FOUND BUT LOCKOUT IS ACTIVE (Prints a down-counter)
                if (isLockoutActive) {
                    unsigned long timeElapsed = currentMillis - lockoutTimerStart;
                    unsigned long timeRemaining = (timeElapsed < ANTI_FLOOD_INTERVAL) ? (ANTI_FLOOD_INTERVAL - timeElapsed) : 0;
                    float remainingSeconds = timeRemaining / 1000.0;

                    Serial.println("\n------------------------------------------------");
                    Serial.print("[🛑 LOCKOUT ACTIVE] Target ID: "); Serial.print(currentID);
                    Serial.print(" (Conf: "); Serial.print(confidence); Serial.println("%) ignored.");
                    Serial.print(" ➔ Next upload available in: ");
                    Serial.print(remainingSeconds, 1);
                    Serial.println(" seconds.");
                    Serial.println("------------------------------------------------");
                    return; 
                }

                // 🚀 CASE 2: MATCH FOUND AND SYSTEM IS READY (No longer cares if it's the same gesture!)
                lockoutTimerStart = currentMillis; 
                isLockoutActive = true; 

                String gestureName = "";
                if (currentID == 0) gestureName = "PAPER";
                else if (currentID == 1) gestureName = "ROCK";
                else if (currentID == 2) gestureName = "SCISSORS";

                Serial.println("\n================================================");
                Serial.print("[🚀 ALERT TRIGGERED] Valid Match: "); Serial.println(gestureName);
                Serial.print("[📷 IMAGE CAPTURE] Fetching JPEG buffer frame...");
                Serial.println("\n================================================");

                // Pull the actual image payload now that the trigger is validated
                AI.invoke(1, false, true);
                String rawBase64 = AI.last_image();

                // Forward to your dummy network function
                sendTelegramJpgFile(rawBase64, gestureName); 
            }
        }
    }
}


// void sendTelegramJpgFile(const String& base64Str, const String& gestureName) {
//     // Check local Wi-Fi state before initiating a network call
//     if (WiFi.status() != WL_CONNECTED) {
//         if (Serial) Serial.println("[❌ OFFLINE] Wi-Fi link dropped. Aborting upload.");
//         isWaitingForClear = false;
//         return;
//     }

//     WiFiClientSecure client;
//     client.setInsecure(); 
//     client.setTimeout(8); // Increased slightly to accommodate slower garden transmission rails

//     if (!client.connect("api.telegram.org", 443)) {
//         if (Serial) Serial.println("[❌ NETWORK ERROR] Connection to Telegram endpoint failed.");
//         isWaitingForClear = false; 
//         return;
//     }

//     size_t actualBinaryLen = 0;
//     int decodeStatus = mbedtls_base64_decode(staticBinaryBuffer, STATIC_BUFFER_SIZE, &actualBinaryLen, (const unsigned char*)base64Str.c_str(), base64Str.length());

//     if (decodeStatus != 0) {
//         if (Serial) Serial.println("[❌ CODEC ERROR] Base64 image stream decode failed.");
//         isWaitingForClear = false;
//         return;
//     }

//     // TELEGRAM HTTP MULTIPART POST MANIFEST PIPELINE
//     String boundary = "----ESP32C3Boundary";
//     String head = "--" + boundary + "\r\n" +
//                   "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + chatID + "\r\n" +
//                   "--" + boundary + "\r\n" +
//                   "Content-Disposition: form-data; name=\"caption\"\r\n\r\nMatch: " + gestureName + " (RSSI: " + String(WiFi.RSSI()) + "dBm)\r\n" +
//                   "--" + boundary + "\r\n" +
//                   "Content-Disposition: form-data; name=\"photo\"; filename=\"capture.jpg\"\r\n" +
//                   "Content-Type: image/jpeg\r\n\r\n";
//     String tail = "\r\n--" + boundary + "--\r\n";

//     uint32_t totalLen = head.length() + actualBinaryLen + tail.length();

//     client.println("POST /bot" + botToken + "/sendPhoto HTTP/1.1");
//     client.println("Host: api.telegram.org");
//     client.println("Content-Length: " + String(totalLen));
//     client.println("Content-Type: multipart/form-data; boundary=" + boundary);
//     client.println("Connection: close");
//     client.println();

//     client.print(head);
//     client.write(staticBinaryBuffer, actualBinaryLen);
//     client.print(tail);

//     if (Serial) Serial.println("[✔ SUCCESS] Data payload pushed out via Wi-Fi stack.");
// }

void sendTelegramJpgFile(const String& base64Str, const String& gestureName) {
    // 🧪 LOCAL TEST DUMMY: No Wi-Fi or Telegram transmission occurs here
    if (Serial) {
        Serial.println("====================================================");
        Serial.print("🌐 [DUMMY NETWORK] Simulating Telegram Upload for: ");
        Serial.println(gestureName);
        Serial.print("📷 [DUMMY NETWORK] Base64 Image Size: ");
        Serial.print(base64Str.length());
        Serial.println(" bytes.");
        Serial.print("📡 [DUMMY NETWORK] Active Hotspot Connection Strength: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.println("====================================================");
    }
    
    // The state machine will safely think the transmission succeeded
}

