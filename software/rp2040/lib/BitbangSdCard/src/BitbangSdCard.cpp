#include "BitbangSdCard.h"

static uint8_t crc7(const uint8_t* data, int len) {
  uint8_t crc = 0;
  for (int i = 0; i < len; i++) {
    uint8_t d = data[i];
    for (int b = 0; b < 8; b++) {
      crc <<= 1;
      if ((d ^ crc) & 0x80) crc ^= 0x09;
      d <<= 1;
    }
  }
  return (crc << 1) | 1;  // CRC7 + end bit
}

static uint16_t crc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int b = 0; b < 8; b++) crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
  }
  return crc;
}

uint8_t BitbangSdCard::xfer(uint8_t out) {
  uint8_t in = 0;
  for (int i = 7; i >= 0; i--) {  // SPI mode 0, MSB first
    digitalWrite(_mosi, (out >> i) & 1);
    delayMicroseconds(_halfPeriodUs);
    digitalWrite(_sck, HIGH);
    in = (in << 1) | digitalRead(_miso);
    delayMicroseconds(_halfPeriodUs);
    digitalWrite(_sck, LOW);
  }
  return in;
}

uint8_t BitbangSdCard::command(uint8_t cmd, uint32_t arg) {
  uint8_t frame[5] = {(uint8_t)(0x40 | cmd), (uint8_t)(arg >> 24), (uint8_t)(arg >> 16), (uint8_t)(arg >> 8),
                      (uint8_t)arg};
  xfer(0xFF);
  for (uint8_t b : frame) xfer(b);
  xfer(crc7(frame, 5));
  uint8_t r1 = 0xFF;
  for (int i = 0; i < 10 && (r1 & 0x80); i++) r1 = xfer(0xFF);
  return r1;
}

bool BitbangSdCard::fail(uint8_t code, uint8_t data) {
  _error = code;
  _errorData = data;
  select(false);
  xfer(0xFF);
  return false;
}

bool BitbangSdCard::waitNotBusy(uint32_t timeout_ms) {
  uint32_t start = millis();
  while (xfer(0xFF) != 0xFF) {
    if (millis() - start > timeout_ms) return false;
  }
  return true;
}

bool BitbangSdCard::readData(uint8_t* dst, size_t len) {
  uint8_t token;
  uint32_t start = millis();
  while ((token = xfer(0xFF)) == 0xFF) {
    if (millis() - start > 500) return fail(ERR_READ_TOKEN, token);
  }
  if (token != 0xFE) return fail(ERR_READ_TOKEN, token);
  for (size_t i = 0; i < len; i++) dst[i] = xfer(0xFF);
  uint16_t got = (uint16_t)xfer(0xFF) << 8;
  got |= xfer(0xFF);
  if (got != crc16(dst, len)) return fail(ERR_READ_CRC, 0);
  return true;
}

bool BitbangSdCard::begin() {
  _error = 0;
  _halfPeriodUs = 5;
  pinMode(_sck, OUTPUT);
  pinMode(_mosi, OUTPUT);
  pinMode(_cs, OUTPUT);
  pinMode(_miso, INPUT_PULLUP);
  digitalWrite(_sck, LOW);
  digitalWrite(_mosi, HIGH);
  select(false);
  delay(10);
  for (int i = 0; i < 10; i++) xfer(0xFF);  // >= 74 clocks with CS high

  select(true);
  uint8_t r1 = command(0, 0);
  if (r1 != 0x01) return fail(ERR_CMD0, r1);
  r1 = command(8, 0x1AA);
  bool v2 = !(r1 & 0x04);
  for (int i = 0; i < 4; i++) xfer(0xFF);  // R7 payload (not needed)

  uint32_t start = millis();
  do {
    command(55, 0);
    r1 = command(41, v2 ? 0x40000000 : 0);
  } while (r1 == 0x01 && millis() - start < 2000);
  if (r1 != 0x00) return fail(ERR_ACMD41, r1);

  _blockAddressing = false;
  if (v2) {
    r1 = command(58, 0);
    uint8_t ocr0 = xfer(0xFF);
    for (int i = 0; i < 3; i++) xfer(0xFF);
    if (r1 != 0x00) return fail(ERR_CMD58, r1);
    _blockAddressing = ocr0 & 0x40;
  }
  if (!_blockAddressing) command(16, 512);  // byte-addressed cards: 512-byte blocks

  // Card size from the CSD register
  r1 = command(9, 0);
  uint8_t csd[16];
  if (r1 != 0x00) return fail(ERR_CMD9, r1);
  if (!readData(csd, 16)) return false;
  if ((csd[0] >> 6) == 1) {  // CSD v2 (SDHC/SDXC)
    uint32_t c_size = ((uint32_t)(csd[7] & 0x3F) << 16) | ((uint32_t)csd[8] << 8) | csd[9];
    _sectors = (c_size + 1) * 1024;
  } else {  // CSD v1 (SDSC)
    uint32_t c_size = ((uint32_t)(csd[6] & 0x03) << 10) | ((uint32_t)csd[7] << 2) | (csd[8] >> 6);
    uint8_t c_size_mult = ((csd[9] & 0x03) << 1) | (csd[10] >> 7);
    uint8_t read_bl_len = csd[5] & 0x0F;
    _sectors = (c_size + 1) << (c_size_mult + 2 + read_bl_len - 9);
  }
  select(false);
  xfer(0xFF);
  _halfPeriodUs = 1;  // faster after init
  return true;
}

bool BitbangSdCard::readSector(Sector_t sector, uint8_t* dst) {
  select(true);
  uint8_t r1 = command(17, address(sector));
  if (r1 != 0x00) return fail(ERR_READ, r1);
  if (!readData(dst, 512)) return false;
  select(false);
  xfer(0xFF);
  return true;
}

bool BitbangSdCard::readSectors(Sector_t sector, uint8_t* dst, size_t ns) {
  for (size_t i = 0; i < ns; i++) {
    if (!readSector(sector + i, dst + i * 512)) return false;
  }
  return true;
}

bool BitbangSdCard::writeSector(Sector_t sector, const uint8_t* src) {
  select(true);
  uint8_t r1 = command(24, address(sector));
  if (r1 != 0x00) return fail(ERR_WRITE, r1);
  xfer(0xFF);
  xfer(0xFE);  // start token
  for (int i = 0; i < 512; i++) xfer(src[i]);
  uint16_t crc = crc16(src, 512);
  xfer(crc >> 8);
  xfer(crc & 0xFF);
  uint8_t resp = xfer(0xFF);
  if ((resp & 0x1F) != 0x05) return fail(ERR_WRITE_RESPONSE, resp);
  if (!waitNotBusy(1000)) return fail(ERR_WRITE_BUSY, 0);
  // Check that programming succeeded
  r1 = command(13, 0);
  uint8_t r2 = xfer(0xFF);
  if (r1 != 0x00 || r2 != 0x00) return fail(ERR_WRITE_STATUS, r2);
  select(false);
  xfer(0xFF);
  return true;
}

bool BitbangSdCard::writeSectors(Sector_t sector, const uint8_t* src, size_t ns) {
  for (size_t i = 0; i < ns; i++) {
    if (!writeSector(sector + i, src + i * 512)) return false;
  }
  return true;
}
