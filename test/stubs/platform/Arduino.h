#pragma once
#include <cstdint>
#include <string>

// String class migrated from NativeBuildStubs.hpp
#ifndef STRING_STUB_DEFINED
#define STRING_STUB_DEFINED
class String {
public:
    String() {}
    String(const char* s) : _str(s ? s : "") {}
    String(const std::string& s) : _str(s) {}
    String(int value, int base) {
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
private:
    std::string _str;
};
#endif

inline unsigned long millis() { return 0; }
inline void delay(uint32_t ms) { (void)ms; }

class Print {
public:
    virtual ~Print() {}
    virtual size_t write(uint8_t) = 0;
};

class SerialClass : public Print {
public:
    void begin(unsigned long baud) { (void)baud; }
    int available() { return 0; }
    int read() { return -1; }
    size_t write(uint8_t c) override { return 1; }
};
inline SerialClass Serial;

#define WL_CONNECTED 3
#define WL_DISCONNECTED 6

class IPAddress {
public:
    IPAddress() : _addr(0) {}
    IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet) {
        _addr = first_octet | (second_octet << 8) | (third_octet << 16) | (fourth_octet << 24);
    }
    String toString() const { return "0.0.0.0"; }
private:
    uint32_t _addr;
};

#ifndef ESP_OK
#define ESP_OK 0
#endif
typedef int esp_err_t;
