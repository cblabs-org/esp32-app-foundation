#pragma once
#include "Arduino.h"
#include <fstream>
#include <filesystem>
#include <string>

class File {
public:
    File() {}
    File(const std::string& path, const char* mode) : _path(path) {
        std::ios_base::openmode open_mode = std::ios_base::in | std::ios_base::out;
        if (mode && mode[0] == 'w') open_mode = std::ios_base::out | std::ios_base::trunc;
        if (mode && mode[0] == 'a') open_mode = std::ios_base::out | std::ios_base::app;
        if (mode && mode[0] == 'r') open_mode = std::ios_base::in;
        _fs.open(path, open_mode);
        _isOpen = _fs.is_open();
    }
    size_t write(const uint8_t *buf, size_t size) {
        if (!_isOpen) return 0;
        _fs.write(reinterpret_cast<const char*>(buf), size);
        return size;
    }
    size_t read(uint8_t* buf, size_t size) {
        if (!_isOpen) return 0;
        _fs.read(reinterpret_cast<char*>(buf), size);
        return _fs.gcount();
    }
    int read() {
        if (!_isOpen) return -1;
        int c = _fs.get();
        return _fs.eof() ? -1 : c;
    }
    size_t write(uint8_t c) {
        if (!_isOpen) return 0;
        _fs.put(c);
        return 1;
    }
    size_t readBytes(char* buf, size_t size) {
        if (!_isOpen) return 0;
        _fs.read(buf, size);
        return _fs.gcount();
    }
    size_t size() const {
        if (!_isOpen) return 0;
        std::error_code ec;
        auto s = std::filesystem::file_size(_path, ec);
        return ec ? 0 : s;
    }
    String readString() { return ""; }
    void print(const char* s) { if (_isOpen) _fs << s; }
    void println(const char* s) { if (_isOpen) _fs << s << "\n"; }
    void close() { if (_isOpen) { _fs.close(); _isOpen = false; } }
    operator bool() const { return _isOpen; }
    bool isDirectory() const { return false; }
    File openNextFile() { return File(); }
    const char* name() const {
        size_t pos = _path.find_last_of('/');
        if (pos != std::string::npos) return _path.c_str() + pos + 1;
        return _path.c_str();
    }
private:
    std::fstream _fs;
    bool _isOpen = false;
    std::string _path;
};

class FSClass {
public:
    bool begin(bool formatOnFail=false, const char * basePath="/littlefs", uint8_t maxOpenFiles=10, const char * partitionLabel="spiffs") { return true; }
    File open(const char* path, const char* mode = "r") {
        std::string fullPath = "test/tmp" + std::string(path);
        return File(fullPath, mode);
    }
    bool exists(const char* path) {
        return std::filesystem::exists("test/tmp" + std::string(path));
    }
    bool remove(const char* path) {
        return std::filesystem::remove("test/tmp" + std::string(path));
    }
    bool mkdir(const char* path) {
        std::error_code ec;
        return std::filesystem::create_directories("test/tmp" + std::string(path), ec);
    }
    bool rename(const char* pathFrom, const char* pathTo) {
        std::error_code ec;
        std::filesystem::rename("test/tmp" + std::string(pathFrom), "test/tmp" + std::string(pathTo), ec);
        return !ec;
    }
    size_t usedBytes() { return 50000; }
    size_t totalBytes() { return 150000; }
};
inline FSClass LittleFS;
namespace fs {
    using LittleFSFS = FSClass;
}
