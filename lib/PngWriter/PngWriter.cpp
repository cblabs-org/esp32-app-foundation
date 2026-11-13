/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/PngWriter/PngWriter.cpp
 * Description: Implementation of Print class.
 *
 * Exported Classes:
 * - Print
 * - DeflateState
 */

#include "PngWriter.h"
#include <esp_heap_caps.h>   // heap_caps_malloc/calloc/free + MALLOC_CAP_* (native stub provides these)
#ifdef ARDUINO
#include <Arduino.h>
#endif
#ifdef ARDUINO
#include <Print.h>
#else
#include <cstdint>
#include <cstring>
class Print {
public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size) {
        size_t n = 0;
        while (size--) {
            n += write(*buffer++);
        }
        return n;
    }
    size_t print(const char* s) {
        return write((const uint8_t*)s, strlen(s));
    }
};
#endif
#if defined(ESP_PLATFORM)
#if __has_include(<miniz.h>)
#include <miniz.h>
#elif __has_include(<esp32/rom/miniz.h>)
#include <esp32/rom/miniz.h>
#elif __has_include(<rom/miniz.h>)
#include <rom/miniz.h>
#elif __has_include(<esp_rom/rom/miniz.h>)
#include <esp_rom/rom/miniz.h>
#endif
#endif
#include <esp_log.h>

