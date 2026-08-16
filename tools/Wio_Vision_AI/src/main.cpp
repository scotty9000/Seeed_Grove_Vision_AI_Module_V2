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

#define TELEGRAM_IP 149.154.166.110

// Static Buffer for Fragmentation Prevention
#define STATIC_BUFFER_SIZE 8192
unsigned char staticBinaryBuffer[STATIC_BUFFER_SIZE];

SSCMA AI;
unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 200; // Snappy 200ms camera scan rate

// ⏱️ ANTI-FLOOD LOCKOUT SYSTEM
const unsigned long ANTI_FLOOD_INTERVAL = 15000; 
unsigned long lockoutTimerStart = 0;             
bool isLockoutActive = false; 

// FIXED TIME-LAPSE TELEMETRY INTERVAL
unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 60000; // Sent exactly every 60 seconds (1 minute)


// TIMEOUTS AND MONITORING
unsigned long lastHeartbeatTime = 0;

void sendTelegramJpgFile(const String& base64Str, const String& gestureName);
void sendTelegramTextMessage(const String& labelText);
uint32_t assembleMultipartBuffer(const String& base64Str, const String& gestureName, const String& boundary);


void setup() {
    Serial.begin(115200);
    
    // HEADLESS OPERATION WINDOW: Avoid freezes when running on garden battery
    unsigned long startWindow = millis();
        // Initialize the telemetry clock right at startup completion
    lastTelemetryTime = millis();

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
    
    //  OPTIMIZATION: Throttling TX Power prevents internal C3 silicon reflections out in the garden
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

    // 1. FIXED TIME-LAPSE TELEMETRY TRIGGER
    // This loop path operates independently from any AI match variables or lockouts
    if (currentMillis - lastTelemetryTime >= telemetryInterval) {
        lastTelemetryTime = currentMillis;
        
        if (Serial) {
            Serial.println("\n================================================");
            Serial.println("[TIMELAPSE] 60s Interval Reached. Send RSSI Message...");
            Serial.println("================================================");
        }
        
        sendTelegramTextMessage("RSSI Message");      
    }


    // ⏱️ AUTOMATIC ANTI-FLOOD LOCKOUT RELEASE CHECKER
    if (isLockoutActive && (currentMillis - lockoutTimerStart >= ANTI_FLOOD_INTERVAL)) {
        Serial.println("\n[⏱️ LOCKOUT SYSTEM] Cooldown window expired. Alerts unlocked and ready.");
        isLockoutActive = false;
    }

    // ⏱️ PRE-FLIGHT LOCKOUT GATE: Skip camera scanning if the system is cooling down
    if (isLockoutActive) {
        // Optional: print down-counter metrics cleanly here if desired
        return; 
    }

    // IDLE HEARTBEAT: Only prints asterisks if the system is unlocked and waiting
    if (currentMillis - lastHeartbeatTime >= 3000) {
        lastHeartbeatTime = currentMillis;
        Serial.print("*"); 
    }

    if (currentMillis - lastCheckTime >= checkInterval) {
        lastCheckTime = currentMillis;

        // Run AI evaluation pass
        int status = AI.invoke(1, false, true);

        if (status < 0) {
            Serial.print("\n[❌ I2C ERROR] Camera bus read failed. Status: ");
            Serial.println(status);
            return; 
        }

        // Check if the AI model found any matching target objects
        if (AI.boxes().size() > 0) {
            int currentID = AI.boxes()[0].target;
            int confidence = AI.boxes()[0].score;

            if (confidence > 75) {
                
                // 🚀 ENGAGE ANTI-FLOOD TIMER IMMEDIATELY
                lockoutTimerStart = currentMillis; 
                isLockoutActive = true; 

                String gestureName = "";
                if (currentID == 0) gestureName = "PERSON";
                else  gestureName = "HUH?";

                Serial.println("\n================================================");
                Serial.print("[🚀 ALERT TRIGGERED] Valid Match: "); Serial.println(gestureName);
                Serial.print("[🚀 ALERT TRIGGERED] Confidence: "); Serial.print(confidence); Serial.println("%");
                Serial.print("[📷 IMAGE CAPTURE] Fetching JPEG buffer frame...");
                Serial.println("\n================================================");

                // Pull the actual image payload now that the trigger is validated
                String rawBase64 = AI.last_image();

                // Forward to your network function
                sendTelegramJpgFile(rawBase64, gestureName); 
                
                // Clear hardware registers to completely resolve retrigger loops
                AI.invoke(1, true, false); 
            }
        }
        
    } 
}


