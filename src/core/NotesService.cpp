#include "NotesService.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Cardputer.h>
#include <Preferences.h>
#include <SD.h>
#include <cstring>
#include "Config.h"
#include "Melody.h"
#include "StorageService.h"

NotesService notesService;

namespace {
constexpr const char* NVS_RECLIM = "reclim";
const int LIMITS[] = {30, 60, 120, 300};

// nombre → id/tipo: "n1786975200.txt" | "n1786975200.wav"
bool parseName(const char* name, uint32_t& id, NotesService::Type& type) {
    if (name[0] != 'n') return false;
    char* end = nullptr;
    id = strtoul(name + 1, &end, 10);
    if (id == 0 || end == nullptr) return false;
    if (strcmp(end, ".txt") == 0) { type = NotesService::Type::Text; return true; }
    if (strcmp(end, ".wav") == 0) { type = NotesService::Type::Voice; return true; }
    return false;
}
}

void NotesService::begin() {
    _queue = xQueueCreate(2, sizeof(Cmd));   // 2×1.5KB: la web escribe poco a poco

    Preferences prefs;
    prefs.begin(config::NVS_NS, true);
    _recLimitS = prefs.getUShort(NVS_RECLIM, 60);
    prefs.end();

    if (!storage.mounted()) return;
    {
        SdLock lock;
        if (!SD.exists(config::NOTES_DIR)) SD.mkdir(config::NOTES_DIR);
        // Grabaciones interrumpidas por un corte de luz: fuera
        File dir = SD.open(config::NOTES_DIR);
        for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
            String name = f.name();
            f.close();
            if (name.endsWith(".tmp")) {
                SD.remove(String(config::NOTES_DIR) + "/" + name);
            }
        }
        dir.close();
    }
    rebuildIndex();
}

void NotesService::loop() {
    Cmd cmd;
    while (_queue != nullptr && xQueueReceive(_queue, &cmd, 0) == pdTRUE) {
        if (cmd.op == OP_SAVE_TEXT) saveText(cmd.id, cmd.text);
        else if (cmd.op == OP_REMOVE) removeNote(cmd.id);
    }
    if (_recActive) pumpRecording();
    if (_playActive) pumpPlayback();
}

// --- Índice -----------------------------------------------------------------

void NotesService::rebuildIndex() {
    _count = 0;
    if (!storage.mounted()) { bump(); return; }
    SdLock lock;

    File dir = SD.open(config::NOTES_DIR);
    for (File f = dir.openNextFile(); f && _count < MAX_NOTES; f = dir.openNextFile()) {
        uint32_t id;
        Type type;
        if (!parseName(f.name(), id, type)) { f.close(); continue; }
        Meta& m = _notes[_count++];
        m.id = id;
        m.type = type;
        m.size = f.size();
        m.preview[0] = '\0';
        if (type == Type::Text) {   // primera línea como preview
            const int n = f.read((uint8_t*)m.preview, sizeof(m.preview) - 1);
            m.preview[(n > 0) ? n : 0] = '\0';
            for (char* p = m.preview; *p; ++p) {
                if (*p == '\n' || *p == '\r') { *p = '\0'; break; }
            }
        }
        f.close();
    }
    dir.close();

    // Más recientes primero (inserción: son ≤100)
    for (int i = 1; i < _count; ++i) {
        const Meta key = _notes[i];
        int j = i - 1;
        while (j >= 0 && _notes[j].id < key.id) { _notes[j + 1] = _notes[j]; --j; }
        _notes[j + 1] = key;
    }
    bump();
}

int NotesService::durationS(const Meta& m) const {
    if (m.type != Type::Voice || m.size <= 44) return 0;
    return (int)((m.size - 44) / (SAMPLE_RATE * 2));
}

bool NotesService::pathFor(uint32_t id, char* out, size_t cap, Type* type) {
    for (int i = 0; i < _count; ++i) {
        if (_notes[i].id != id) continue;
        snprintf(out, cap, "%s/n%u.%s", config::NOTES_DIR, (unsigned)id,
                 _notes[i].type == Type::Text ? "txt" : "wav");
        if (type != nullptr) *type = _notes[i].type;
        return true;
    }
    return false;
}

// --- Texto ------------------------------------------------------------------

bool NotesService::saveText(uint32_t id, const char* content) {
    if (!storage.mounted() || content == nullptr || content[0] == '\0') return false;
    if (id == 0) {
        id = (uint32_t)time(nullptr);
        if (id < 1600000000) id = millis() / 1000 + 1;   // sin NTP: id no-epoch
    }
    char path[48];
    snprintf(path, sizeof(path), "%s/n%u.txt", config::NOTES_DIR, (unsigned)id);
    const size_t len = strnlen(content, MAX_TEXT);
    const bool ok = storage.atomicWrite(path, [&](File& f) {
        return f.write((const uint8_t*)content, len) == len;
    });
    if (ok) rebuildIndex();
    return ok;
}

