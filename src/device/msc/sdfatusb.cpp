#include "sdfatusb.h"

#ifdef CFG_TUD_MSC

SDFat2USB::SDFat2USB() {
  MSCusb::setCallbacks( this );
}


// MSCusb

bool SDFat2USB::begin(char* str) {
  assert(sdBlockCount);
  assert(sdBlockSize);
  return MSCusb::begin(str);
}

bool SDFat2USB::initSD(SdSpiConfig ssConfig) {
  if( !sdFat.begin( ssConfig ) ) {
    log_e("Card Mount Failed. Check if card is inserted and proper SS pin configured");
    return false;
  }
  sdSSPin      = ssConfig.csPin;
  sdOptions    = ssConfig.options;
  sdSck        = ssConfig.maxSck;
  sdBlockCount = sdFat.card()->sectorCount();
  sdBlockSize  = sdFat.bytesPerSector();
  return true;
}

bool SDFat2USB::initSD(SdCsPin_t ssPin, uint32_t maxSck) {
  return initSD( SdSpiConfig(ssPin, DEDICATED_SPI, maxSck) );
}

bool SDFat2USB::initSD(SdCsPin_t ssPin) {
  return initSD(ssPin, SPI_FULL_SPEED);
}

void SDFat2USB::setCapacity(uint32_t count, uint32_t size) {
  sdBlockCount = count;
  sdBlockSize  = size;
}

void SDFat2USB::setCallbacks(MSCCallbacks* cb) {
  cb->setDefaultCallbacks(this);
  MSCusb::setCallbacks( cb );
}

void SDFat2USB::setReady(bool ready) {
  sdCardReady = ready;
}

bool SDFat2USB::isReady() {
  return sdCardReady;
}


// MSCCallbacks

void SDFat2USB::onInquiry(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
  const char vid[] = "ESP32-S2";
  const char pid[] = "SDFAT32";
  const char rev[] = "1.0";
  memcpy(vendor_id  , vid, strlen(vid));
  memcpy(product_id , pid, strlen(pid));
  memcpy(product_rev, rev, strlen(rev));
  log_v("default onInquiry");
}

bool SDFat2USB::onReady(uint8_t lun) {
  log_v("default onReady");
  return sdCardReady;
}

void SDFat2USB::onCapacity(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
  (void) lun;
  *block_count = sdBlockCount;
  *block_size  = sdBlockSize;
  log_v("default onCapacity: disk block count: %d, block size: %d", *block_count, *block_size);
}

bool SDFat2USB::onStop(uint8_t lun, uint8_t power_condition, bool start, bool load_eject, bool stop_start_sdfat = true) {
  (void) lun;
  (void) power_condition;
  if ( load_eject ) {
    if (start) { // load disk storage
      log_d("default start/stop load");
      if( stop_start_sdfat )
        return sdSSPin ? initSD( SdSpiConfig(sdSSPin, sdOptions, sdSck) ) : false;

    } else { // unload disk storage
      log_d("default start/stop unload");
      if( stop_start_sdfat )
        sdFat.end();
      setReady(false);
    }
  }
  return true;
}

bool SDFat2USB::onStop(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
  return onStop(lun, power_condition, start, load_eject, true);
}

int32_t SDFat2USB::onRead(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  log_v("default onRead");
  (void) lun;
  (void) offset;
  return isReady() ? ( sdFat.card()->readSectors(lba, (uint8_t*) buffer, bufsize / sdBlockSize) ? bufsize : -1 ) : 0;
}

int32_t SDFat2USB::onWrite(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  log_v("default onWrite");
  (void) lun;
  (void) offset;
  return isReady() ? ( sdFat.card()->writeSectors(lba, (uint8_t*) buffer, bufsize / sdBlockSize) ? bufsize : -1 ) : 0;
}

void SDFat2USB::onFlush(uint8_t lun) {
  log_v("default onFlush");
  (void) lun;
  sdFat.card()->syncDevice();
  sdFat.cacheClear();
}

#endif