void sendDummyTelegramJpgFile(const String& base64Str, const String& gestureName) {
    String boundary = "----ESP32C3Boundary";

    // 🌟 EXECUTE ABSTRACTED BUFFER ASSEMBLY
    uint32_t totalPayloadLen = assembleMultipartBuffer(base64Str, gestureName, boundary);

    // If the helper function encountered an error or memory exception, it returns 0
    if (totalPayloadLen == 0) {
        if (Serial) Serial.println("[⚠️ SIMULATION] Aborting due to buffer assembly failure.");
        return;
    }

    // 6. SIMULATE LOCAL NETWORK TRANSMISSION
    if (Serial) {
        Serial.println("====================================================");
        Serial.print("[DUMMY NETWORK] Simulating Telegram Upload for: ");
        Serial.println(gestureName);
        Serial.print("[DUMMY NETWORK] Total Consolidated Payload Size: ");
        Serial.print(totalPayloadLen);
        Serial.println(" bytes.");
        Serial.print("[DUMMY NETWORK] Active Hotspot Connection Strength: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        Serial.println("====================================================");
    }
}

void sendTelegramTextMessage(const String& labelText) {
    // Verify local Wi-Fi state before initiating an external radio network handshake
    if (WiFi.status() != WL_CONNECTED) {
        if (Serial) Serial.println("[ERROR] Wi-Fi link dropped. Aborting text telemetry.");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(5); // Fast 5-second timeout for text packets

    if (!client.connect("api.telegram.org", 443)) {
        if (Serial) Serial.println("[ERROR] Connection to Telegram text gateway failed.");
        return;
    }

    // Construct standard URL-encoded text message parameters
    // Format: "TIMELAPSE | Link Strength: -34 dBm"
    String messageText = labelText + " | Link Strength: " + String(WiFi.RSSI()) + " dBm";
    
    // Replace any spaces with URL-safe equivalents (%20) to prevent HTTP protocol breaks
    messageText.replace(" ", "%20");
    
    String urlPath = "/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + messageText;

    // Execute standard lightweight HTTP GET request
    client.println("GET " + urlPath + " HTTP/1.1");
    client.println("Host: api.telegram.org");
    client.println("Connection: close");
    client.println();

    if (Serial) {
        Serial.print("[SUCCESS] Text telemetry RSSI message (");
        Serial.print(messageText);
        Serial.println(") successfully delivered via secure radio stacks.");
    }
}


void sendTelegramJpgFile(const String& base64Str, const String& gestureName) {
    String boundary = "----ESP32C3Boundary";

    // Call our abstracted alignment function to map out memory arrays natively
    uint32_t totalPayloadLen = assembleMultipartBuffer(base64Str, gestureName, boundary);
    Serial.print("[NETWORK] Total Consolidated Payload Size: ");
    Serial.println(totalPayloadLen);

    if (totalPayloadLen == 0) {
        if (Serial) Serial.println("[WARNING] Aborting network send due to buffer assembly failure.");
        return;
    }

    // Verify local Wi-Fi state before initiating an external radio network handshake
    if (WiFi.status() != WL_CONNECTED) {
        if (Serial) Serial.println("[ERROR] Wi-Fi link dropped. Aborting secure upload.");
        return;
    }

    WiFiClientSecure client;
    client.setInsecure(); 
    client.setTimeout(8); // Generous 8s window protects against garden network path fragmentation

    if (!client.connect("api.telegram.org", 443)) {
        if (Serial) Serial.println("[ERROR] Connection to secure Telegram gateway endpoint failed.");
        return;
    }

    // SEND SYSTEM MANIFEST HTTP POST ROUTING HEADERS
    client.println("POST /bot" + botToken + "/sendPhoto HTTP/1.1");
    client.println("Host: api.telegram.org");
    client.println("Content-Length: " + String(totalPayloadLen));
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println("Connection: close");
    client.println();

    // THE HIGH-SPEED INDIVISIBLE PUSH
    // Pushes the exact total composite payload block in one solid packet over the air rails
    client.write(staticBinaryBuffer, totalPayloadLen);

    if (Serial) {
        Serial.print("[SUCCESS] Indivisible payload package (");
        Serial.print(totalPayloadLen);
        Serial.println(" bytes) successfully delivered via secure radio stacks.");
    }
}


uint32_t assembleMultipartBuffer(const String& base64Str, const String& gestureName, const String& boundary) {
    // 1. PRE-FLIGHT BOUNDS CALCULATION
    size_t estimatedBinaryLen = (base64Str.length() * 3) / 4;

    // 2. CONSTRUCT INDIVIDUAL TEXT HEAD AND TAIL RAILS
    String head = "--" + boundary + "\r\n" +
                  "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + chatID + "\r\n" +
                  "--" + boundary + "\r\n" +
                  "Content-Disposition: form-data; name=\"caption\"\r\n\r\nMatch: " + gestureName + " (RSSI: " + String(WiFi.RSSI()) + "dBm)\r\n" +
                  "--" + boundary + "\r\n" +
                  "Content-Disposition: form-data; name=\"photo\"; filename=\"capture.jpg\"\r\n" +
                  "Content-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--" + boundary + "--\r\n";

    // 3. RUN STRUCTURAL SIZE EVALUATION GATE
    uint32_t totalPayloadLen = head.length() + estimatedBinaryLen + tail.length();

    if (totalPayloadLen > STATIC_BUFFER_SIZE) {
        if (Serial) {
            Serial.println("\n[❌ MEMORY EXCEPTION] Composite allocation limit exceeded!");
            Serial.print(" -> Required Buffer Size: "); Serial.print(totalPayloadLen); Serial.println(" bytes.");
            Serial.print(" -> Static Buffer Limit:  "); Serial.print(STATIC_BUFFER_SIZE); Serial.println(" bytes.");
        }
        return 0; // Returns 0 as an explicit error signature to tell the caller to abort
    }

    // 4. DECODE RAW BINARY DIRECTLY INTO BASE POSITION ZERO FOR STRICT 4-BYTE ALIGNMENT
    size_t actualBinaryLen = 0;
    int decodeStatus = mbedtls_base64_decode(
        staticBinaryBuffer,              
        STATIC_BUFFER_SIZE,              
        &actualBinaryLen, 
        (const unsigned char*)base64Str.c_str(), 
        base64Str.length()
    );

    if (decodeStatus != 0) {
        if (Serial) {
            Serial.print("[❌ CODEC ERROR] Base64 image decode failed. Status: ");
            Serial.println(decodeStatus);
        }
        return 0; 
    }

    // 5. STITCH PACKET COMPONENT RAILS INSIDE STATIC GLOBAL SPACE
    // Step A: Shift the raw binary data cleanly down the buffer to carve out room for headers
    memmove(staticBinaryBuffer + head.length(), staticBinaryBuffer, actualBinaryLen);

    // Step B: Copy the text header string directly into the newly opened front slot
    memcpy(staticBinaryBuffer, head.c_str(), head.length());
    
    // Step C: Append the multipart footer text right after the shifted binary payload ends
    memcpy(staticBinaryBuffer + head.length() + actualBinaryLen, tail.c_str(), tail.length());

    // 6. CALCULATE AND RETURN PRECISE TOTAL COMBINED LENGTH
    totalPayloadLen = head.length() + actualBinaryLen + tail.length();
    return totalPayloadLen;
}


