#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Seeed_Arduino_SSCMA.h>
#include "main.h" // Holds your function prototype declarations
#include "secrets.h" // Contains your Wi-Fi and Telegram credentials

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

// Create a state flag tracker at the top of your loop (or add to your global variables)
bool isAwaitingPhotoFetch = false;

void loop() {
    
    // --- STATE PATH A: HIGH-OVERHEAD IMAGE FETCH ---
    if (isAwaitingPhotoFetch) {
        Serial.println("\n[CAMERA] State Active: Executing isolated Step 2 photo fetch...");
        delay(200); // Allow the camera to stabilize before invoking the fetch
        int statusStep2 = AI.invoke(1, false, true);
        
        if (statusStep2 == 0) {
            String livePhoto = AI.last_image();
            
            if (livePhoto.length() > 0) {
                // Re-extract the score from the existing box buffer to include in our caption
                int score = (AI.boxes().size() > 0) ? AI.boxes()[0].score : 50;
                String alertMsg = "Security Alert: A person was detected with " + String(score) + "% confidence!";
                
                // Dispatch your piece-by-piece text file document utility
                sendTelegramTextFileAlert(botToken, chatId, livePhoto, alertMsg);
            } else {
                Serial.println("[CAMERA ERROR] Step 2 image extraction buffer returned empty.");
            }
        } else {
            Serial.print("[CAMERA ERROR] Step 2 Image Fetch failed with code: ");
            Serial.println(statusStep2);
        }
        
        // 🌟 CRITICAL: Immediately flip back to scanner mode for the next loop iteration
        isAwaitingPhotoFetch = false;
    }
    
    // --- STATE PATH B: LIGHTWEIGHT MONITOR SCAN ---
    else {
        int statusStep1 = AI.invoke(1, true, false);
        
        if (statusStep1 == 0) { 
            int targetCount = AI.boxes().size();
            
            if (targetCount > 0) {
                int score = AI.boxes()[0].score;
                
                Serial.print("\n=== [MATCH DETECTED] Person tracked at score: ");
                Serial.print(score);
                Serial.println("% ===");

                // Check our Wi-Fi rate-limiting cooldown timers
                unsigned long currentTime = millis();
                bool sendNow = false;
                
                if (!firstAlertSent) {
                    Serial.println("[WIFI] First match detected! Queueing Step 2 photo state...");
                    firstAlertSent = true;
                    lastAlertTime = currentTime; 
                    sendNow = true;
                } 
                else if (currentTime - lastAlertTime >= alertInterval) {
                    Serial.println("[WIFI] Interval elapsed. Queueing Step 2 photo state...");
                    lastAlertTime = currentTime;
                    sendNow = true;
                } 
                else {
                    Serial.print("[WIFI SKIPPED] Cooldown remaining: ");
                    Serial.print((alertInterval - (currentTime - lastAlertTime)) / 1000);
                    Serial.println("s");
                }

                if (sendNow) {
                    // 🌟 CRITICAL: Instead of triggering Step 2 here, we just flip the flag.
                    // The microcontroller will finish this loop and execute Step 2 freshly on the next turn.
                    isAwaitingPhotoFetch = true;
                }
            }
        } else {
            Serial.print("[STEP 1 SCAN] Result code: ");
            Serial.println(statusStep1);
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

void sendTelegramTextFileAlert(const char* token, const char* chat, String base64ImageStr, String captionText) {
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

    Serial.println("[WIFI] Secure socket open. Preparing chunked Base64 file transmission...");

    // Create a strict boundary marker for the multipart layout headers
    String boundary = "----XIAOESP32C3MultipartBoundary";
    
    // Assemble the HTTP form body headers piece-by-piece
    String head = "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
    head += String(chat) + "\r\n";
    head += "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"caption\"\r\n\r\n";
    head += captionText + "\r\n";
    head += "--" + boundary + "\r\n";
    head += "Content-Disposition: form-data; name=\"document\"; filename=\"alert.txt\"\r\n";
    head += "Content-Type: text/plain\r\n\r\n";
                  
    String tail = "\r\n--" + boundary + "--\r\n";

    // Calculate the total exact size of our payload parts combined
    uint32_t totalLength = head.length() + base64ImageStr.length() + tail.length();

    // Send the raw HTTP/1.1 POST instructions out to the socket channel
    client.println("POST /bot" + String(token) + "/sendDocument HTTP/1.1");
    client.println("Host: api.telegram.org");
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.print("Content-Length: ");
    client.println(totalLength);
    client.println("Connection: close");
    client.println(); // Terminal empty header line

    // Step A: Transmit the starting Form variables text
    client.print(head);

    // Step B: Stream the long Base64 payload out in small 512-byte slices to protect RAM bounds
    int totalBytes = base64ImageStr.length();
    int chunkSize = 512;
    Serial.print("[WIFI] Streaming raw text payload: ");
    
    for (int i = 0; i < totalBytes; i += chunkSize) {
        int currentChunk = min(chunkSize, totalBytes - i);
        client.print(base64ImageStr.substring(i, i + currentChunk));
        Serial.print(".");
        delay(5); // Soft pause ensures internal Wi-Fi hardware arrays don't drop packets
    }
    Serial.println(" Done!");

    // Step C: Transmit the final multipart closing boundary tail
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
