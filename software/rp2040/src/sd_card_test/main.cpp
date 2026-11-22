// SD card read/write test: C++ port of the MicroPython sdtest.py by Peter Hinch
// (micropython-lib), adapted to this board's pins.
// Writes a large and a small file, reads them back and checks the contents.
// Results are printed to USB serial (115200). Creates rats.txt and rats1.txt on the card.
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

static BitbangSdCard card(SD_SCK, SD_MOSI, SD_MISO, SD_CS);
static FsVolume sd;

static const char LINE[] = "abcdefghijklmnopqrstuvwxyz\n";  // 27 chars
static const int LINE_REPEAT = 200;                         // 5400 chars
static const char SHORT_LINE[] = "1234567890\n";

static void write_lines(FsFile &f) {
  for (int i = 0; i < LINE_REPEAT; i++) f.print(LINE);
}

static bool read_expect(FsFile &f, const char *text) {
  for (size_t j = 0; j < strlen(text); j++) {
    if (f.read() != text[j]) return false;
  }
  return true;
}

// Checks that the file contains exactly: LINE x200, SHORT_LINE, LINE x200
static bool verify_large(const char *path) {
  FsFile f = sd.open(path, O_RDONLY);
  if (!f) return false;
  size_t expected = 2 * LINE_REPEAT * strlen(LINE) + strlen(SHORT_LINE);
  Serial.printf("%lu bytes read\n", (unsigned long)f.fileSize());
  bool ok = f.fileSize() == expected;
  for (int i = 0; ok && i < LINE_REPEAT; i++) ok = read_expect(f, LINE);
  ok = ok && read_expect(f, SHORT_LINE);
  for (int i = 0; ok && i < LINE_REPEAT; i++) ok = read_expect(f, LINE);
  f.close();
  return ok;
}

static bool verify_small(const char *path) {
  FsFile f = sd.open(path, O_RDONLY);
  if (!f) return false;
  Serial.printf("%lu bytes read\n", (unsigned long)f.fileSize());
  bool ok = f.fileSize() == strlen(SHORT_LINE) && read_expect(f, SHORT_LINE);
  f.close();
  return ok;
}

void setup() {
  Serial.begin(115200);
  delay(3000);  // time to open the serial monitor

  if (!card.begin() || !sd.begin(&card)) {
    Serial.printf("SD card mount failed (errorCode=%u errorData=0x%02X)\n", card.errorCode(), card.errorData());
    return;
  }
  Serial.printf("Filesystem check (%lu sectors)\n", (unsigned long)card.sectorCount());
  sd.ls(&Serial, LS_SIZE);

  Serial.println("\nMultiple block read/write");
  sd.remove("rats.txt");
  FsFile f = sd.open("rats.txt", O_WRONLY | O_CREAT | O_TRUNC);
  write_lines(f);
  f.print(SHORT_LINE);
  write_lines(f);
  Serial.printf("%lu bytes written (errorCode=%u)\n", (unsigned long)f.fileSize(), card.errorCode());
  f.close();
  bool large_ok = verify_large("rats.txt");

  Serial.println("\nSingle block read/write");
  sd.remove("rats1.txt");
  f = sd.open("rats1.txt", O_WRONLY | O_CREAT | O_TRUNC);
  f.print(SHORT_LINE);
  Serial.printf("%lu bytes written (errorCode=%u)\n", (unsigned long)f.fileSize(), card.errorCode());
  f.close();
  bool small_ok = verify_small("rats1.txt");

  Serial.println("\nVerifying data read back");
  Serial.println(large_ok ? "Large file Pass" : "Large file Fail");
  Serial.println(small_ok ? "Small file Pass" : "Small file Fail");
  Serial.printf("\nTests %s\n", (large_ok && small_ok) ? "passed" : "failed");
}

void loop() {}
