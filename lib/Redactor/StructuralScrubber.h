#pragma once

#include "interfaces/ISanitizer.hpp"
#include <cstddef>

class StructuralScrubber : public ISanitizer {
public:
    static StructuralScrubber& instance() {
        static StructuralScrubber _instance;
        return _instance;
    }
    
    void sanitize(char* buffer, size_t bufferSize, bool isTruncated = false) override;
};
