#include <Arduino.h>
#include <Wire.h>
#include <Seeed_Arduino_SSCMA.h>
#include "mbedtls/base64.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

SSCMA AI;

#define TEST_IMAGE_SIZE 354

// 🌟 TEST SUITE: Standard 16x16 pixel bright red square JPEG file layout profile
const uint8_t PROGMEM testBlackSpot16x16[TEST_IMAGE_SIZE] = {
  0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x01, 0x00, 0x48,
  0x00, 0x48, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x03, 0x02, 0x02, 0x03, 0x02, 0x02, 0x03,
  0x03, 0x03, 0x03, 0x04, 0x03, 0x03, 0x04, 0x05, 0x08, 0x05, 0x05, 0x04, 0x04, 0x05, 0x0A, 0x07,
  0x07, 0x06, 0x08, 0x0C, 0x0A, 0x0C, 0x0C, 0x0B, 0x0A, 0x0B, 0x0B, 0x0D, 0x0E, 0x12, 0x10, 0x0D,
  0x0E, 0x11, 0x0E, 0x0B, 0x0B, 0x10, 0x16, 0x10, 0x11, 0x13, 0x14, 0x15, 0x15, 0x15, 0x0C, 0x0F,
  0x17, 0x18, 0x16, 0x14, 0x18, 0x12, 0x14, 0x15, 0x14, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x10,
  0x00, 0x10, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0x1F, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
  0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0xFF, 0xC4, 0x00, 0xB5, 0x10, 0x00, 0x02, 0x01, 0x03,
  0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00,
  0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32,
  0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72,
  0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35,
  0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55,
  0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75,
  0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94,
  0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2,
  0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
  0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,
  0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA,
  0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00, 0xFC, 0x24, 0x28, 0xA2, 0x8A, 0x28, 0xA2, 0x8A,
  0x28, 0xA2, 0x8A, 0x28, 0xA2, 0x8A, 0x28, 0xA2, 0x8A, 0x28, 0xA2, 0x8A, 0x28, 0xA2, 0x8A, 0x3F,
  0xFF, 0xD9
};


// 📦 FIXED STATIC HEAP RING BUFFER (4 slots x 8000 bytes = ~32 KB)
#define BUFFER_SLOTS 4
#define IMAGE_SLOT_SIZE 8000

unsigned char imageRingBuffer[BUFFER_SLOTS][IMAGE_SLOT_SIZE];
uint32_t imageLengths[BUFFER_SLOTS] = {0, 0, 0, 0};
String imageMetadata[BUFFER_SLOTS] = {"Empty Slot", "Empty Slot", "Empty Slot", "Empty Slot"};

int writeIndex = 0;      // Tracks where the camera writes the next fresh image
int selectedSlot = 0;    // Tracks which slot your phone app wants to read
uint32_t currentOffset = 0; // Tracks data chunk position during transmission


// BLE REWRITE SERVICE & CHARACTERISTIC AUTO-GENERATION UUIDS
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define METADATA_CHAR_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26a8" // Read Only
#define CONTROL_CHAR_UUID      "e3223119-9445-4e7d-8700-d5382c474d21" // Read/Write
#define DATA_STREAM_CHAR_UUID  "622a5785-5ee6-4e58-9cf8-6628fb05cf71" // Read Only

BLEServer* pServer = NULL;
BLECharacteristic* pMetadataChar = NULL;
BLECharacteristic* pControlChar = NULL;
BLECharacteristic* pDataStreamChar = NULL;
bool deviceConnected = false;


void storeImageInRingBuffer(const String& base64Str, const String& labelName, int confidence);

class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        if (Serial) Serial.println("\n[📱 BLE] Phone linked successfully! Synchronizing links...");
    }
    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        if (Serial) Serial.println("\n[❌ BLE] Phone disconnected. Returning to camera monitoring loop.");
        // Restart advertising so it can be re-found in the garden
        BLEDevice::startAdvertising();
    }
};

