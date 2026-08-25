/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/utils/BufferStream.h
 * Description: In-memory Print sink that accumulates byte output into a
 *              contiguous PSRAM-backed buffer. Used by display modules
 *              (U8G2DisplayModule, LVGLDisplayModule) and MCP tools for
 *              buffered PNG rendering and base64 encoding.
 *
 *              Internally wraps a VectorCompat struct — a minimal
 *              std::vector<uint8_t> replacement that allocates via
 *              heap_caps_malloc with a PSRAM-first / DRAM-fallback
 *              strategy. This avoids the default allocator, which would
 *              place large image buffers in scarce internal DRAM.
 *
 *              Growth policy: starts at 16 KiB, doubles on each
 *              reallocation. Buffers are freed on destruction.
 *
 * Exported Functions/Classes:
 * - BufferStream: Arduino Print sink that writes to a PSRAM-backed buffer
 *   - write():   Append a single byte or a byte range to the buffer  [Print]
 *   - buffer:    VectorCompat member holding the accumulated output
 *
 * - VectorCompat: Minimal contiguous byte buffer with PSRAM-first allocation
 *   - data():      Return pointer to the raw byte array
 *   - size():      Return the number of bytes written
 *   - empty():     Return true if no bytes have been written
 *   - push_back(): Append a single byte (delegates to insert)
 *   - insert():    Append a single byte or a [start, end) byte range
 */

#pragma once

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <cstdint>
#include <Print.h>

/**
 * @brief In-memory Print sink that appends to a PSRAM-backed byte buffer.
 *
 * Implements the Arduino Print interface so any API that accepts a Print&
 * (e.g. PNGenc) can write directly into this buffer. The accumulated bytes
 * are accessible via the public `buffer` member.
 *
 * @note Allocation favours PSRAM (MALLOC_CAP_SPIRAM) and falls back to
 *       internal DRAM if PSRAM is unavailable or exhausted.
 */
struct BufferStream : public Print {

    /**
     * @brief Minimal contiguous byte buffer with PSRAM-first allocation.
     *
     * Replaces std::vector<uint8_t> to give explicit control over the
     * heap region used for large image/binary buffers. Provides a
     * subset of the std::vector API used by BufferStream.
     */
    struct VectorCompat {
        uint8_t* _data = nullptr;     // Pointer to the heap-allocated byte array
        size_t _size = 0;             // Number of valid bytes written
        size_t _capacity = 0;         // Total allocated capacity in bytes

        /** @brief Frees the heap-allocated buffer. */
        ~VectorCompat() {
            if (_data) heap_caps_free(_data);
        }

        /** @brief Returns a const pointer to the raw byte array. */
        const uint8_t* data() const { return _data; }

        /** @brief Returns the number of bytes currently stored. */
        size_t size() const { return _size; }

        /** @brief Returns true if no bytes have been written. */
        bool empty() const { return _size == 0; }

        /**
         * @brief Appends a single byte to the buffer.
         * @param c  Byte to append.
         */
        void push_back(uint8_t c) {
            insert(c);
        }

        /**
         * @brief Appends a single byte, growing the buffer if necessary.
         * @param c  Byte to append.
         */
        void insert(uint8_t c) {
            if (_size + 1 > _capacity) grow(_size + 1);
            if (_data && _size < _capacity) _data[_size++] = c;
        }

        /**
         * @brief Appends a contiguous range of bytes [start, end).
         *
         * Grows the buffer if the current capacity is insufficient.
         * Copies up to the available capacity to prevent overrun.
         *
         * @param start  Pointer to the first byte to copy.
         * @param end    Pointer past the last byte to copy.
         */
        void insert(const uint8_t* start, const uint8_t* end) {
            size_t len = end - start;
            if (_size + len > _capacity) grow(_size + len);
            if (_data && _size < _capacity) {
                size_t available = _capacity - _size;
                size_t copyLen = len < available ? len : available;  // Clamp to remaining capacity
                memcpy(_data + _size, start, copyLen);
                _size += copyLen;
            }
        }

    private:
        /**
         * @brief Grows the internal buffer to at least minCap bytes.
         *
         * Uses a doubling strategy (initial size 16 KiB) to amortise
         * reallocation cost. Allocates from PSRAM first; falls back to
         * internal DRAM if PSRAM allocation fails.
         *
         * @param minCap  Minimum required capacity in bytes.
         */
        void grow(size_t minCap) {
            ESP_LOGI("BufferStream", "grow called with minCap=%u, current _capacity=%u", minCap, _capacity);

            // --- Doubling strategy: start at 16 KiB, double each time ---
            size_t newCap = _capacity == 0 ? 16384 : _capacity * 2;
            if (newCap < minCap) newCap = minCap;   // Ensure request is satisfied
            
            // --- PSRAM-first allocation with DRAM fallback ---
            ESP_LOGI("BufferStream", "Allocating newBuf %u bytes in PSRAM", newCap);
            uint8_t* newBuf = (uint8_t*)heap_caps_malloc(newCap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!newBuf) {
                ESP_LOGI("BufferStream", "PSRAM failed, trying DRAM");
                newBuf = (uint8_t*)heap_caps_malloc(newCap, MALLOC_CAP_8BIT);
            }
            
            // --- Copy existing data and free old buffer ---
            if (newBuf) {
                if (_data) {
                    ESP_LOGI("BufferStream", "Copying %u bytes to newBuf", _size);
                    memcpy(newBuf, _data, _size);
                    ESP_LOGI("BufferStream", "Freeing old _data");
                    heap_caps_free(_data);
                }
                _data = newBuf;
                _capacity = newCap;
            }
            ESP_LOGI("BufferStream", "grow finished");
        }
    } buffer;   // Public buffer holding the accumulated output

    /**
     * @brief Writes a single byte to the buffer.
     * @param c  Byte to write.
     * @return Always returns 1 (number of bytes written).
     */
    size_t write(uint8_t c) override { buffer.push_back(c); return 1; }

    /**
     * @brief Writes a block of bytes to the buffer.
     * @param buf   Pointer to the byte array to write.
     * @param size  Number of bytes to write.
     * @return The number of bytes written (always equals size).
     */
    size_t write(const uint8_t* buf, size_t size) override {
        buffer.insert(buf, buf + size);
        return size;
    }
};
