// C++ port of the original MicroPython test_sd_card.py
// Every 5 seconds, appends a record to log.txt on the SD card and blinks the LED.
//
// SD card (SPI mode): CLK=GP5, CMD(MOSI)=GP18, DAT0(MISO)=GP19, DAT3/CS=GP22
// Uses SdFat's FAT code on top of BitbangSdCard (lib/BitbangSdCard), a small
// bit-banged driver. SdFat's own drivers could not write to the card on this board.
#include <Arduino.h>
#include <BitbangSdCard.h>
#include <SdFat.h>

static const uint8_t SD_SCK = 5;
static const uint8_t SD_MOSI = 18;
static const uint8_t SD_MISO = 19;
static const uint8_t SD_CS = 22;
static const uint8_t LED_PIN = 25;

static BitbangSdCard card(SD_SCK, SD_MOSI, SD_MISO, SD_CS);
static FsVolume sd;

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

  if (!card.begin() || !sd.begin(&card)) {
    Serial.printf("SD card mount failed (errorCode=%u errorData=0x%02X)\n", card.errorCode(), card.errorData());
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

  FsFile f = sd.open("log.txt", O_WRONLY | O_CREAT | O_APPEND);
  if (f) {
    f.println("Time: ");
    f.println(time);
    f.println("Temp: ");
    f.println(temperature);
    f.println(" ");
    f.close();
    Serial.println("appended to log.txt");
  } else {
    Serial.printf("failed to open log.txt (errorCode=%u)\n", card.errorCode());
  }

  blink_twice();
  delay(4700);
}
