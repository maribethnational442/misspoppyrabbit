#include "StorageService.h"
#include <SPI.h>
#include <SD.h>
#include "Config.h"

StorageService storage;

void StorageService::begin() {
    _mutex = xSemaphoreCreateMutex();
    // El lector SD del Cardputer cuelga de un bus SPI propio (distinto del
    // de la pantalla), así que lo configuramos con sus pines explícitos.
    SPI.begin(config::SD_SCK, config::SD_MISO, config::SD_MOSI, config::SD_CS);
    _mounted = SD.begin(config::SD_CS, SPI, 25000000);

    if (_mounted) {
        if (!SD.exists(config::DATA_DIR)) SD.mkdir(config::DATA_DIR);
        log_i("microSD montada (%llu MB)", SD.cardSize() / (1024ULL * 1024ULL));
    } else {
        log_w("microSD no detectada: se trabaja solo en RAM");
    }
}

bool StorageService::atomicWrite(const char* path, std::function<bool(File&)> writer) {
    if (!_mounted) return false;
    SdLock lock;

    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    File f = SD.open(tmp, FILE_WRITE);
    if (!f) return false;
    const bool ok = writer(f);
    f.close();

    if (!ok) {
        SD.remove(tmp);
        return false;
    }
    SD.remove(path);              // FAT32 no permite rename sobre existente
    return SD.rename(tmp, path);
}
