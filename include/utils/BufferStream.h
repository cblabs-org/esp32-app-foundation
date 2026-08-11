#pragma once
#include <Print.h>
#include <vector>
#include <cstdint>

/**
 * @brief In-memory Print sink that appends to a std::vector<uint8_t>.
 * Used by display modules and MCP tools for buffered PNG/base64 encoding.
 */
struct BufferStream : public Print {
    std::vector<uint8_t> buffer;
    size_t write(uint8_t c) override { buffer.push_back(c); return 1; }
    size_t write(const uint8_t* buf, size_t size) override {
        buffer.insert(buffer.end(), buf, buf + size);
        return size;
    }
};
