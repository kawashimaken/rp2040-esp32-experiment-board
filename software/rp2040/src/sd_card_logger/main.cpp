// C++ port of the original MicroPython test_sd_card.py
// Every 5 seconds, appends a record to log.txt on the SD card and blinks the LED.
//
// SD card (SPI mode): CLK=GP5, CMD(MOSI)=GP18, DAT0(MISO)=GP19, DAT3/CS=GP22
// These pins do not fit the RP2040 hardware SPI roles, so a PIO-based
// SoftwareSPI is used (the MicroPython version used SoftSPI for the same reason).
#include <Arduino.h>
#include <SD.h>
#include <SoftwareSPI.h>

static const uint8_t SD_SCK = 5;
static const uint8_t SD_MOSI = 18;
static const uint8_t SD_MISO = 19;
static const uint8_t SD_CS = 22;
static const uint8_t LED_PIN = 25;

static SoftwareSPI sdSpi(SD_SCK, SD_MISO, SD_MOSI);

static void blink_twice() {
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  if (!SD.begin(SD_CS, sdSpi)) {
    Serial.println("SD card mount failed");
    // Fast blink forever so the failure is visible without USB serial
    while (true) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(100);
    }
  }
  Serial.println("SD card mounted");
}

void loop() {
  // Same fixed values as the original script
  const int temperature = 27;
  const int time = 5;

  File f = SD.open("log.txt", FILE_WRITE);  // FILE_WRITE appends
  if (f) {
    f.println("Time: ");
    f.println(time);
    f.println("Temp: ");
    f.println(temperature);
    f.println(" ");
    f.close();
    Serial.println("appended to log.txt");
  } else {
    Serial.println("failed to open log.txt");
  }

  blink_twice();
  delay(4700);
}
