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
    // 1. FAST SCANNER MODE
    int status = AI.invoke(1, true, false);
    
    if (status == 0) { 
        int targetCount = AI.boxes().size();
        
        if (targetCount > 0) {
            int score = AI.boxes()[0].score; // Explicit index 0 points to the primary tracked box
            
            Serial.print("\n=== [MATCH DETECTED] Person tracked at score: ");
            Serial.print(score);
            Serial.println("% ===");

            // --- WI-FI DIRECT SECURE TELEGRAM PHOTO EXECUTION ---
            unsigned long currentTime = millis();
            String alertMsg = "Security Alert: A person was detected with " + String(score) + "% confidence!";

            bool sendNow = false;
            if (!firstAlertSent) {
                Serial.println("[WIFI] First match detected! Preparing instant photo alert...");
                firstAlertSent = true;
                lastAlertTime = currentTime; 
                sendNow = true;
            } 
            else if (currentTime - lastAlertTime >= alertInterval) {
                Serial.println("[WIFI] Interval elapsed. Preparing secure photo alert...");
                lastAlertTime = currentTime;
                sendNow = true;
            } 
            else {
                Serial.print("[WIFI SKIPPED] Cooldown remaining: ");
                Serial.print((alertInterval - (currentTime - lastAlertTime)) / 1000);
                Serial.println("s");
            }

            if (sendNow) {
                // 3. THE SWITCH: Request raw base64 image data string
                if (AI.invoke(1, false, true) == CMD_OK) {
                    String livePhoto = AI.last_image();
                    
                    if (livePhoto.length() > 0) {
                        sendTelegramPhotoAlert(botToken, chatId, livePhoto, alertMsg);
                    } else {
                        Serial.println("[CAMERA ERROR] Image extraction buffer returned empty.");
                    }
                } else {
                    Serial.println("[CAMERA ERROR] Frame invocation failed during photo request.");
                }
            }
        }
    } else {
        // 🌟 THE EXPLICIT TRACKER: Prints whenever the I2C camera bus returns something other than 0
        Serial.print("[CAMERA STATUS] Invoke returned error code: ");
        Serial.println(status);
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



void sendTelegramPhotoAlert(const char* token, const char* chat, String base64ImageStr, String captionText) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI ERROR] Network offline. Abandoning request.");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure(); // Native ESP32 SSL bypass keeps processing fast

    const char* server = "api.telegram.org";
    if (!client.connect(server, 443)) {
        Serial.println("[WIFI ERROR] Secure connection to Telegram failed!");
        return;
    }

    Serial.println("[WIFI] Secure socket open. Preparing chunked photo transmission...");

    // Create a strict multipart boundary marker to separate our form elements
    String boundary = "----XIAOESP32C3MultipartBoundary";
    
    // Assemble the HTTP form headers
    String head = "--" + boundary + "\r\n" +
                  "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + String(chat) + "\r\n" +
                  "--" + boundary + "\r\n" +
                  "Content-Disposition: form-data; name=\"caption\"\r\n\r\n" + captionText + "\r\n" +
                  "--" + boundary + "\r\n" +
                  "Content-Disposition: form-data; name=\"photo\"; filename=\"alert.jpg\"\r\n" +
                  "Content-Type: image/jpeg\r\n\r\n";
                  
    String tail = "\r\n--" + boundary + "--\r\n";

    // Calculate total layout length so Telegram's server knows exactly when the payload finishes
    uint32_t totalLength = head.length() + base64ImageStr.length() + tail.length();

    // Stream out standard HTTP POST header instructions to the Telegram api
    client.println("POST /bot" + String(token) + "/sendPhoto HTTP/1.1");
    client.println("Host: api.telegram.org");
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.print("Content-Length: ");
    client.println(totalLength);
    client.println("Connection: close");
    client.println(); // Terminal empty header line

    // Step A: Transmit the Form data variables
    client.print(head);

    // Step B: Stream the Base64 payload out in small 512-byte slices to protect internal RAM bounds
    int totalBytes = base64ImageStr.length();
    int chunkSize = 512;
    Serial.print("[WIFI] Streaming photo bytes: ");
    
    for (int i = 0; i < totalBytes; i += chunkSize) {
        int currentChunk = min(chunkSize, totalBytes - i);
        client.print(base64ImageStr.substring(i, i + currentChunk));
        Serial.print(".");
        delay(5); // Soft millisecond pause ensures the Wi-Fi hardware buffers do not overflow
    }
    Serial.println(" Done!");

    // Step C: Transmit the closing form tail wrap
    client.print(tail);

    // Monitor for a brief server acknowledgment confirmation response line
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println("[WIFI ERROR] Telegram server acknowledgment timeout.");
            client.stop();
            return;
        }
    }

    String responseLine = client.readStringUntil('\r');
    Serial.print("[WIFI] Server Response: ");
    Serial.println(responseLine); // Looking for HTTP/1.1 200 OK

    client.stop();
}

