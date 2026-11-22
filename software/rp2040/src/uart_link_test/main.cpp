// UART link test (RP2040 side). UART1: TX=GP8 -> ESP32 IO16, RX=GP9 <- ESP32 IO17
// Sends "P<n>\n" every second and echoes whatever the ESP32 sends to USB serial.
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  bool tx_ok = Serial2.setTX(8);
  bool rx_ok = Serial2.setRX(9);
  Serial2.begin(9600, SERIAL_8N1);
  delay(500);
  Serial.printf("RP2040 link test: setTX(8)=%d setRX(9)=%d\n", tx_ok, rx_ok);
}

void loop() {
  static uint32_t last = 0, n = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Serial2.printf("P%lu\n", n);
    Serial.printf("sent P%lu\n", n);
    n++;
  }
  while (Serial2.available()) {
    int c = Serial2.read();
    Serial.printf("RX from ESP32: 0x%02X '%c'\n", c, (c >= 32 && c < 127) ? c : '.');
  }
}
