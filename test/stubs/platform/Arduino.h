#include "esp_heap_caps.h"
#include "freertos/task.h"
#pragma once

// Prevent U8x8lib.h from requiring SPI.h / Wire.h on native builds.
// These are checked AFTER Arduino.h is included by U8x8lib.h.
#ifndef U8X8_NO_HW_SPI
#define U8X8_NO_HW_SPI
#endif
#ifndef U8X8_NO_HW_I2C
#define U8X8_NO_HW_I2C
#endif
#include <cstdint>
#include <string>
#include <cstring>

#ifndef PROGMEM
#define PROGMEM
#endif

// PROGMEM read stubs — on native, flash == RAM so reads are plain dereferences.
#ifndef pgm_read_byte
#define pgm_read_byte(addr)  (*(const uint8_t*)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr)  (*(const uint16_t*)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const uint32_t*)(addr))
#endif
#ifndef pgm_read_ptr
#define pgm_read_ptr(addr)   (*(const void* const*)(addr))
#endif
#ifndef strlen_P
#define strlen_P strlen
#endif
#ifndef strncpy_P
#define strncpy_P strncpy
#endif
#ifndef strcmp_P
#define strcmp_P strcmp
#endif
#ifndef memcpy_P
#define memcpy_P memcpy
#endif
#ifndef PSTR
#define PSTR(s) (s)
#endif
// Forward-declare __FlashStringHelper so the F() macro compiles here.
// The class is fully declared further down in this file.
class __FlashStringHelper;
#ifndef F
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper*>(PSTR(string_literal)))
#endif

// String class migrated from NativeBuildStubs.hpp
#ifndef STRING_STUB_DEFINED
#define STRING_STUB_DEFINED
class String {
public:
    bool endsWith(const char* suffix) const { return false; }
public:
    String() {}
    String(const char* s) : _str(s ? s : "") {}
    String(const std::string& s) : _str(s) {}
    String(int value, int base = 10) {
        if (base == 16) {
            char buf[20];
            snprintf(buf, sizeof(buf), "%x", value);
            _str = buf;
        } else {
            _str = std::to_string(value);
        }
    }
    const char* c_str() const { return _str.c_str(); }
    bool isEmpty() const { return _str.empty(); }
    int length() const { return static_cast<int>(_str.length()); }
    String& operator=(const char* s) { _str = s ? s : ""; return *this; }
    String operator+(const char* s) const { return String(_str + s); }
    String operator+(const String& s) const { return String(_str + s._str); }
    String& operator+=(char c) { _str += c; return *this; }
    String& operator+=(const String& s) { _str += s._str; return *this; }
    friend String operator+(const char* lhs, const String& rhs) { return String(lhs) + rhs; }
    bool operator==(const String& other) const { return _str == other._str; }
    bool operator==(const char* s) const { return _str == s; }
    char operator[](int i) const { return _str[i]; }
    operator std::string() const { return _str; }
    operator const char*() const { return _str.c_str(); }
    bool startsWith(const char* prefix) const { return _str.rfind(prefix, 0) == 0; }
    size_t write(uint8_t c) { _str += static_cast<char>(c); return 1; }
    size_t write(const uint8_t* buf, size_t size) { _str.append(reinterpret_cast<const char*>(buf), size); return size; }
    bool concat(const char* s) { if (s) _str += s; return true; }
    bool concat(char c) { _str += c; return true; }
    bool concat(const String& s) { _str += s._str; return true; }
    void reserve(unsigned int size) { _str.reserve(size); }
    int indexOf(char c, unsigned int fromIndex = 0) const {
        auto pos = _str.find(c, fromIndex);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }
    String substring(unsigned int beginIndex) const { return String(_str.substr(beginIndex)); }
    String substring(unsigned int beginIndex, unsigned int endIndex) const {
        return String(_str.substr(beginIndex, endIndex - beginIndex));
    }
    void remove(unsigned int index, unsigned int count = 1000000) {
        if (index < _str.length()) {
            _str.erase(index, count);
        }
    }
private:
    std::string _str;
};

template <typename T>
inline void convertToJson(const String& src, T dst) {
    dst.set(src.c_str());
}
#endif

inline void delay(uint32_t ms) { (void)ms; }
inline float temperatureRead() { return 42.0f; }
inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }

// GPIO stubs — no physical pins on native.
#ifndef INPUT
#define INPUT    0x0
#define OUTPUT   0x1
#define INPUT_PULLUP 0x2
#endif
#ifndef LOW
#define LOW  0x0
#define HIGH 0x1
#endif
inline void     pinMode(uint8_t, uint8_t) {}
inline void     digitalWrite(uint8_t, uint8_t) {}
inline int      digitalRead(uint8_t) { return LOW; }
inline int      analogRead(uint8_t) { return 0; }
inline void     analogWrite(uint8_t, int) {}
inline void     tone(uint8_t, unsigned int, unsigned long = 0) {}
inline void     noTone(uint8_t) {}
inline void     yield() {}
inline void     interrupts() {}
inline void     noInterrupts() {}
inline void     attachInterrupt(uint8_t, void(*)(), int) {}
inline void     detachInterrupt(uint8_t) {}
inline uint8_t  digitalPinToInterrupt(uint8_t pin) { return pin; }

#ifndef CHANGE
#define CHANGE  1
#define FALLING 2
#define RISING  3
#endif

class Print {
public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t c) = 0;
    virtual size_t write(const uint8_t* buf, size_t size) {
        size_t n = 0;
        for (size_t i = 0; i < size; ++i) n += write(buf[i]);
        return n;
    }
    size_t print(const char* s) {
        return write(reinterpret_cast<const uint8_t*>(s), strlen(s));
    }
    size_t println(const char* s) {
        size_t n = print(s);
        n += write('\n');
        return n;
    }
    size_t println() { return write('\n'); }
};

// Printable — base class for objects that can print themselves to a Print stream.
// ArduinoJson references this in ConverterImpl.hpp when ARDUINO is defined.
class Printable {
public:
    virtual ~Printable() = default;
    virtual size_t printTo(Print& p) const = 0;
};

// Stream class — inherits Print, defined in Stream.h.
#include "Stream.h"

class MockSerial : public Print {
public:
    size_t write(uint8_t c) override {
        _output += static_cast<char>(c);
        return 1;
    }
    int available() { return 0; }
    int read() { return -1; }
    void begin(unsigned long) {}

    std::string flush() {
        std::string out = std::move(_output);
        _output.clear();
        return out;
    }
    const std::string& getOutput() const { return _output; }

private:
    std::string _output;
};
inline MockSerial Serial;

#define WL_CONNECTED 3
#define WL_DISCONNECTED 6

class IPAddress {
public:
    IPAddress() : _addr(0) {}
    IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet) {
        _addr = first_octet | (second_octet << 8) | (third_octet << 16) | (fourth_octet << 24);
    }
    std::string toString() const {
        return std::to_string(_addr & 0xFF) + "." + 
               std::to_string((_addr >> 8) & 0xFF) + "." + 
               std::to_string((_addr >> 16) & 0xFF) + "." + 
               std::to_string((_addr >> 24) & 0xFF);
    }
    uint8_t operator[](int index) const {
        return (_addr >> (index * 8)) & 0xFF;
    }
private:
    uint32_t _addr;
};

#ifndef ESP_OK
#define ESP_OK 0
#endif
#ifndef ESP_FAIL
#define ESP_FAIL -1
#endif
typedef int esp_err_t;
