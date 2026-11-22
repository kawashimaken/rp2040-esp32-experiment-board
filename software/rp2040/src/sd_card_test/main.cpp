// SD card read/write test: C++ port of the MicroPython sdtest.py by Peter Hinch
// (micropython-lib), adapted to this board's pins.
// Writes a large and a small file, reads them back and checks the contents.
// Results are printed to USB serial (115200). Creates rats.txt and rats1.txt on the card.
//
// SD card (SPI mode): CLK=GP5, CMD(MOSI)=GP18, DAT0(MISO)=GP19, DAT3/CS=GP22
#include <Arduino.h>
#include <SD.h>
#include <SoftwareSPI.h>

static const uint8_t SD_SCK = 5;
static const uint8_t SD_MOSI = 18;
static const uint8_t SD_MISO = 19;
static const uint8_t SD_CS = 22;

static SoftwareSPI sdSpi(SD_SCK, SD_MISO, SD_MOSI);

static const char LINE[] = "abcdefghijklmnopqrstuvwxyz\n";  // 27 chars
static const int LINE_REPEAT = 200;                         // 5400 chars
static const char SHORT_LINE[] = "1234567890\n";

static void write_lines(File &f) {
  for (int i = 0; i < LINE_REPEAT; i++) f.print(LINE);
}

// Checks that the file contains exactly: LINE x200, SHORT_LINE, LINE x200
static bool verify_large(const char *path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  size_t expected = 2 * LINE_REPEAT * strlen(LINE) + strlen(SHORT_LINE);
  Serial.printf("%u bytes read\n", (unsigned)f.size());
  bool ok = f.size() == expected;
  for (int part = 0; ok && part < 3; part++) {
    int repeat = (part == 1) ? 1 : LINE_REPEAT;
    const char *text = (part == 1) ? SHORT_LINE : LINE;
    for (int i = 0; ok && i < repeat; i++) {
      for (size_t j = 0; ok && j < strlen(text); j++) ok = f.read() == text[j];
    }
  }
  f.close();
  return ok;
}

static bool verify_small(const char *path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  String s = f.readString();
  f.close();
  Serial.printf("%u bytes read\n", (unsigned)s.length());
  return s == SHORT_LINE;
}

static void list_root() {
  File root = SD.open("/");
  for (File e = root.openNextFile(); e; e = root.openNextFile()) {
    Serial.printf("  %s%s\n", e.name(), e.isDirectory() ? "/" : "");
    e.close();
  }
  root.close();
}

void setup() {
  Serial.begin(115200);
  delay(3000);  // time to open the serial monitor

  if (!SD.begin(SD_CS, sdSpi)) {
    Serial.println("SD card mount failed");
    return;
  }
  Serial.println("Filesystem check");
  list_root();

  Serial.println("\nMultiple block read/write");
  SD.remove("rats.txt");
  File f = SD.open("rats.txt", FILE_WRITE);
  write_lines(f);
  f.print(SHORT_LINE);
  write_lines(f);
  Serial.printf("%u bytes written\n", (unsigned)f.size());
  f.close();
  bool large_ok = verify_large("rats.txt");

  Serial.println("\nSingle block read/write");
  SD.remove("rats1.txt");
  f = SD.open("rats1.txt", FILE_WRITE);
  f.print(SHORT_LINE);
  Serial.printf("%u bytes written\n", (unsigned)f.size());
  f.close();
  bool small_ok = verify_small("rats1.txt");

  SD.end();

  Serial.println("\nVerifying data read back");
  Serial.println(large_ok ? "Large file Pass" : "Large file Fail");
  Serial.println(small_ok ? "Small file Pass" : "Small file Fail");
  Serial.printf("\nTests %s\n", (large_ok && small_ok) ? "passed" : "failed");
}

void loop() {}
