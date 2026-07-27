#include <Seeed_Arduino_SSCMA.h>
#include <Wire.h> 
#include <TFT_eSPI.h> 
#include <rpcWiFi.h>      // Required for Wio Terminal Wi-Fi
#include <WiFiClientSecure.h> // Secure connection layer for official APIs
#include "main.h" // Custom header for function prototypes

SSCMA AI;
TFT_eSPI tft = TFT_eSPI();

const int BUZZER_PIN = WIO_BUZZER; 

// --- 1. FILL IN YOUR WI-FI DETAILS ---
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// --- 2. FILL IN YOUR DIRECT TELEGRAM BOT DETAILS ---
const char* botToken = SECRET_TOKEN;
const char* chatId = SECRET_ID;

// --- Rate Limiting (Cool-down) ---
unsigned long lastAlertTime = 0; 
const unsigned long alertInterval = 60000; // 1 minute cool-down

unsigned long lastHeartbeatTime = 0;
const unsigned long heartbeatInterval = 2000; // Print a heartbeat every 2000ms (2 seconds)
bool firstAlertSent = false;


void setup() {
    Serial.begin(115200);
    delay(2000); 
    Serial.println("\n[SYSTEM] Serial line initialized. Starting setup...");

    tft.init();
    tft.setRotation(3); 
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.drawString("Connecting Wi-Fi...", 10, 10);
    tft.drawLine(0, 35, 320, 35, TFT_BLUE);

    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        tft.setTextColor(TFT_GREEN);
        tft.drawString("Wi-Fi Connected!", 10, 40);
        Serial.println("\nWi-Fi Connected!");
    } else {
        tft.setTextColor(TFT_RED);
        tft.drawString("Wi-Fi Failed!", 10, 40);
    }
    delay(1000);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    Wire.begin(); 
    Wire.setClock(400000); 
    
    if (!AI.begin(&Wire)) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        tft.drawString("Camera Wire Dead!", 10, 40);
        while (1); 
    }

    tft.fillRect(0, 40, 320, 200, TFT_BLACK);
    tft.setTextColor(TFT_DARKGREEN);
    tft.drawString("[SCANNING] Awaiting stream...", 10, 110);
}

void loop() {
    // Fast Filter Stream Mode (No image requested)
    int status = AI.invoke(1, true, false);
    
    if (status == 0) { 
        int targetCount = AI.boxes().size();
        
        if (targetCount > 0) {
            // 1. UPDATE THE HARDWARE SCREEN LAYER IMMEDIATELY
            tft.fillRect(0, 80, 320, 160, TFT_BLACK); 
            tft.setTextColor(TFT_RED);
            tft.drawString("ALERT: Person Spotted!", 10, 100);
            
            int score = AI.boxes()[0].score; // Grabs confidence of first target
            tft.setTextColor(TFT_WHITE);
            tft.drawString("Confidence: " + String(score) + "%", 10, 130);

            // 2. HARDWARE AUDIO BUZZ
            analogWrite(BUZZER_PIN, 128); 
            delay(100);
            analogWrite(BUZZER_PIN, 0); 

            // 3. FORCE SERIAL PRINT (This proves the match logic is executed by the compiler)
            Serial.print("\n=== [MATCH DETECTED] Screen score matches serial: ");
            Serial.print(score);
            Serial.println("% ===");

            // 4. WI-FI NETWORK EXECUTION
            unsigned long currentTime = millis();
            String alertMsg = "Security Alert: A person was detected with " + String(score) + "% confidence!";

            if (!firstAlertSent) {
                // First match ever: bypass countdown and fire instantly
                Serial.println("[WIFI] First match detected! Sending immediate payload...");
                
                firstAlertSent = true;
                lastAlertTime = currentTime; 
                
                sendDirectTelegramAlert(botToken, chatId, alertMsg);
            } 
            else if (currentTime - lastAlertTime >= alertInterval) {
                // Future matches: fire only if the 1-minute window has passed
                Serial.println("[WIFI] Interval elapsed. Sending network payload...");
                
                lastAlertTime = currentTime;
                
                sendDirectTelegramAlert(botToken, chatId, alertMsg);
            } 
            else {
                // Still inside the quiet window
                Serial.print("[WIFI SKIPPED] Cooldown remaining: ");
                Serial.print((alertInterval - (currentTime - lastAlertTime)) / 1000);
                Serial.println("s");
            }
        } 
        else {
            // No target found in this frame execution loop
            tft.fillRect(0, 80, 320, 160, TFT_BLACK); 
            tft.setTextColor(TFT_DARKGREEN);
            tft.drawString("[SCANNING] Area Clear.", 10, 110);
        }
    }
    
    // --- SYSTEM SERIAL HEARTBEAT ---
    unsigned long currentMillis = millis();
    if (currentMillis - lastHeartbeatTime >= heartbeatInterval) {
        Serial.print("[HEARTBEAT] System is alive. Uptime: ");
        Serial.print(currentMillis / 1000);
        Serial.println("s");
        lastHeartbeatTime = currentMillis;
    }
    
    delay(60); 
}


// Sends a direct raw HTTPS request to official Telegram servers
void sendDirectTelegramAlert(const char* token, const char* chat, String message) {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;
        
        client.setCACert(nullptr); // Wio Terminal SSL bypass

        const char* server = "api.telegram.org";
        if (client.connect(server, 443)) {
            Serial.println("[WIFI] Handshaking with Telegram...");
            
            // Note: Changed hardcoded variables to use the incoming 'chat' and 'token' parameters
            String payload = "{\"chat_id\":\"" + String(chat) + "\",\"text\":\"" + message + "\"}";
            
            client.println("POST /bot" + String(token) + "/sendMessage HTTP/1.1");
            client.println("Host: api.telegram.org");
            client.println("Content-Type: application/json");
            client.print("Content-Length: ");
            client.println(payload.length());
            client.println("Connection: close");
            client.println();
            client.print(payload);
            
            Serial.println("[WIFI] Text alert successfully sent directly!");
        } else {
            Serial.println("[WIFI ERROR] Failed to connect to api.telegram.org");
        }
        client.stop();
    } else {
        Serial.println("[WIFI ERROR] Network disconnected.");
    }
}
