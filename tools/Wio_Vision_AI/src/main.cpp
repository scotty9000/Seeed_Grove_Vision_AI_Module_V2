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
    
<<<<<<< HEAD
    // 🌟 THE HEADLESS FIX: Replaced 'while(!Serial);' with a non-blocking timeout window.
    // Gives a computer 3 seconds to link up. If no PC is found, it continues booting automatically!
    unsigned long startWindow = millis();
    while (!Serial && (millis() - startWindow < 3000)) {
        delay(10);
    }
    
    if (Serial) {
        Serial.println("\n================================================");
        Serial.println("[🔋 STANDALONE PRODUCTION] Booting Staggered Rails...");
        Serial.println("================================================");
=======
    Serial.println("\n================================================");
    Serial.println("[🤖 SERIAL HEARTBEAT BOT] Initializing Core...");
    Serial.println("================================================");

    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
>>>>>>> 2db8c68c4b1620ea8f9a5a7ba76086bbbe17af64
    }

    // Initialize I2C layers first
    Wire.begin(6, 7); 
    Wire.setClock(400000); 
    
    if (!AI.begin(&Wire)) {
        if (Serial) Serial.println("[❌ ERROR] Camera board not found over I2C.");
        while (1) { delay(1000); }
    }
<<<<<<< HEAD
    if (Serial) Serial.println("[SUCCESS] Camera board online over I2C.");
    delay(2000); // Allow electrical rails to settle before radio power-up

    // Turn on Wi-Fi cleanly 
    WiFi.disconnect(true); 
    delay(500);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int connectionTimeoutCounter = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (Serial) Serial.print(".");
        connectionTimeoutCounter++;
        
        if (connectionTimeoutCounter > 30) { 
            if (Serial) Serial.println("\n[⚠️ TIMEOUT] Resetting Wi-Fi adapter...");
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
    }
=======
    Serial.println("[SUCCESS] Heartbeat system armed. Ready for gestures.");
>>>>>>> 2db8c68c4b1620ea8f9a5a7ba76086bbbe17af64
}

void loop() {
    unsigned long currentMillis = millis();

<<<<<<< HEAD
    // Heartbeat indicator over Serial (only outputs if a terminal is listening)
    if (Serial && (currentMillis - lastHeartbeatTime >= 3000)) {
=======
    // THE SERIAL HEARTBEAT: Prints a dot every 3 seconds when idle
    if (currentMillis - lastHeartbeatTime >= 3000) {
>>>>>>> 2db8c68c4b1620ea8f9a5a7ba76086bbbe17af64
        lastHeartbeatTime = currentMillis;
        Serial.print("."); 
    }

    // WATCHDOG RESET: If the interlock flag stays stuck for > 15s, force-release it
    if (isWaitingForClear && (currentMillis - lastTelegramUploadTime >= telegramCooldown)) {
<<<<<<< HEAD
        if (Serial) Serial.println("\n[⚠️ WATCHDOG] Force-releasing lock state.");
=======
        Serial.println("\n[⚠️ WATCHDOG] Lock state exceeded cooldown. Force-releasing interlock.");
>>>>>>> 2db8c68c4b1620ea8f9a5a7ba76086bbbe17af64
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

<<<<<<< HEAD
                    if (Serial) {
                        Serial.println("\n------------------------------------------------");
                        Serial.println("[EVENT] " + gestureName + " Detected! Capturing image...");
                    }
=======
                    Serial.println("\n------------------------------------------------");
                    Serial.println("[EVENT] " + gestureName + " Detected! Capturing image...");
>>>>>>> 2db8c68c4b1620ea8f9a5a7ba76086bbbe17af64

                    AI.invoke(1, false, true);
                    String rawBase64 = AI.last_image();

                    if (rawBase64.length() > 0) {
                        sendTelegramJpgFile(rawBase64, gestureName);
                    } else {
<<<<<<< HEAD
                        if (Serial) Serial.println(" ➔ [⚠️ WARNING] Image payload returned empty.");
=======
                        Serial.println(" ➔ [⚠️ WARNING] Image payload returned empty.");
>>>>>>> 2db8c68c4b1620ea8f9a5a7ba76086bbbe17af64
                        isWaitingForClear = false; 
                    }
                }
            }
        } else {
            // Hand was removed completely from the frame, reset our tracker states safely
            if (lastDetectedID != -1 || isWaitingForClear) {
<<<<<<< HEAD
                if (Serial) Serial.println("\n[-] Hand cleared. Resetting triggers.");
=======
                Serial.println("\n[-] Hand cleared. Resetting triggers.");
>>>>>>> 2db8c68c4b1620ea8f9a5a7ba76086bbbe17af64
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
<<<<<<< HEAD
        if (Serial) Serial.println("[❌ NETWORK ERROR] Connection to Telegram failed.");
=======
        Serial.println("[❌ NETWORK ERROR] Connection to Telegram failed.");
>>>>>>> 2db8c68c4b1620ea8f9a5a7ba76086bbbe17af64
        isWaitingForClear = false; 
        return;
    }

    size_t actualBinaryLen = 0;
    int decodeStatus = mbedtls_base64_decode(staticBinaryBuffer, STATIC_BUFFER_SIZE, &actualBinaryLen, (const unsigned char*)base64Str.c_str(), base64Str.length());

    if (decodeStatus != 0) {
<<<<<<< HEAD
        if (Serial) Serial.println("[❌ CODEC ERROR] Decoding failed.");
=======
        Serial.println("[❌ CODEC ERROR] Decoding failed.");
>>>>>>> 2db8c68c4b1620ea8f9a5a7ba76086bbbe17af64
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

<<<<<<< HEAD
    if (Serial) Serial.println("[🚀 TELEGRAM] Document uploaded successfully.");
=======
    Serial.println("[🚀 TELEGRAM] Document uploaded successfully.");
>>>>>>> 2db8c68c4b1620ea8f9a5a7ba76086bbbe17af64
    client.stop();

    // 🌟 STATE RESET EMBEDDED: Fixes the 15-second watchdog lag bug instantly
    isWaitingForClear = false; 
}
