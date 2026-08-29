#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// Exact UUID configurations matching your production environment
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define COUNTER_CHAR_UUID       "d8a1c322-1f45-4e7d-8700-d5382c474d21"

BLEServer* pServer = nullptr;
BLECharacteristic* pCounterChar = nullptr;
uint32_t totalMatchCounter = 0;

unsigned long powerTimer = 0;
bool isRadioAsleep = false;

void setup() {
    // 🌟 INITIALISED ONCE AT BOOT: Keeps USB COM port stable
    Serial.begin(115200);

    // Quick 3-second startup safety timeout loop
    unsigned long startWindow = millis();
    while (!Serial && (millis() - startWindow < 3000)) {
        delay(10);
    }

    Serial.println("\n=========================================================");
    Serial.println("[⚡ BENCH RUN] Starting Clean Isolation Power Script...");
    Serial.println("=========================================================");

    // Initialize core Bluetooth radio hardware layers
    BLEDevice::init("GC-BLE");
    BLEDevice::setPower(ESP_PWR_LVL_N3); // Quieted power index

    pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Build independent match counter characteristic
    pCounterChar = pService->createCharacteristic(
        COUNTER_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCounterChar->addDescriptor(new BLE2902());
    pCounterChar->setValue((uint8_t*)&totalMatchCounter, 4);

    // Apply the 500ms timing window optimizations to smooth out spikes
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinInterval(800); // 800 * 0.625ms = 500ms sleep pace
    pAdvertising->setMaxInterval(800);

    pService->start();
    BLEDevice::startAdvertising();

    powerTimer = millis();
}

void loop() {
    unsigned long currentMillis = millis();

    // 🌟 STATE 1: ACTIVE BASELINE TRANSMITTER RUN (Lasts 6 seconds)
    if (!isRadioAsleep) {
        if (currentMillis % 2000 < 20) {
            Serial.print("[⚡ RADIO ACTIVE] Broadcasting BLE beacons. Uptime: ");
            Serial.print(currentMillis / 1000); Serial.println("s");
        }

        if (currentMillis - powerTimer >= 6000) {
            Serial.println("\n---------------------------------------------------------");
            Serial.println("[💤 SHUTDOWN] Completely defuelling high-power RF registers...");
            Serial.println(" -> USB lines remain alive. Observe 5V bench meter now!");
            Serial.println("---------------------------------------------------------\n");
            Serial.flush();

            // De-initialize the core Bluetooth radio hardware completely
            BLEDevice::deinit(true);

            isRadioAsleep = true;
            powerTimer = millis();
        }
    }
    // 🌟 STATE 2: RADIO SHUTDOWN ISOLATION RUN (Lasts 6 seconds)
    else {
        if (currentMillis % 2000 < 20) {
            Serial.print("[💤 SILENT HIBERNATION] Antenna is completely OFF. Uptime: ");
            Serial.print(currentMillis / 1000); Serial.println("s");
        }

        if (currentMillis - powerTimer >= 6000) {
            Serial.println("\n---------------------------------------------------------");
            Serial.println("[☀️ RE-ARM] Powering up local RF amplifier cells...");
            Serial.println("---------------------------------------------------------\n");
            Serial.flush();

            // Reinitialise the radio hardware smoothly
            BLEDevice::init("GC-BLE");
            BLEDevice::setPower(ESP_PWR_LVL_N3);

            // Re-bind advertising layers to keep it discoverable on your scan loops
            BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
            pAdvertising->addServiceUUID(SERVICE_UUID);
            pAdvertising->setMinInterval(800);
            pAdvertising->setMaxInterval(800);
            BLEDevice::startAdvertising();

            isRadioAsleep = false;
            powerTimer = millis();
        }
    }
}