namespace PngWriter {

// --- Checksums ---

static uint32_t crc32_le(uint32_t crc, const uint8_t *buf, size_t len) {
    crc = ~crc;
    for (size_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
    }
    return ~crc;
}

static uint32_t adler32(uint32_t adler, const uint8_t *buf, size_t len) {
    uint32_t s1 = adler & 0xffff;
    uint32_t s2 = (adler >> 16) & 0xffff;
    for (size_t i = 0; i < len; i++) {
        s1 = (s1 + buf[i]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    return (s2 << 16) + s1;
}

// --- Write Helpers ---

static void write32(Print& sink, uint32_t v) {
    sink.write((v >> 24) & 0xFF);
    sink.write((v >> 16) & 0xFF);
    sink.write((v >> 8) & 0xFF);
    sink.write(v & 0xFF);
}

static void writeChunk(Print& sink, const char* type, const uint8_t* data, uint32_t len) {
    write32(sink, len);
    uint32_t crc = crc32_le(0, (const uint8_t*)type, 4);
    if (len > 0 && data != nullptr) {
        crc = crc32_le(crc, data, len);
    }
    sink.print(type);
    if (len > 0 && data != nullptr) {
        sink.write(data, len);
    }
    write32(sink, crc);
}

// --- PNG Structures ---

static const uint8_t PNG_SIGNATURE[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };

static bool writeIHDR(Print& sink, uint32_t width, uint32_t height, uint8_t bitDepth, uint8_t colorType) {
    uint8_t ihdr[13];
    ihdr[0] = (width >> 24) & 0xFF; ihdr[1] = (width >> 16) & 0xFF; ihdr[2] = (width >> 8) & 0xFF; ihdr[3] = width & 0xFF;
    ihdr[4] = (height >> 24) & 0xFF; ihdr[5] = (height >> 16) & 0xFF; ihdr[6] = (height >> 8) & 0xFF; ihdr[7] = height & 0xFF;
    ihdr[8] = bitDepth;
    ihdr[9] = colorType;
    ihdr[10] = 0; // Compression (Deflate)
    ihdr[11] = 0; // Filter
    ihdr[12] = 0; // Interlace
    writeChunk(sink, "IHDR", ihdr, 13);
    return true;
}

// --- Implementations ---

bool writeGrayscale1bpp(Print& sink, uint16_t width, uint16_t height, const uint8_t* buffer) {
    sink.write(PNG_SIGNATURE, sizeof(PNG_SIGNATURE));
    writeIHDR(sink, width, height, 1, 0); 
    
    // Note: Stored block size cannot exceed 65535 bytes. 
    // This implementation writes a single stored block, which is sufficient for displays up to ~256x128.
    // Larger monochrome displays would require splitting into multiple stored blocks.
    uint16_t rowBytes = (width + 7) / 8;
    uint32_t dataLen = (rowBytes + 1) * height;
    if (dataLen > 65535) {
        return false; // Stored DEFLATE block exceeds maximum size
    }
    uint32_t idatLen = 2 + 5 + dataLen + 4;
    write32(sink, idatLen);
    
    uint32_t chunkCrc = crc32_le(0, (const uint8_t*)"IDAT", 4);
    sink.print("IDAT");
    
    uint8_t zlibHeader[2] = { 0x78, 0x01 };
    sink.write(zlibHeader, 2);
    chunkCrc = crc32_le(chunkCrc, zlibHeader, 2);
    
    uint8_t blockHeader[5];
    blockHeader[0] = 0x01 | 0x00; 
    blockHeader[1] = dataLen & 0xFF;
    blockHeader[2] = (dataLen >> 8) & 0xFF;
    blockHeader[3] = (~dataLen) & 0xFF;
    blockHeader[4] = ((~dataLen) >> 8) & 0xFF;
    sink.write(blockHeader, 5);
    chunkCrc = crc32_le(chunkCrc, blockHeader, 5);
    
    uint32_t adler = 1; 
    uint8_t filterByte = 0;
    
    for (uint16_t y = 0; y < height; ++y) {
        sink.write(filterByte);
        chunkCrc = crc32_le(chunkCrc, &filterByte, 1);
        adler = adler32(adler, &filterByte, 1);
        
        const uint8_t* rowData = buffer + y * rowBytes;
        sink.write(rowData, rowBytes);
        chunkCrc = crc32_le(chunkCrc, rowData, rowBytes);
        adler = adler32(adler, rowData, rowBytes);
    }
    
    uint8_t adlerBytes[4];
    adlerBytes[0] = (adler >> 24) & 0xFF;
    adlerBytes[1] = (adler >> 16) & 0xFF;
    adlerBytes[2] = (adler >> 8) & 0xFF;
    adlerBytes[3] = adler & 0xFF;
    sink.write(adlerBytes, 4);
    chunkCrc = crc32_le(chunkCrc, adlerBytes, 4);
    
    write32(sink, chunkCrc);
    writeChunk(sink, "IEND", nullptr, 0);
    return true;
}

bool writeGrayscale1bppFromTiles(Print& sink, uint16_t width, uint16_t height, const uint8_t* tileBuffer) {
    sink.write(PNG_SIGNATURE, sizeof(PNG_SIGNATURE));
    writeIHDR(sink, width, height, 1, 0); 
    
    uint16_t rowBytes = (width + 7) / 8;
    uint32_t dataLen = (rowBytes + 1) * height;
    if (dataLen > 65535) {
        return false; // Stored DEFLATE block exceeds maximum size
    }
    
    uint32_t idatLen = 2 + 5 + dataLen + 4;
    write32(sink, idatLen);
    
    uint32_t chunkCrc = crc32_le(0, (const uint8_t*)"IDAT", 4);
    sink.print("IDAT");
    
    uint8_t zlibHeader[2] = { 0x78, 0x01 };
    sink.write(zlibHeader, 2);
    chunkCrc = crc32_le(chunkCrc, zlibHeader, 2);
    
    uint8_t blockHeader[5];
    blockHeader[0] = 0x01 | 0x00; 
    blockHeader[1] = dataLen & 0xFF;
    blockHeader[2] = (dataLen >> 8) & 0xFF;
    blockHeader[3] = (~dataLen) & 0xFF;
    blockHeader[4] = ((~dataLen) >> 8) & 0xFF;
    sink.write(blockHeader, 5);
    chunkCrc = crc32_le(chunkCrc, blockHeader, 5);
    
    uint32_t adler = 1; 
    uint8_t filterByte = 0;
    
    for (uint16_t y = 0; y < height; ++y) {
        sink.write(filterByte);
        chunkCrc = crc32_le(chunkCrc, &filterByte, 1);
        adler = adler32(adler, &filterByte, 1);
        
        uint16_t page = y / 8;
        uint8_t bit = y % 8;
        
        uint8_t rowByte = 0;
        for (uint16_t x = 0; x < width; ++x) {
            uint8_t tileByte = tileBuffer[page * width + x];
            bool pixel = (tileByte >> bit) & 1;
            
            rowByte |= (pixel ? 1 : 0) << (7 - (x % 8));
            
            if (x % 8 == 7 || x == width - 1) {
                sink.write(rowByte);
                chunkCrc = crc32_le(chunkCrc, &rowByte, 1);
                adler = adler32(adler, &rowByte, 1);
                rowByte = 0;
            }
        }
    }
    
    uint8_t adlerBytes[4];
    adlerBytes[0] = (adler >> 24) & 0xFF;
    adlerBytes[1] = (adler >> 16) & 0xFF;
    adlerBytes[2] = (adler >> 8) & 0xFF;
    adlerBytes[3] = adler & 0xFF;
    sink.write(adlerBytes, 4);
    chunkCrc = crc32_le(chunkCrc, adlerBytes, 4);
    
    write32(sink, chunkCrc);
    writeChunk(sink, "IEND", nullptr, 0);
    return true;
}

#if defined(ESP_PLATFORM)
struct DeflateState {
    Print* sink;
    uint8_t* outBuf;
    uint32_t outLen;
    uint32_t chunkCrc;
    
    void flush() {
        if (outLen == 0) return;
        write32(*sink, outLen);
        chunkCrc = crc32_le(0, (const uint8_t*)"IDAT", 4);
        chunkCrc = crc32_le(chunkCrc, outBuf, outLen);
        sink->print("IDAT");
        sink->write(outBuf, outLen);
        write32(*sink, chunkCrc);
        outLen = 0;
    }
};

static int tdefl_put_buf_cb(const void* pBuf, int len, void* pUser) {
    DeflateState* s = (DeflateState*)pUser;
    const uint8_t* src = (const uint8_t*)pBuf;
    while (len > 0) {
        uint32_t space = 16384 - s->outLen;
        uint32_t copy = len < space ? len : space;
        memcpy(s->outBuf + s->outLen, src, copy);
        s->outLen += copy;
        src += copy;
        len -= copy;
        if (s->outLen == 16384) {
            s->flush();
        }
    }
    return 1;
}
#endif

bool writeRGB565Stored(Print& sink, uint16_t width, uint16_t height, const uint16_t* buffer) {
    sink.write(PNG_SIGNATURE, sizeof(PNG_SIGNATURE));
    writeIHDR(sink, width, height, 8, 2); 
    
    uint16_t fallbackRowBytes = width * 3;
    uint32_t totalBytes = (fallbackRowBytes + 1) * height;
    
    uint32_t totalBlocks = (totalBytes + 65535 - 1) / 65535;
    uint32_t idatLen = 2 + (totalBlocks * 5) + totalBytes + 4;
    
    write32(sink, idatLen);
    uint32_t chunkCrc = crc32_le(0, (const uint8_t*)"IDAT", 4);
    sink.print("IDAT");
    
    uint8_t zlibHeader[2] = { 0x78, 0x01 };
    sink.write(zlibHeader, 2);
    chunkCrc = crc32_le(chunkCrc, zlibHeader, 2);
    
    uint32_t fallbackAdler = 1;
    uint32_t offset = 0;
    
    while (offset < totalBytes) {
        uint32_t bLen = (totalBytes - offset) > 65535 ? 65535 : (totalBytes - offset);
        uint8_t bFinal = (offset + bLen == totalBytes) ? 1 : 0;
        
        uint8_t blockHeader[5];
        blockHeader[0] = bFinal | 0x00;
        blockHeader[1] = bLen & 0xFF;
        blockHeader[2] = (bLen >> 8) & 0xFF;
        blockHeader[3] = (~bLen) & 0xFF;
        blockHeader[4] = ((~bLen) >> 8) & 0xFF;
        sink.write(blockHeader, 5);
        chunkCrc = crc32_le(chunkCrc, blockHeader, 5);
        
        // For performance, generate row chunks instead of byte-by-byte
        uint8_t* chunkBuf = (uint8_t*)heap_caps_malloc(bLen, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!chunkBuf) return false;
        
        for (uint32_t i = 0; i < bLen; i++) {
            uint32_t pixelOffset = offset + i;
            uint32_t rowIdx = pixelOffset / (fallbackRowBytes + 1);
            uint32_t byteInRow = pixelOffset % (fallbackRowBytes + 1);
            
            if (byteInRow == 0) {
                chunkBuf[i] = 0; 
            } else {
                uint32_t x = (byteInRow - 1) / 3;
                uint32_t c = (byteInRow - 1) % 3;
                uint16_t p = buffer[rowIdx * width + x];
                if (c == 0) {
                    uint8_t r5 = (p >> 11) & 0x1F;
                    chunkBuf[i] = (r5 << 3) | (r5 >> 2);
                } else if (c == 1) {
                    uint8_t g6 = (p >> 5) & 0x3F;
                    chunkBuf[i] = (g6 << 2) | (g6 >> 4);
                } else {
                    uint8_t b5 = p & 0x1F;
                    chunkBuf[i] = (b5 << 3) | (b5 >> 2);
                }
            }
        }
        
        sink.write(chunkBuf, bLen);
        chunkCrc = crc32_le(chunkCrc, chunkBuf, bLen);
        fallbackAdler = adler32(fallbackAdler, chunkBuf, bLen);
        heap_caps_free(chunkBuf);
        
        offset += bLen;
    }
    
    uint8_t adlerBytes[4];
    adlerBytes[0] = (fallbackAdler >> 24) & 0xFF;
    adlerBytes[1] = (fallbackAdler >> 16) & 0xFF;
    adlerBytes[2] = (fallbackAdler >> 8) & 0xFF;
    adlerBytes[3] = fallbackAdler & 0xFF;
    sink.write(adlerBytes, 4);
    chunkCrc = crc32_le(chunkCrc, adlerBytes, 4);
    
    write32(sink, chunkCrc);
    writeChunk(sink, "IEND", nullptr, 0);
    return true;
}

bool writeRGB565ToRGB888(Print& sink, uint16_t width, uint16_t height, const uint16_t* buffer) {
#if defined(ESP_PLATFORM)
    // The miniz compressor state is 167KB, so we allocate it securely in PSRAM.
    tdefl_compressor* comp = (tdefl_compressor*)heap_caps_calloc(1, sizeof(tdefl_compressor), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    
    DeflateState state;
    state.sink = &sink;
    state.outBuf = (uint8_t*)heap_caps_malloc(16384, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    state.outLen = 0;
    
    uint32_t adler = 1;
    uint16_t rowBytes = width * 3;
    uint8_t* rowBuf = (uint8_t*)heap_caps_malloc(rowBytes + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    
    ESP_LOGI("PngWriter", "Allocated buffers. Initializing deflate...");
    
    bool useDeflate = false;
    if (comp && state.outBuf && rowBuf) {
        tdefl_status init_status = tdefl_init(comp, tdefl_put_buf_cb, &state, 1); // 1 max probe for PSRAM speed
        ESP_LOGI("PngWriter", "tdefl_init status: %d", (int)init_status);
        if (init_status == TDEFL_STATUS_OKAY) {
            // Pass zlib header into the compressor stream manually
            uint8_t zlibHeader[2] = { 0x78, 0x9C }; 
            tdefl_put_buf_cb(zlibHeader, 2, &state);
            
            // Try to compress the first row to ensure ROM tdefl works
            rowBuf[0] = 0; 
            adler = adler32(adler, rowBuf, 1);
            
            const uint16_t* srcRow = buffer;
            uint8_t* dst = rowBuf + 1;
            for (uint16_t x = 0; x < width; ++x) {
                uint16_t p = srcRow[x];
                uint8_t r5 = (p >> 11) & 0x1F;
                uint8_t g6 = (p >> 5) & 0x3F;
                uint8_t b5 = p & 0x1F;
                dst[0] = (r5 << 3) | (r5 >> 2);
                dst[1] = (g6 << 2) | (g6 >> 4);
                dst[2] = (b5 << 3) | (b5 >> 2);
                
                adler = adler32(adler, dst, 3);
                dst += 3;
            }
            
            tdefl_status status = tdefl_compress_buffer(comp, rowBuf, rowBytes + 1, (height == 1) ? TDEFL_FINISH : TDEFL_NO_FLUSH);
            ESP_LOGI("PngWriter", "tdefl_compress_buffer first row status: %d", (int)status);
            if (status == TDEFL_STATUS_OKAY || status == TDEFL_STATUS_DONE) {
                useDeflate = true;
            }
        }
    }
    
    ESP_LOGI("PngWriter", "useDeflate: %d", useDeflate);
    
    if (!useDeflate) {
        if (rowBuf) heap_caps_free(rowBuf);
        if (state.outBuf) heap_caps_free(state.outBuf);
        if (comp) heap_caps_free(comp);
        
        // Fall back to stored-PNG.
        return writeRGB565Stored(sink, width, height, buffer);
    }
    
    // We are proceeding with deflate. Write headers first.
    sink.write(PNG_SIGNATURE, sizeof(PNG_SIGNATURE));
    writeIHDR(sink, width, height, 8, 2); 
    
    ESP_LOGI("PngWriter", "Starting deflate loop for %d rows...", height - 1);
    
    for (uint16_t y = 1; y < height; ++y) { // Start from row 1 since row 0 is already processed
        rowBuf[0] = 0; 
        adler = adler32(adler, rowBuf, 1);
        
        const uint16_t* srcRow = buffer + y * width;
        uint8_t* dst = rowBuf + 1;
        for (uint16_t x = 0; x < width; ++x) {
            uint16_t p = srcRow[x];
            uint8_t r5 = (p >> 11) & 0x1F;
            uint8_t g6 = (p >> 5) & 0x3F;
            uint8_t b5 = p & 0x1F;
            dst[0] = (r5 << 3) | (r5 >> 2);
            dst[1] = (g6 << 2) | (g6 >> 4);
            dst[2] = (b5 << 3) | (b5 >> 2);
            
            adler = adler32(adler, dst, 3);
            dst += 3;
        }
        
        tdefl_compress_buffer(comp, rowBuf, rowBytes + 1, (y == height - 1) ? TDEFL_FINISH : TDEFL_NO_FLUSH);
        if (y % 50 == 0) ESP_LOGI("PngWriter", "Deflated row %d", y);
    }
    
    ESP_LOGI("PngWriter", "Deflate loop finished. Writing adler32 and IEND...");
    
    uint8_t adlerBytes[4];
    adlerBytes[0] = (adler >> 24) & 0xFF;
    adlerBytes[1] = (adler >> 16) & 0xFF;
    adlerBytes[2] = (adler >> 8) & 0xFF;
    adlerBytes[3] = adler & 0xFF;
    tdefl_put_buf_cb(adlerBytes, 4, &state);
    
    state.flush();
    
    writeChunk(sink, "IEND", nullptr, 0);
    
    heap_caps_free(rowBuf);
    heap_caps_free(state.outBuf);
    heap_caps_free(comp);
    
    ESP_LOGI("PngWriter", "PNG write complete!");
    return true;
#else
    // Fall back to uncompressed stored-PNG on host builds
    return writeRGB565Stored(sink, width, height, buffer);
#endif
}

} // namespace PngWriter
