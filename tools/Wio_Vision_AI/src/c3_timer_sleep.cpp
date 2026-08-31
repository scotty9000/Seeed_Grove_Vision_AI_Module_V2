#include <Arduino.h>

// Define sleep time in microseconds (1 second = 1,000,000 microseconds)
#define TIME_TO_SLEEP_SEC  10                       // Sleep for 10 seconds
#define uS_TO_S_FACTOR     1000000ULL               // Conversion factor for microseconds

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Wait for serial monitor to connect (optional, for debugging)
  delay(2000);

  Serial.println("XIAO ESP32-C3 has awoken!");

  // --- Insert your main code task here (e.g., read sensor, send data) ---
  Serial.println("Performing tasks before going to sleep...");
  delay(1000);
  // ----------------------------------------------------------------------

  // Configure the timer wake-up source
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP_SEC * uS_TO_S_FACTOR);
  Serial.printf("Setup deep sleep for %d seconds.\n", TIME_TO_SLEEP_SEC);

  Serial.println("Going to sleep now. Goodnight!");
  Serial.flush(); // Ensure all serial data is sent before powering down

  // Enter deep sleep mode
  esp_deep_sleep_start();

  // Code past this point will never execute.
  // When the ESP32-C3 wakes up, it starts again from the beginning of setup().
}

void loop() {
  // This remains empty because the chip sleeps before ever reaching loop()
}
