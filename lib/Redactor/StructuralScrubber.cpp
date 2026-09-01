#include "StructuralScrubber.h"
#include <cstring>

namespace {
    void scrubKeyValues(char* buffer, size_t bufLen, const char* keyPattern) {
        // Very rudimentary structural scrubber that searches for "pattern"
        // and blanks out the subsequent string literal or token.
        // C-style string functions for safety and zero-allocation.
        
        char* searchStart = buffer;
        size_t keyLen = std::strlen(keyPattern);
        
        while (char* match = strcasestr(searchStart, keyPattern)) {
            char* valStart = match + keyLen;
            // Skip spaces, colons, equals, quotes
            while (*valStart != '\0' && (*valStart == ' ' || *valStart == ':' || *valStart == '=' || *valStart == '"')) {
                valStart++;
            }
            
            if (*valStart != '\0') {
                // Find end of the value
                char* valEnd = valStart;
                while (*valEnd != '\0' && *valEnd != '"' && *valEnd != '\r' && *valEnd != '\n' && *valEnd != ',' && *valEnd != '&') {
                    valEnd++;
                }
                
                // Redact the value
                std::memset(valStart, '*', valEnd - valStart);
                searchStart = valEnd;
            } else {
                break;
            }
        }
    }
}

void StructuralScrubber::sanitize(char* buffer, size_t bufferSize, bool isTruncated) {
    if (!buffer || bufferSize == 0) return;
    
    size_t bufLen = strnlen(buffer, bufferSize);
    if (bufLen == 0) return;

    // HTTP Headers
    scrubKeyValues(buffer, bufLen, "Authorization");
    scrubKeyValues(buffer, bufLen, "Cookie");
    scrubKeyValues(buffer, bufLen, "Set-Cookie");
    
    // JSON Keys or Form data
    scrubKeyValues(buffer, bufLen, "\"password\"");
    scrubKeyValues(buffer, bufLen, "\"token\"");
    scrubKeyValues(buffer, bufLen, "\"access_token\"");
    scrubKeyValues(buffer, bufLen, "\"refresh_token\"");
}
