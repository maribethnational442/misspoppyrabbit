#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <FS.h>
#include <cstdint>

class String;

// ============================================================================
// NotesService — notas de texto y de voz en /mspos/notes/ de la microSD.
//
// A diferencia de TaskStore (datos pequeños residentes en RAM), las notas son
// BLOBS: en RAM solo vive un índice de metadatos (~4KB); el contenido se lee
// y escribe bajo demanda, siempre bajo SdLock.
//
// * Texto: n<epoch>.txt, máx 1.5KB por nota. CRUD desde dispositivo y web
//   (la web encola, como siempre: un solo escritor en el loop principal).
// * Voz: n<epoch>.wav — mono 16-bit 16kHz, streaming Mic→SD por chunks
//   (2×4KB ping-pong, alocados SOLO durante la grabación), header WAV
//   finalizado al cerrar y patrón tmp+rename: un corte de luz jamás deja
//   un WAV corrupto (los .tmp huérfanos se limpian al arrancar).
// * Mic y Speaker comparten I2S: son mutuamente exclusivos; la grabación
//   silencia melodías y la reproducción se hace por chunks desde loop().
// ============================================================================

class NotesService {
public:
    static constexpr int    MAX_NOTES  = 100;
    static constexpr size_t MAX_TEXT   = 1536;   // tope de una nota de texto
    static constexpr int    REC_CHUNK  = 2048;   // samples por chunk (128ms)
    static constexpr int    SAMPLE_RATE = 16000;

    enum class Type : uint8_t { Text, Voice };

    struct Meta {
        uint32_t id;         // epoch de creación (del nombre de archivo)
        uint32_t size;       // bytes en SD
        Type     type;
        char     preview[25];  // primera línea (texto) — vacío en voz
    };

    void begin();   // mkdir, limpiar .tmp huérfanos, construir índice
    void loop();    // comandos web + bombeo de grabación/reproducción

    // --- Índice (lectura desde el loop principal) ---
    const Meta* notes() const { return _notes; }
    int  count() const        { return _count; }
    uint32_t revision() const { return _rev; }
    int  durationS(const Meta& m) const;   // duración de una nota de voz

    // --- Texto (loop principal) ---
    bool saveText(uint32_t id, const char* content);   // id==0 crea
    bool removeNote(uint32_t id);
    int  readText(uint32_t id, char* out, size_t cap); // bytes leídos o -1

    // --- Para los handlers web (otra tarea) ---
    bool enqueueSaveText(uint32_t id, const char* content);
    bool enqueueRemove(uint32_t id);
    void indexJson(String& out);
    bool pathFor(uint32_t id, char* out, size_t cap, Type* type = nullptr);

    // --- Grabación ---
    bool recStart();
    void recStop(bool keep);
    bool recording() const  { return _recActive; }
    uint32_t recMs() const;
    uint8_t recLevel() const { return _recLevel; }   // VU 0-100
    int  recLimitS() const   { return _recLimitS; }
    void adjustRecLimit(int dir);   // cicla 30/60/120/300, persiste en NVS

    // --- Reproducción en el dispositivo ---
    bool playStart(uint32_t id);
    void playStop();
    bool playing() const     { return _playActive; }
    uint32_t playingId() const { return _playId; }

private:
    enum Op : uint8_t { OP_SAVE_TEXT, OP_REMOVE };
    struct Cmd {
        uint8_t  op;
        uint32_t id;
        char     text[MAX_TEXT];
    };

    void rebuildIndex();
    void bump() { _rev = _rev + 1; }
    bool allocBuffers();
    void freeBuffers();
    void writeWavHeader(File& f, uint32_t dataBytes);
    void pumpRecording();
    void pumpPlayback();

    Meta _notes[MAX_NOTES];
    int  _count = 0;
    volatile uint32_t _rev = 1;
    QueueHandle_t _queue = nullptr;

    // Buffers compartidos grabación/reproducción (heap, solo mientras se usan)
    int16_t* _buf[2] = {nullptr, nullptr};

    // Grabación
    bool     _recActive = false;
    File     _recFile;
    uint32_t _recStartMs = 0;
    uint32_t _recBytes = 0;
    uint32_t _recId = 0;
    uint8_t  _recLevel = 0;
    int      _recLimitS = 60;

    // Reproducción
    bool     _playActive = false;
    File     _playFile;
    uint32_t _playId = 0;
    uint8_t  _playFlip = 0;
};

extern NotesService notesService;
