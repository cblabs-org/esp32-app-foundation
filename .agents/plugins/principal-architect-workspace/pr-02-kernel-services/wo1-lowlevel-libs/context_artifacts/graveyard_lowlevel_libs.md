# Graveyard: Low-Level Libraries

## Raw Standard Library Crypto
Initial attempts used standard C++ library components for hashing and encrypting. This added significant binary footprint overhead.
- **Lesson Learned**: Rely on the ESP32's built-in cryptographic hardware acceleration via ESP-IDF/MbedTLS APIs rather than bundling custom crypto libraries.