bool NotesService::removeNote(uint32_t id) {
    if (playing() && _playId == id) playStop();
    char path[48];
    if (!pathFor(id, path, sizeof(path))) return false;
    bool ok;
    {
        SdLock lock;
        ok = SD.remove(path);
    }
    if (ok) rebuildIndex();
    return ok;
}

int NotesService::readText(uint32_t id, char* out, size_t cap) {
    char path[48];
    Type type;
    if (!pathFor(id, path, sizeof(path), &type) || type != Type::Text) return -1;
    SdLock lock;
    File f = SD.open(path, FILE_READ);
    if (!f) return -1;
    const int n = f.read((uint8_t*)out, cap - 1);
    f.close();
    out[(n > 0) ? n : 0] = '\0';
    return n;
}

// --- Cola web ---------------------------------------------------------------

bool NotesService::enqueueSaveText(uint32_t id, const char* content) {
    if (_queue == nullptr) return false;
    static Cmd cmd;   // 1.5KB: mejor estática que en el stack del async_tcp
    cmd.op = OP_SAVE_TEXT;
    cmd.id = id;
    strncpy(cmd.text, content, sizeof(cmd.text) - 1);
    cmd.text[sizeof(cmd.text) - 1] = '\0';
    return xQueueSend(_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE;
}

bool NotesService::enqueueRemove(uint32_t id) {
    if (_queue == nullptr) return false;
    static Cmd cmd;
    cmd.op = OP_REMOVE;
    cmd.id = id;
    cmd.text[0] = '\0';
    return xQueueSend(_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE;
}

void NotesService::indexJson(String& out) {
    out = "";
    out.reserve(64 + (size_t)_count * 80);
    out += "{\"notes\":[";
    JsonDocument item;
    String tmp;
    for (int i = 0; i < _count; ++i) {
        item.clear();
        item["id"]   = _notes[i].id;
        item["type"] = (_notes[i].type == Type::Text) ? "t" : "v";
        item["size"] = _notes[i].size;
        if (_notes[i].type == Type::Text) item["preview"] = _notes[i].preview;
        else item["dur"] = durationS(_notes[i]);
        tmp = "";
        serializeJson(item, tmp);
        if (i > 0) out += ',';
        out += tmp;
    }
    out += "]}";
}

// --- Grabación WAV ----------------------------------------------------------

bool NotesService::allocBuffers() {
    for (int i = 0; i < 2; ++i) {
        if (_buf[i] == nullptr) _buf[i] = (int16_t*)malloc(REC_CHUNK * sizeof(int16_t));
        if (_buf[i] == nullptr) { freeBuffers(); return false; }
    }
    return true;
}

void NotesService::freeBuffers() {
    for (int i = 0; i < 2; ++i) {
        free(_buf[i]);
        _buf[i] = nullptr;
    }
}

void NotesService::writeWavHeader(File& f, uint32_t dataBytes) {
    // Header WAV canónico: PCM mono 16-bit 16kHz
    const uint32_t byteRate = SAMPLE_RATE * 2;
    uint8_t h[44] = {'R','I','F','F', 0,0,0,0, 'W','A','V','E',
                     'f','m','t',' ', 16,0,0,0, 1,0, 1,0,
                     0,0,0,0, 0,0,0,0, 2,0, 16,0,
                     'd','a','t','a', 0,0,0,0};
    const uint32_t riff = dataBytes + 36;
    const uint32_t rate = SAMPLE_RATE;   // copia local: constexpr sin direccion
    memcpy(h + 4,  &riff, 4);
    memcpy(h + 24, &rate, 4);
    memcpy(h + 28, &byteRate, 4);
    memcpy(h + 40, &dataBytes, 4);
    f.write(h, sizeof(h));
}

bool NotesService::recStart() {
    if (_recActive || !storage.mounted() || !allocBuffers()) return false;
    if (_playActive) playStop();

    _recId = (uint32_t)time(nullptr);
    if (_recId < 1600000000) _recId = millis() / 1000 + 1;
    char tmp[48];
    snprintf(tmp, sizeof(tmp), "%s/n%u.wav.tmp", config::NOTES_DIR, (unsigned)_recId);
    {
        SdLock lock;
        _recFile = SD.open(tmp, FILE_WRITE);
        if (!_recFile) { freeBuffers(); return false; }
        writeWavHeader(_recFile, 0);   // placeholder: se corrige al cerrar
    }

    // El mic y el speaker comparten el I2S: silencio total mientras grabamos
    melodyPlayer.stop();
    M5Cardputer.Speaker.end();
    M5Cardputer.Mic.begin();

    _recBytes = 0;
    _recLevel = 0;
    _recStartMs = millis();
    _recActive = true;
    return true;
}

void NotesService::pumpRecording() {
    // record() llena el chunk (bloquea ~128ms): el loop respira entre chunks,
    // el audio jamás se acumula en RAM y la SD recibe 4KB por vez.
    if (M5Cardputer.Mic.record(_buf[0], REC_CHUNK, SAMPLE_RATE)) {
        int16_t peak = 0;
        for (int i = 0; i < REC_CHUNK; ++i) {
            const int16_t v = (_buf[0][i] < 0) ? -_buf[0][i] : _buf[0][i];
            if (v > peak) peak = v;
        }
        _recLevel = (uint8_t)((peak * 100) / 32768);
        SdLock lock;
        _recFile.write((const uint8_t*)_buf[0], REC_CHUNK * sizeof(int16_t));
        _recBytes += REC_CHUNK * sizeof(int16_t);
    }
    if (recMs() >= (uint32_t)_recLimitS * 1000) recStop(true);   // límite: se guarda
}

void NotesService::recStop(bool keep) {
    if (!_recActive) return;
    _recActive = false;
    M5Cardputer.Mic.end();
    M5Cardputer.Speaker.begin();

    char tmp[48], final_[48];
    snprintf(tmp, sizeof(tmp), "%s/n%u.wav.tmp", config::NOTES_DIR, (unsigned)_recId);
    snprintf(final_, sizeof(final_), "%s/n%u.wav", config::NOTES_DIR, (unsigned)_recId);
    {
        SdLock lock;
        _recFile.seek(0);
        writeWavHeader(_recFile, _recBytes);   // header definitivo
        _recFile.close();
        if (keep && _recBytes > 0) SD.rename(tmp, final_);
        else SD.remove(tmp);
    }
    freeBuffers();
    if (keep && _recBytes > 0) rebuildIndex();
}

uint32_t NotesService::recMs() const {
    return _recActive ? millis() - _recStartMs : 0;
}

void NotesService::adjustRecLimit(int dir) {
    const int n = sizeof(LIMITS) / sizeof(LIMITS[0]);
    int idx = 1;
    for (int i = 0; i < n; ++i) {
        if (LIMITS[i] == _recLimitS) idx = i;
    }
    idx = (idx + dir + n) % n;
    _recLimitS = LIMITS[idx];
    Preferences prefs;
    prefs.begin(config::NVS_NS, false);
    prefs.putUShort(NVS_RECLIM, (uint16_t)_recLimitS);
    prefs.end();
}

// --- Reproducción -----------------------------------------------------------

bool NotesService::playStart(uint32_t id) {
    if (_recActive) return false;
    if (_playActive) playStop();
    char path[48];
    Type type;
    if (!pathFor(id, path, sizeof(path), &type) || type != Type::Voice) return false;
    if (!allocBuffers()) return false;
    {
        SdLock lock;
        _playFile = SD.open(path, FILE_READ);
        if (!_playFile) { freeBuffers(); return false; }
        _playFile.seek(44);   // saltar el header WAV
    }
    melodyPlayer.stop();
    _playId = id;
    _playFlip = 0;
    _playActive = true;
    return true;
}

void NotesService::pumpPlayback() {
    // Un chunk por tick como máximo (128ms de audio por 33ms de tick: sobra).
    // playRaw REFERENCIA el buffer (no copia): el ping-pong garantiza que el
    // chunk que suena sigue vivo mientras se carga el siguiente.
    int n;
    {
        SdLock lock;
        n = _playFile.read((uint8_t*)_buf[_playFlip], REC_CHUNK * sizeof(int16_t));
    }
    if (n <= 0) {
        if (!M5Cardputer.Speaker.isPlaying(0)) playStop();   // terminó de sonar
        return;
    }
    if (M5Cardputer.Speaker.playRaw(_buf[_playFlip], n / 2, SAMPLE_RATE, false, 1, 0)) {
        _playFlip ^= 1;
    } else {
        // Cola del speaker llena: devolver el cursor y reintentar al siguiente tick
        SdLock lock;
        _playFile.seek(_playFile.position() - n);
    }
}

void NotesService::playStop() {
    if (!_playActive) return;
    _playActive = false;
    M5Cardputer.Speaker.stop(0);
    {
        SdLock lock;
        _playFile.close();
    }
    freeBuffers();
    _playId = 0;
    bump();   // la UI repinta el indicador ▶
}
