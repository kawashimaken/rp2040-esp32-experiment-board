// Minimal SD card driver (SPI mode, bit-banged GPIO) for SdFat.
//
// SdFat's own drivers could mount the card on this board but failed to write,
// while a plain bit-banged sequence (like the MicroPython sdcard.py driver)
// works. This class implements SdFat's FsBlockDeviceInterface with that
// sequence, so SdFat's FAT code can be used on top of it.
//
// - Any GPIO pins (the SD pins on this board do not fit the hardware SPI roles)
// - Every command carries a correct CRC7 and every data block a correct CRC16,
//   so it works whether or not the card checks CRCs in SPI mode
// - Enables the internal pull-up on MISO (the board has none on DAT0)
//
// Needs build flag -DUSE_BLOCK_DEVICE_INTERFACE=1
#pragma once
#include <Arduino.h>
#include <SdFat.h>

class BitbangSdCard : public FsBlockDeviceInterface {
 public:
  BitbangSdCard(uint8_t sck, uint8_t mosi, uint8_t miso, uint8_t cs) : _sck(sck), _mosi(mosi), _miso(miso), _cs(cs) {}

  // Initializes the card. Returns false on failure (see errorCode()).
  bool begin();

  bool isBusy() override { return false; }
  bool readSector(Sector_t sector, uint8_t* dst) override;
  bool readSectors(Sector_t sector, uint8_t* dst, size_t ns) override;
  Sector_t sectorCount() override { return _sectors; }
  bool syncDevice() override { return true; }
  bool writeSector(Sector_t sector, const uint8_t* src) override;
  bool writeSectors(Sector_t sector, const uint8_t* src, size_t ns) override;

  // Step that failed last (0 = none), and the card's response at that point
  uint8_t errorCode() const { return _error; }
  uint8_t errorData() const { return _errorData; }

  enum : uint8_t { ERR_CMD0 = 1, ERR_ACMD41, ERR_CMD58, ERR_CMD9, ERR_READ, ERR_READ_TOKEN, ERR_READ_CRC,
                   ERR_WRITE, ERR_WRITE_RESPONSE, ERR_WRITE_BUSY, ERR_WRITE_STATUS };

 private:
  uint8_t xfer(uint8_t out);
  uint8_t command(uint8_t cmd, uint32_t arg);
  bool fail(uint8_t code, uint8_t data);
  bool waitNotBusy(uint32_t timeout_ms);
  bool readData(uint8_t* dst, size_t len);
  uint32_t address(Sector_t sector) const { return _blockAddressing ? sector : sector * 512; }
  void select(bool on) { digitalWrite(_cs, on ? LOW : HIGH); }

  uint8_t _sck, _mosi, _miso, _cs;
  uint32_t _halfPeriodUs = 5;  // ~100 kHz during init, faster afterwards
  bool _blockAddressing = false;
  Sector_t _sectors = 0;
  uint8_t _error = 0;
  uint8_t _errorData = 0;
};
