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

// ANTI-BOMBARDMENT & LOCKOUT VARIABLES
unsigned long lastTelegramUploadTime = 0;
const unsigned long telegramCooldown = 15000; // Minimum 15s between uploads
int lastDetectedID = -1;
bool isWaitingForClear = false;

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

    // OUTDOOR HEARTBEAT: Prints signal strength to track dropouts over distance
    if (currentMillis - lastHeartbeatTime >= 3000) {
        lastHeartbeatTime = currentMillis;
        if (Serial) {
            Serial.print("[RSSI: ");
            Serial.print(WiFi.RSSI());
            Serial.print("dBm] ");
        } else {
            Serial.print("."); 
        }
    }

    // WATCHDOG RESET: If the interlock flag stays stuck for > 15s, force-release it
    if (isWaitingForClear && (currentMillis - lastTelegramUploadTime >= telegramCooldown)) {
        if (Serial) Serial.println("\n[⚠️ WATCHDOG] Force-releasing lock state.");
        isWaitingForClear = false;
        lastDetectedID = -1;
    }

    if (currentMillis - lastCheckTime >= checkInterval) {
        lastCheckTime = currentMillis;

        // Run ultra-light fast gesture evaluation pass (show=false)
        int status = AI.invoke(1, false, false);

        if (status >= 0 && AI.boxes().size() > 0) {
            int currentID = AI.boxes()[0].target;
            int confidence = AI.boxes()[0].score;

            if (confidence > 65) {
                if (isWaitingForClear) {
                    return; 
                }

                if (currentID != lastDetectedID && (currentMillis - lastTelegramUploadTime >= telegramCooldown)) {
                    lastDetectedID = currentID;
                    lastTelegramUploadTime = currentMillis; 
                    isWaitingForClear = true; 

                    String gestureName = "";
                    if (currentID == 0) gestureName = "PAPER";
                    else if (currentID == 1) gestureName = "ROCK";
                    else if (currentID == 2) gestureName = "SCISSORS";

                    if (Serial) {
                        Serial.println("\n------------------------------------------------");
                        Serial.println("[EVENT] " + gestureName + " Detected! Capturing image...");
                    }

                    AI.invoke(1, false, true);
                    String rawBase64 = AI.last_image();

                    if (rawBase64.length() > 0) {
                        sendTelegramJpgFile(rawBase64, gestureName);
                    } else {
                        if (Serial) Serial.println(" ➔ [⚠️ WARNING] Image payload returned empty.");
                        isWaitingForClear = false; 
                    }
                }
            }
        } else {
            if (lastDetectedID != -1 || isWaitingForClear) {
                if (Serial) Serial.println("\n[-] Hand cleared. Resetting triggers.");
                lastDetectedID = -1;
                isWaitingForClear = false; 
            }
        }
    }
}

void sendTelegramJpgFile(const String& base64Str, const String& gestureName) {
    // Check local Wi-Fi state before initiating a network call
    if (WiFi.status() != WL_CONNECTED) {
        if (Serial) Serial.println("[❌ OFFLINE] Wi-Fi link dropped. Aborting upload.");
        isWaitingForClear = false;
        return;
    }

    WiFiClientSecure client;
    client.setInsecure(); 
    client.setTimeout(8); // Increased slightly to accommodate slower garden transmission rails

    if (!client.connect("api.telegram.org", 443)) {
        if (Serial) Serial.println("[❌ NETWORK ERROR] Connection to Telegram endpoint failed.");
        isWaitingForClear = false; 
        return;
    }

    size_t actualBinaryLen = 0;
    int decodeStatus = mbedtls_base64_decode(staticBinaryBuffer, STATIC_BUFFER_SIZE, &actualBinaryLen, (const unsigned char*)base64Str.c_str(), base64Str.length());

    if (decodeStatus != 0) {
        if (Serial) Serial.println("[❌ CODEC ERROR] Base64 image stream decode failed.");
        isWaitingForClear = false;
        return;
    }

    // TELEGRAM HTTP MULTIPART POST MANIFEST PIPELINE
    String boundary = "----ESP32C3Boundary";
    String head = "--" + boundary + "\r\n" +
                  "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + chatID + "\r\n" +
                  "--" + boundary + "\r\n" +
                  "Content-Disposition: form-data; name=\"caption\"\r\n\r\nMatch: " + gestureName + " (RSSI: " + String(WiFi.RSSI()) + "dBm)\r\n" +
                  "--" + boundary + "\r\n" +
                  "Content-Disposition: form-data; name=\"photo\"; filename=\"capture.jpg\"\r\n" +
                  "Content-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--" + boundary + "--\r\n";

    uint32_t totalLen = head.length() + actualBinaryLen + tail.length();

    client.println("POST /bot" + botToken + "/sendPhoto HTTP/1.1");
    client.println("Host: api.telegram.org");
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println("Connection: close");
    client.println();

    client.print(head);
    client.write(staticBinaryBuffer, actualBinaryLen);
    client.print(tail);

    if (Serial) Serial.println("[✔ SUCCESS] Data payload pushed out via Wi-Fi stack.");
}
