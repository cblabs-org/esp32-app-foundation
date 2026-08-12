#pragma once
#include <cstdint>
#include <cstddef>
#include "Arduino.h"  // For Print base class

class Stream : public Print {
public:
    virtual ~Stream() = default;
    virtual int available() { return 0; }
    virtual int read() { return -1; }
    virtual int peek() { return -1; }
    virtual void flush() {}

    // readBytes — used by ArduinoJson's ArduinoStreamReader
    virtual size_t readBytes(char* buffer, size_t length) {
        size_t count = 0;
        while (count < length) {
            int c = read();
            if (c < 0) break;
            *buffer++ = static_cast<char>(c);
            count++;
        }
        return count;
    }

    // Needed for write override (Print is abstract)
    size_t write(uint8_t c) override { (void)c; return 1; }
};
