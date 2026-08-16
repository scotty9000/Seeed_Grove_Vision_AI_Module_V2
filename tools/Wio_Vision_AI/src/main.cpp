#include <Arduino.h>
#include <Wire.h>
#include <Seeed_Arduino_SSCMA.h>
#include "mbedtls/base64.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

SSCMA AI;

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

// Handlers for Slot Selection & Stream Chunking
class ControlCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        std::string rawValue = pCharacteristic->getValue();
        if (rawValue.length() > 0) {
            int requestedSlot = rawValue[0] - '0'; // Convert ASCII char digit to literal int
            if (requestedSlot >= 0 && requestedSlot < BUFFER_SLOTS) {
                selectedSlot = requestedSlot;
                currentOffset = 0; // Reset chunk pointer to the beginning of the new picture
                
                // Dynamically update your phone view with what target is saved inside that slot
                String meta = "Slot [" + String(selectedSlot) + "] Meta: " + imageMetadata[selectedSlot] + " | Length: " + String(imageLengths[selectedSlot]) + " bytes";
                pMetadataChar->setValue(meta.c_str());
                pMetadataChar->notify();

                // Prime the first 240 bytes of raw data inside the stream register
                uint32_t totalLen = imageLengths[selectedSlot];
                uint32_t chunkLen = (totalLen > 240) ? 240 : totalLen;
                if (totalLen > 0) {
                    pDataStreamChar->setValue(&imageRingBuffer[selectedSlot][0], chunkLen);
                    currentOffset += chunkLen;
                } else {
                    pDataStreamChar->setValue("No Data");
                }
                
                if (Serial) {
                    Serial.print("[🎯 BLE CONTROL] Phone selected Slot: "); Serial.print(selectedSlot);
                    Serial.print(" | Meta parsed: "); Serial.println(imageMetadata[selectedSlot]);
                }
            }
        }
    }

    void onRead(BLECharacteristic* pCharacteristic) {
        // When your phone reads Characteristic 2 (Data Chunker), automatically advance 
        // forward to package the NEXT sequential 240-byte slice of the image
        uint32_t totalLen = imageLengths[selectedSlot];
        if (currentOffset < totalLen) {
            uint32_t remaining = totalLen - currentOffset;
            uint32_t chunkLen = (remaining > 240) ? 240 : remaining;
            
            pDataStreamChar->setValue(&imageRingBuffer[selectedSlot][currentOffset], chunkLen);
            currentOffset += chunkLen;
            
            if (Serial) {
                Serial.print("[📤 BLE STREAM] Piped chunk offset: "); Serial.print(currentOffset);
                Serial.print("/"); Serial.print(totalLen); Serial.println(" bytes down the air rails.");
            }
        } else {
            // End of File marker signature to signal your terminal app that the JPEG is finished
            pDataStreamChar->setValue("EOF");
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
    BLEDevice::init("XIAO-GARDEN-CAM");
    
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
}


void loop() {
    unsigned long currentMillis = millis();

    // ⏱️ AUTOMATIC ANTI-FLOOD LOCKOUT RELEASE CHECKER
    if (isLockoutActive && (currentMillis - lockoutTimerStart >= ANTI_FLOOD_INTERVAL)) {
        if (Serial) Serial.println("\n[⏱️ LOCKOUT SYSTEM] Cooldown window expired. Buffers ready for next target.");
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
                Serial.print("\n[❌ I2C ERROR] Camera bus read failed. Status: ");
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
                    Serial.print("[🚀 TARGET DETECTED] Valid Match: "); Serial.println(targetName);
                    Serial.print("[🚀 TARGET DETECTED] Confidence: "); Serial.print(confidence); Serial.println("%");
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
            Serial.print("[❌ CACHE FAULT] Base64 decoding failed. Status: ");
            Serial.println(decodeStatus);
        }
        return;
    }

    // Save the metrics profile into the matching tracker arrays
    imageLengths[writeIndex] = actualBinaryLen;
    imageMetadata[writeIndex] = "Target: " + labelName + " (" + String(confidence) + "%)";

    if (Serial) {
        Serial.println("\n====================================================");
        Serial.print("[💾 MEMORY BUFFER] Image stored in Slot ["); Serial.print(writeIndex); Serial.println("]");
        Serial.print("[💾 MEMORY BUFFER] Payload Size: "); Serial.print(actualBinaryLen); Serial.println(" bytes.");
        Serial.print("[💾 MEMORY BUFFER] Active Slot Label: "); Serial.println(imageMetadata[writeIndex]);
        Serial.println("====================================================");
    }

    // Advance the write pointer (wraps around automatically from 3 back to 0)
    writeIndex = (writeIndex + 1) % BUFFER_SLOTS;
}


