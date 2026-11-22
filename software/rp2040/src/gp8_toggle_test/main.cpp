// GP8 GPIO test: drive GP8 HIGH/LOW alternately (1 s each), no UART.
// The ESP32 uart_link_test prints the IO16 level every second.
#include <Arduino.h>
void setup() {
  Serial.begin(115200);
  pinMode(8, OUTPUT);
}
void loop() {
  static bool level = false;
  level = !level;
  digitalWrite(8, level);
  Serial.printf("GP8 -> %d (readback %d)\n", level, digitalRead(8));
  delay(1000);
}
