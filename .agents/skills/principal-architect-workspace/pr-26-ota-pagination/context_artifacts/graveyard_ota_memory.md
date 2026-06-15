# Graveyard — OTA Memory Failures

## getString() on DRAM (KILLED)
- `http.getString()` allocates an Arduino `String` on DRAM heap
- For GitHub API responses (5-15 KB JSON), this consumed 5-15 KB DRAM
- Combined with WiFiClientSecure (~45 KB) and OTA task stack (16 KB), caused heap exhaustion
- Symptoms: `IncompleteInput` JSON parse errors, stream timeouts at 48%/81% download progress
- **Fix**: Manual `heap_caps_malloc(MALLOC_CAP_SPIRAM)` with DRAM fallback + `getStream().readBytes()`

## getStream() with ArduinoJson (KILLED)
- `deserializeJson(doc, http.getStream())` reads directly from the TLS stream
- ArduinoJson reads byte-by-byte, which is extremely slow over TLS
- TLS read timeout fires before the full JSON payload is parsed
- **Fix**: Read entire payload into PSRAM buffer first, then parse from buffer

## Stack-allocated buf[1024] in OTADownloadEngine (KILLED)
- `uint8_t buf[1024]` on the 16 KB task stack consumed 1 KB of headroom
- Combined with WiFiClientSecure + HTTPClient stack frames, left dangerously little margin
- **Fix**: `static uint8_t buf[kChunkSize]` — placed in BSS (DRAM), safe for SPI flash ops

## Class-member chunkBuf_[1024] approach (KILLED)
- Attempted to add `uint8_t chunkBuf_[kChunkSize]` as a class member of OTADownloadEngine
- OTADownloadEngine is instantiated as a stack local (`OTADownloadEngine engine;` at L700)
- This means the member array STILL lives on the stack → no improvement
- **Fix**: Use `static` local instead (BSS segment)
