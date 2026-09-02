#pragma once
#include "interfaces/ISanitizer.hpp"
#include <vector>

class CompositeSanitizer : public ISanitizer {
public:
    void add(ISanitizer* sanitizer) {
        if (sanitizer) {
            _sanitizers.push_back(sanitizer);
        }
    }
    
    void sanitize(char* buffer, size_t bufferSize, bool isTruncated = false) override {
        for (auto* sanitizer : _sanitizers) {
            sanitizer->sanitize(buffer, bufferSize, isTruncated);
        }
    }
private:
    std::vector<ISanitizer*> _sanitizers;
};
