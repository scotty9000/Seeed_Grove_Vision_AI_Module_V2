// Write some code to explore the uart (RHS) link to Grove Visual AI v2
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial1.begin(115200);

  Serial.println("Wio Terminal UART test started");
}

void loop() {
  if (Serial1.available()) {
    Serial.print("RX: ");
    while (Serial1.available()) {
      Serial.write(Serial1.read());
    }
    Serial.println();
  }

  Serial.println("-");
  delay(1000);
}
