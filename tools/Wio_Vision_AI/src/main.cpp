#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Seeed_Arduino_SSCMA.h>
#include "main.h" // Holds your function prototype declarations

SSCMA AI;

// --- WI-FI & TELEGRAM CREDENTIALS FROM PLATFORMIO.INI ---
const char* ssid     = SECRET_SSID; 
const char* password = SECRET_PASS; 
const char* botToken = SECRET_TOKEN; 
const char* chatId   = SECRET_ID;

// --- Rate Limiting & System Variables ---
unsigned long lastAlertTime = 0; 
const unsigned long alertInterval = 60000; // 1-minute cool-down window
unsigned long lastHeartbeatTime = 0;
const unsigned long heartbeatInterval = 2000; // 2-second background heartbeat
bool firstAlertSent = false;

void setup() {
    Serial.begin(115200);
    delay(3000); // USB port synchronization window
    Serial.println("\n[SYSTEM] Mated XIAO ESP32-C3 Initializing...");

    // Initialize Native ESP32 Wi-Fi Radio
    WiFi.begin(ssid, password);
    Serial.print("[WIFI] Connecting to network");
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI] Connected Successfully!");
    } else {
        Serial.println("\n[WIFI ERROR] Connection Failed. Operating locally only.");
    }

    // Start I2C Communication using hardware trace paths
    Wire.begin(); 
    Wire.setClock(400000); 
    
    if (!AI.begin(&Wire)) {
        Serial.println("[CAMERA ERROR] Grove Vision AI V2 not found on I2C bus!");
        while (1) { delay(1000); }
    }

    Serial.println("[SYSTEM] Setup Ready. Awaiting filtered camera streams...");
}

void loop() {
    // Fast Filter Stream Mode (No image requested yet)
    int status = AI.invoke(1, true, false);
    
    if (status == 0) { 
        int targetCount = AI.boxes().size();
        
        if (targetCount > 0) {
            int score = AI.boxes()[0].score; // Grabs confidence score of the person
            
            Serial.print("\n=== [MATCH DETECTED] Person tracked at score: ");
            Serial.print(score);
            Serial.println("% ===");

            // --- WI-FI DIRECT TELEGRAM EXECUTION ---
            unsigned long currentTime = millis();
            String alertMsg = "Security Alert: A person was detected with " + String(score) + "% confidence!";

            if (!firstAlertSent) {
                Serial.println("[WIFI] First match detected! Sending immediate secure payload...");
                firstAlertSent = true;
                lastAlertTime = currentTime; 
                sendDirectTelegramAlert(botToken, chatId, alertMsg);
            } 
            else if (currentTime - lastAlertTime >= alertInterval) {
                Serial.println("[WIFI] Interval elapsed. Sending secure network payload...");
                lastAlertTime = currentTime;
                sendDirectTelegramAlert(botToken, chatId, alertMsg);
            } 
            else {
                Serial.print("[WIFI SKIPPED] Cooldown remaining: ");
                Serial.print((alertInterval - (currentTime - lastAlertTime)) / 1000);
                Serial.println("s");
            }
        }
    }
    
    // --- SYSTEM SERIAL HEARTBEAT ---
    unsigned long currentMillis = millis();
    if (currentMillis - lastHeartbeatTime >= heartbeatInterval) {
        Serial.print("[HEARTBEAT] XIAO Active. Uptime: ");
        Serial.print(currentMillis / 1000);
        Serial.println("s");
        lastHeartbeatTime = currentMillis;
    }
    
    delay(60); 
}

// Sends a direct raw HTTPS request natively using ESP32 secure libraries
void sendDirectTelegramAlert(const char* token, const char* chat, String message) {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;
        
        // Supported natively by ESP32. Bypasses SSL cert verification without blocking or freezing
        client.setInsecure(); 

        HTTPClient http;
        http.setTimeout(5000); // Network drop protection timeout


        // 1. Convert any sentence whitespaces to safe web syntax formatting (%20)
        message.replace(" ", "%20");

        // 2. Build the string block using clear parameters so the compiler cannot drop text
        String url = "https://api.telegram.org/bot";
        url += String(token);
        url += "/sendMessage?chat_id=";
        url += String(chat);
        url += "&text=";
        url += message;

        Serial.println("[WIFI] Initializing secure direct handshake with Telegram...");
        Serial.print("[DEBUG WIFI] Target URL: ");
        Serial.println(url);
        
        if (http.begin(client, url)) {
            int httpResponseCode = http.GET(); // Execute request securely over HTTPS
            
            if (httpResponseCode > 0) {
                Serial.print("[WIFI] Handshake Success! Telegram response code: ");
                Serial.println(httpResponseCode); // 200 = Success!
            } else {
                Serial.print("[WIFI ERROR] Secure transmission dropped. Code: ");
                Serial.println(httpResponseCode);
            }
            http.end(); 
        } else {
            Serial.println("[WIFI ERROR] Unable to construct HTTP structure.");
        }
    } else {
        Serial.println("[WIFI ERROR] Network offline. Abandoning request.");
    }
}
