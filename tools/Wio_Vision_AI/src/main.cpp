#include <Seeed_Arduino_SSCMA.h>
#include <Wire.h> 
#include <TFT_eSPI.h> 

SSCMA AI;
TFT_eSPI tft = TFT_eSPI();

const int BUZZER_PIN = WIO_BUZZER; // Onboard Wio Terminal Buzzer

void setup() {

        // Open the serial port so it can log to a PC IF one happens to be plugged in
    Serial.begin(115200);
    
    // REMOVED: while (!Serial) { delay(10); } 
    // This allows the Wio Terminal to skip past the computer check instantly!

    // Give the hardware and screen layers exactly 2 seconds to stabilize on boot
    delay(2000); 

    // Initialize LCD Screen
    tft.init();
    tft.setRotation(3); 
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.drawString("Event Stream Mode...", 10, 10);
    tft.drawLine(0, 35, 320, 35, TFT_BLUE);

    // Initialize Buzzer Pin
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // Start I2C physical communication
    Wire.begin(); 
    Wire.setClock(400000); // 400kHz matches the high-speed data stream capabilities
    
    if (!AI.begin(&Wire)) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        tft.drawString("Camera Wire Dead!", 10, 40);
        while (1); 
    }

    tft.fillRect(0, 40, 320, 200, TFT_BLACK);
    tft.setTextColor(TFT_DARKGREEN);
    tft.drawString("[SCANNING] Awaiting stream...", 10, 110);
    Serial.println("System Ready. Listening for filtered hardware stream events...");
}

void loop() {
    // HARDWARE STREAM CONFIGURATION:
    // Parameter 1: 1     -> Evaluate current frame capture layout.
    // Parameter 2: true  -> STREAM FILTER MODE active. Only pull structural delta updates!
    // Parameter 3: false -> Do not request raw JPEG binary visual images over the line.
    int status = AI.invoke(1, true, false);
    
    if (status == 0) { 
        int targetCount = AI.boxes().size();
        
        if (targetCount > 0) {
            // A dynamic event triggered! A target entered or moved in the frame
            tft.fillRect(0, 80, 320, 160, TFT_BLACK); 
            tft.setTextColor(TFT_RED);
            tft.drawString("ALERT: Person Spotted!", 10, 100);
            
            for (int i = 0; i < targetCount; i++) {
                int score = AI.boxes()[i].score; 
                tft.setTextColor(TFT_WHITE);
                tft.drawString("Confidence: " + String(score) + "%", 10, 130 + (i * 20));
                
                Serial.print("[STREAM MATCH] Target Found. Confidence: ");
                Serial.print(score);
                Serial.println("%");
            }

            // Clean, sharp alert beep
            analogWrite(BUZZER_PIN, 128); 
            delay(100);
            analogWrite(BUZZER_PIN, 0); 
        } 
        else {
            // A stream update triggered indicating the region is empty again
            tft.fillRect(0, 80, 320, 160, TFT_BLACK); 
            tft.setTextColor(TFT_DARKGREEN);
            tft.drawString("[SCANNING] Area Clear.", 10, 110);
            Serial.println("[STREAM RESET] Environment clear.");
        }
    }
    
    // 60ms pause gives the Wio Core breathing room, 
    // but data delivery is entirely managed by the camera module.
    delay(60); 
}