// Handlers for Bounded Slot Selection and Stateless Chunk Extraction
class ControlCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string rawValue = pCharacteristic->getValue();
        
        // 🌟 RAW DIAGNOSTIC SCANNER: Fires instantly before ANY if-statements or variables process
        if (Serial) {
            Serial.println("\n========================================================");
            Serial.print("[⚙️ HARDWARE INTERCEPT] Incoming packet length: "); 
            Serial.print(rawValue.length()); Serial.println(" bytes.");
            
            Serial.print(" -> Byte Contents (Hex): ");
            for(size_t i = 0; i < rawValue.length(); i++) {
                Serial.print("0x");
                if((unsigned char)rawValue[i] < 16) Serial.print("0");
                Serial.print((unsigned char)rawValue[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            Serial.println("========================================================");
        }
        
        if (rawValue.length() > 0) {
            char slotChar = rawValue[0]; // Intercept the primary action character
            
            // 🌟 THE CLEAR ALL COMMAND ENGINE
            // Triggers if you send the character 'C' (ASCII 67) or raw machine byte 0x43
            if (slotChar == 'C' || slotChar == 67) {
                for (int i = 0; i < BUFFER_SLOTS; i++) {
                    imageLengths[i] = 0; // Resetting size registers to 0 declares them "empty"
                    imageMetadata[i] = "Empty Slot (Wiped)";
                }
                writeIndex = 0; // Rewind the camera's injection pointer back to the beginning
                
                // Push an instant status update down your phone display line
                pMetadataChar->setValue("All Memory Slots Cleared Successfully.");
                pMetadataChar->notify();
                
                if (Serial) {
                    Serial.println("\n================================================");
                    Serial.println("[🧹 MEMORY CLEAR] Wiped all 4 static SRAM slots.");
                    Serial.println(" -> System counters and writeIndex reset to zero.");
                    Serial.println("================================================");
                }
                return; // Exit function early after successful flush execution
            }

            // Check if browser sent a 2-byte packet (Byte 0: Slot choice, Byte 1: Requested Packet Number)
            if (rawValue.length() == 2) {
                int requestedSlot = (unsigned char)rawValue[0];
                int requestedPacket = (unsigned char)rawValue[1];
                
                if (requestedSlot >= 0 && requestedSlot < BUFFER_SLOTS) {
                    selectedSlot = requestedSlot;
                    
                    uint32_t totalLen = imageLengths[selectedSlot];
                    uint32_t targetOffset = requestedPacket * 240;
                    
                    if (targetOffset < totalLen) {
                        uint32_t remaining = totalLen - targetOffset;
                        uint32_t chunkLen = (remaining > 240) ? 240 : remaining;
                        
                        // Extract the exact binary row index memory pointer offset address natively
                        uint8_t* pChunkAddress = (uint8_t*)&imageRingBuffer[selectedSlot][targetOffset];
                        pDataStreamChar->setValue(pChunkAddress, chunkLen);
                        
                        if (Serial) {
                            Serial.print("[📥 BLE TRACK] Handed Browser Call #"); Serial.print(requestedPacket);
                            Serial.print(" | Slot: "); Serial.print(selectedSlot);
                            Serial.print(" | Size: "); Serial.print(chunkLen);
                            Serial.print(" bytes from offset: "); Serial.println(targetOffset);
                        }
                    } else {
                        pDataStreamChar->setValue((uint8_t*)"EOF", 3);
                    }
                }
                return; // Exit out of the packet index handler safely
            }
            
            // Standard single-byte setup gate (Used during initial connection initialization)
            int requestedSlot = ((unsigned char)slotChar < 4) ? (int)slotChar : (slotChar - '0');
            
            if (requestedSlot >= 0 && requestedSlot < BUFFER_SLOTS) {
                selectedSlot = requestedSlot;
                
                String meta = "Slot [" + String(selectedSlot) + "] Meta: " + imageMetadata[selectedSlot] + " | Length: " + String(imageLengths[selectedSlot]) + " bytes";
                pMetadataChar->setValue(meta.c_str());
                pMetadataChar->notify();
                
                // Pre-load Call #0 data bytes straight into the stream register out of the gate
                uint32_t totalLen = imageLengths[selectedSlot];
                uint32_t chunkLen = (totalLen > 240) ? 240 : totalLen;
                if (totalLen > 0) {
                    pDataStreamChar->setValue((uint8_t*)&imageRingBuffer[selectedSlot][0], chunkLen);
                } else {
                    pDataStreamChar->setValue((uint8_t*)"No Data", 7);
                }
            }
        }
    }
};

// loop control
unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 200; // Snappy 200ms camera scan rate
unsigned long lastHeartbeatTime = 0;

// ⏱️ ANTI-FLOOD LOCKOUT SYSTEM
const unsigned long ANTI_FLOOD_INTERVAL = 15000; 
unsigned long lockoutTimerStart = 0;             
bool isLockoutActive = false; 

// FIXED TIME-LAPSE TELEMETRY INTERVAL
unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 60000; // Sent exactly every 60 seconds (1 minute)

void setup() {
    Serial.begin(115200);
    
    unsigned long startWindow = millis();
    while (!Serial && (millis() - startWindow < 3000)) {
        delay(10);
    }
    
    if (Serial) {
        Serial.println("\n================================================");
        Serial.println("[🔋 STANDALONE BLE CAPTURE] Booting Local Memory Rig...");
        Serial.println("================================================");
    }

    // Initialize I2C layers cleanly
    Wire.begin(6, 7); 
    Wire.setClock(400000); 
    
    if (!AI.begin(&Wire)) {
        if (Serial) Serial.println("[❌ ERROR] Camera board not found over I2C.");
        while (1) { delay(1000); }
    }
    if (Serial) Serial.println("[SUCCESS] Camera board online over I2C.");
    delay(500);

    // 🌟 INITIATE SECURE STANDALONE BLE CONTROLLER
    BLEDevice::init("GC-BLE");
    
    // Configure local radio layers for optimal transmission
    BLEDevice::setPower(ESP_PWR_LVL_P9); // Fire radio at full strength for garden range penetration

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Create Core Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Characteristic A: Metadata Info display line (Read-Only)
    pMetadataChar = pService->createCharacteristic(
                      METADATA_CHAR_UUID,
                      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                    );
    pMetadataChar->addDescriptor(new BLE2902());
    pMetadataChar->setValue("No image requested yet. Write slot 0-3 to Control characteristic.");

    // Characteristic B: Slot selector command terminal (Read/Write)
    pControlChar = pService->createCharacteristic(
                     CONTROL_CHAR_UUID,
                     BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
                   );
    pControlChar->setCallbacks(new ControlCallbacks());

    // Characteristic C: High-Speed sequential data stream rail (Read-Only)
    pDataStreamChar = pService->createCharacteristic(
                        DATA_STREAM_CHAR_UUID,
                        BLECharacteristic::PROPERTY_READ
                      );

    // Launch background services
    pService->start();

    // Configure Advertising wrapper so your phone can find the device offline
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions help with iPhone connection speed syncing
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    if (Serial) {
        Serial.println("[🎉 INITIALIZED] Offline BLE Array Engine Broadcasting.");
        Serial.println(" -> Bluetooth Device Name: XIAO-GARDEN-CAM");
        Serial.println(" -> Ready to buffer target alerts natively into RAM.");
        Serial.println("================================================");
    }

    // TEST INJECTION: Permanently lock the 270-byte red square into Slot 0
    memcpy(imageRingBuffer[0], testBlackSpot16x16, TEST_IMAGE_SIZE);
    imageLengths[0] = TEST_IMAGE_SIZE;
    imageMetadata[0] = "Target: RED_SQUARE_TEST (100%) | Length: 354 bytes";
    
    // 🛡️ SECURITY PROTECTION: Shift the camera's write pointer to Slot 1.
    // This forces the AI to store live captures in Slot 1 next, keeping Slot 0 protected.
    writeIndex = 1; 
    
    if (Serial) {
        Serial.println("===============================================================");
        Serial.println("[TEST IMAGE] 354-Byte 16x16 Black Spot loaded into Slot 0.");
        Serial.println(" AI model redirected to store live frames in Slot 1.");
        Serial.println("===============================================================");
    }
}


void loop() {
    unsigned long currentMillis = millis();

    // ⏱️ AUTOMATIC ANTI-FLOOD LOCKOUT RELEASE CHECKER
    if (isLockoutActive && (currentMillis - lockoutTimerStart >= ANTI_FLOOD_INTERVAL)) {
        if (Serial) Serial.println("\n[LOCKOUT SYSTEM] Cooldown window expired. Buffers ready for next target.");
        isLockoutActive = false;
    }

    // ⏱️ PRE-FLIGHT LOCKOUT GATE: Skip camera scanning if the system is cooling down
    if (isLockoutActive) {
        return; 
    }

    // IDLE HEARTBEAT: Only prints asterisks if a computer is actively listening
    if (currentMillis - lastHeartbeatTime >= 3000) {
        lastHeartbeatTime = currentMillis;
        if (Serial) Serial.print("*"); 
    }

    // STANDARD AI PERSON TRACKING WINDOW
    if (currentMillis - lastCheckTime >= checkInterval) {
        lastCheckTime = currentMillis;

        // Run AI evaluation pass
        int status = AI.invoke(1, false, true);

        if (status < 0) {
            if (Serial) {
                Serial.print("\n[I2C ERROR] Camera bus read failed. Status: ");
                Serial.println(status);
            }
            return; 
        }

        // Check if the AI model found any matching target objects
        if (AI.boxes().size() > 0) {
            int currentID = AI.boxes()[0].target;
            int confidence = AI.boxes()[0].score;

            if (confidence > 60) {
                
                // ENGAGE ANTI-FLOOD TIMER IMMEDIATELY
                lockoutTimerStart = currentMillis; 
                isLockoutActive = true; 

                // Remap the incoming data index for the Person YOLO model
                String targetName = "";
                if (currentID == 0) {
                    targetName = "PERSON"; 
                } else {
                    targetName = "UNKNOWN"; 
                }

                if (Serial) {
                    Serial.println("\n================================================");
                    Serial.print("[TARGET DETECTED] Valid Match: "); Serial.println(targetName);
                    Serial.print("[TARGET DETECTED] Confidence: "); Serial.print(confidence); Serial.println("%");
                    Serial.println("================================================");
                }

                // Extract the raw Base64 data string from the camera registers
                String rawBase64 = AI.last_image();

                // Save the data directly inside your 4-slot static ring buffer array
                storeImageInRingBuffer(rawBase64, targetName, confidence);
                
                // Clear hardware registers to completely resolve retrigger loops
                AI.invoke(1, true, false); 
            }
        }
    }
}

void storeImageInRingBuffer(const String& base64Str, const String& labelName, int confidence) {
    if (base64Str.length() == 0) {
        if (Serial) Serial.println("[⚠️ DIAGNOSTIC] Base64 string is entirely empty!");
        return;
    }

    if (Serial) {
        Serial.println("\n=================== [STORAGE DIAGNOSTICS] ===================");
        Serial.print(" -> Raw Base64 String Character Length: "); Serial.println(base64Str.length());
        Serial.print(" -> Front Base64 Characters: ");
        for (int i = 0; i < min((int)base64Str.length(), 15); i++) {
            Serial.print(base64Str[i]);
        }
        Serial.println("...");
    }

    size_t actualBinaryLen = 0;
    
    // Decode Base64 straight into the exact 4-byte aligned slot pointer row
    int decodeStatus = mbedtls_base64_decode(
        imageRingBuffer[writeIndex],              
        IMAGE_SLOT_SIZE,              
        &actualBinaryLen, 
        (const unsigned char*)base64Str.c_str(), 
        base64Str.length()
    );

    if (decodeStatus != 0) {
        if (Serial) {
            Serial.print(" -> [DECODE ERROR] mbedtls_base64_decode failed! Code: ");
            Serial.println(decodeStatus);
            Serial.println("===============================================================");
        }
        return;
    }

    // Save the metrics profile into the matching tracker arrays
    imageLengths[writeIndex] = actualBinaryLen;
    imageMetadata[writeIndex] = "Target: " + labelName + " (" + String(confidence) + "%)";

    if (Serial) {
        Serial.print(" -> [MEMORY OK] Stored binary array row inside Slot ["); Serial.print(writeIndex); Serial.println("]");
        Serial.print(" -> Decoded Binary Output Length: "); Serial.print(actualBinaryLen); Serial.println(" bytes.");
        Serial.print(" -> First 6 Decoded Bytes (Hex): ");
        for (size_t i = 0; i < min((size_t)actualBinaryLen, (size_t)6); i++) {
            Serial.print("0x");
            if (imageRingBuffer[writeIndex][i] < 16) Serial.print("0");
            Serial.print(imageRingBuffer[writeIndex][i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        Serial.println("===============================================================");
    }

    // Advance the write pointer index safely
    writeIndex = (writeIndex + 1) % BUFFER_SLOTS;
}



