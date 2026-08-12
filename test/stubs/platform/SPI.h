#pragma once
#include <cstdint>
#include <cstddef>

class SPISettings {
public:
    SPISettings() {}
    SPISettings(uint32_t, uint8_t, uint8_t) {}
};

class SPIClass {
public:
    void begin() {}
    void end() {}
    void beginTransaction(SPISettings) {}
    void endTransaction() {}
    uint8_t transfer(uint8_t b) { return 0; }
    void transfer(void*, size_t) {}
};

inline SPIClass SPI;
