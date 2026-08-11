#pragma once
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_log.h>
#include <cstdint>
#include <Print.h>
#include <vector>

/**
 * @brief In-memory Print sink that appends to a std::vector<uint8_t>.
 * Used by display modules and MCP tools for buffered PNG/base64 encoding.
 */
#include <esp_heap_caps.h>

struct BufferStream : public Print {
    struct VectorCompat {
        uint8_t* _data = nullptr;
        size_t _size = 0;
        size_t _capacity = 0;

        ~VectorCompat() {
            if (_data) heap_caps_free(_data);
        }

        const uint8_t* data() const { return _data; }
        size_t size() const { return _size; }
        bool empty() const { return _size == 0; }

        void push_back(uint8_t c) {
            insert(c);
        }

        void insert(uint8_t c) {
            if (_size + 1 > _capacity) grow(_size + 1);
            if (_data && _size < _capacity) _data[_size++] = c;
        }

        void insert(const uint8_t* start, const uint8_t* end) {
            size_t len = end - start;
            if (_size + len > _capacity) grow(_size + len);
            if (_data && _size < _capacity) {
                size_t available = _capacity - _size;
                size_t copyLen = len < available ? len : available;
                memcpy(_data + _size, start, copyLen);
                _size += copyLen;
            }
        }

    private:
        void grow(size_t minCap) {
            ESP_LOGI("BufferStream", "grow called with minCap=%u, current _capacity=%u", minCap, _capacity);
            size_t newCap = _capacity == 0 ? 16384 : _capacity * 2;
            if (newCap < minCap) newCap = minCap;
            
            ESP_LOGI("BufferStream", "Allocating newBuf %u bytes in PSRAM", newCap);
            uint8_t* newBuf = (uint8_t*)heap_caps_malloc(newCap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!newBuf) {
                ESP_LOGI("BufferStream", "PSRAM failed, trying DRAM");
                newBuf = (uint8_t*)heap_caps_malloc(newCap, MALLOC_CAP_8BIT);
            }
            
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
    } buffer;

    size_t write(uint8_t c) override { buffer.push_back(c); return 1; }
    size_t write(const uint8_t* buf, size_t size) override {
        buffer.insert(buf, buf + size);
        return size;
    }
};
