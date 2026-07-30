#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Seeed_Arduino_SSCMA.h>
#include "mbedtls/base64.h"
#include "secrets.h"

//🌟 STEP 1: CONFIGURE NETWORK CREDENTIALS
const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;

// 🌟 STEP 2: CONFIGURE TELEGRAM BOT ACCESS METRICS
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
const unsigned long telegramCooldown = 15000; // 🛑 Minimum 15s between uploads
int lastDetectedID = -1;
bool isWaitingForClear = false;

// SERIAL HEARTBEAT VARIABLE
unsigned long lastHeartbeatTime = 0;

void sendTelegramJpgFile(const String& base64Str, const String& gestureName);

void setup() {
    Serial.begin(115200);
    while(!Serial);
    
    Serial.println("\n================================================");
    Serial.println("[🤖 SERIAL HEARTBEAT BOT] Initializing Core...");
    Serial.println("================================================");

    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[CONNECTED] IP Address: " + WiFi.localIP().toString());

    Wire.begin(6, 7); 
    Wire.setClock(400000); 
    
    if (!AI.begin(&Wire)) {
        Serial.println("[❌ ERROR] Camera board not found over I2C.");
        while (1) { delay(1000); }
    }
    Serial.println("[SUCCESS] Heartbeat system armed. Ready for gestures.");
}

void loop() {
    unsigned long currentMillis = millis();

    // THE SERIAL HEARTBEAT: Prints a dot every 3 seconds when idle
    if (currentMillis - lastHeartbeatTime >= 3000) {
        lastHeartbeatTime = currentMillis;
        Serial.print("."); 
    }

    // WATCHDOG RESET: If the interlock flag stays stuck for > 15s, force-release it
    if (isWaitingForClear && (currentMillis - lastTelegramUploadTime >= telegramCooldown)) {
        Serial.println("\n[⚠️ WATCHDOG] Lock state exceeded cooldown. Force-releasing interlock.");
        isWaitingForClear = false;
        lastDetectedID = -1;
    }

    if (currentMillis - lastCheckTime >= checkInterval) {
        lastCheckTime = currentMillis;

        // Run ultra-light fast gesture evaluation pass (show=false)
        int status = AI.invoke(1, false, false);

        if (status >= 0 && AI.boxes().size() > 0) {
            // 🌟 THE ARRAYS FIX: Access vector index position 0 elements explicitly
            int currentID = AI.boxes()[0].target;
            int confidence = AI.boxes()[0].score;

            if (confidence > 65) {
                // If locked waiting for a hand clear, bypass data capturing logic
                if (isWaitingForClear) {
                    return; 
                }

                // Check both state changes and the mandatory time window layout
                if (currentID != lastDetectedID && (currentMillis - lastTelegramUploadTime >= telegramCooldown)) {
                    lastDetectedID = currentID;
                    lastTelegramUploadTime = currentMillis; 
                    isWaitingForClear = true; 

                    String gestureName = "";
                    if (currentID == 0) gestureName = "PAPER";
                    else if (currentID == 1) gestureName = "ROCK";
                    else if (currentID == 2) gestureName = "SCISSORS";

                    Serial.println("\n------------------------------------------------");
                    Serial.println("[EVENT] " + gestureName + " Detected! Capturing image...");

                    AI.invoke(1, false, true);
                    String rawBase64 = AI.last_image();

                    if (rawBase64.length() > 0) {
                        sendTelegramJpgFile(rawBase64, gestureName);
                    } else {
                        Serial.println(" ➔ [⚠️ WARNING] Image payload returned empty.");
                        isWaitingForClear = false; 
                    }
                }
            }
        } else {
            // Hand was removed completely from the frame, reset our tracker states safely
            if (lastDetectedID != -1 || isWaitingForClear) {
                Serial.println("\n[-] Hand cleared. Resetting triggers.");
                lastDetectedID = -1;
                isWaitingForClear = false; 
            }
        }
    }
}

void sendTelegramJpgFile(const String& base64Str, const String& gestureName) {
    WiFiClientSecure client;
    client.setInsecure(); 
    
    // THE NETWORK FIXED GATEWAY: Drop connections that stall for more than 5s
    client.setTimeout(5); 

    if (!client.connect("api.telegram.org", 443)) {
        Serial.println("[❌ NETWORK ERROR] Connection to Telegram failed.");
        isWaitingForClear = false; 
        return;
    }

    size_t actualBinaryLen = 0;
    int decodeStatus = mbedtls_base64_decode(staticBinaryBuffer, STATIC_BUFFER_SIZE, &actualBinaryLen, (const unsigned char*)base64Str.c_str(), base64Str.length());

    if (decodeStatus != 0) {
        Serial.println("[❌ CODEC ERROR] Decoding failed.");
        isWaitingForClear = false;
        client.stop();
        return;
    }

    String boundary = "----ESP32C3BoundaryLineFires----";
    String headerText = "--" + boundary + "\r\n";
    headerText += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + chatID + "\r\n";
    headerText += "--" + boundary + "\r\n";
    headerText += "Content-Disposition: form-data; name=\"caption\"\r\n\r\n🎯 Gesture Event: " + gestureName + "\r\n";
    headerText += "--" + boundary + "\r\n";
    headerText += "Content-Disposition: form-data; name=\"document\"; filename=\"motion_snapshot.jpg\"\r\n";
    headerText += "Content-Type: image/jpeg\r\n\r\n";

    String footerText = "\r\n--" + boundary + "--\r\n";
    long totalContentLength = headerText.length() + actualBinaryLen + footerText.length();

    client.print("POST /bot" + botToken + "/sendDocument HTTP/1.1\r\n");
    client.print("Host: api.telegram.org\r\n");
    client.print("Content-Length: " + String(totalContentLength) + "\r\n");
    client.print("Content-Type: multipart/form-data; boundary=" + boundary + "\r\n");
    client.print("Connection: close\r\n\r\n");

    client.print(headerText);
    client.write(staticBinaryBuffer, actualBinaryLen);
    client.print(footerText);

    Serial.println("[🚀 TELEGRAM] Document uploaded successfully.");
    client.stop();
}
