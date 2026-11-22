// UART link test (ESP32 side). UART2: RX=IO16 <- RP2040 GP8, TX=IO17 -> RP2040 GP9
// Sends "E<n>\n" every second and prints whatever the RP2040 sends.
#include <Arduino.h>

HardwareSerial Link(2);

void setup() {
  Serial.begin(9600);
  Link.begin(9600, SERIAL_8N1, 16, 17);
  delay(200);
  Serial.println("ESP32 link test");
}

void loop() {
  static uint32_t last = 0, n = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Link.printf("E%lu\n", n);
    Serial.printf("sent E%lu  (IO16 level=%d)\n", n, digitalRead(16));
    n++;
  }
  while (Link.available()) {
    int c = Link.read();
    Serial.printf("RX from RP2040: 0x%02X '%c'\n", c, (c >= 32 && c < 127) ? c : '.');
  }
}
