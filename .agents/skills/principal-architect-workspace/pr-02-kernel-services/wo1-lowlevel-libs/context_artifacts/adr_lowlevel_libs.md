# Architecture Decision Record: Low-Level Libraries

## Custom Non-Monolithic Libraries
All helper functions are grouped into distinct, independent libraries: `deviceCrypto`, `logger`, `md5Utils`, and `githubClient`. This avoids a single helper file and isolates responsibilities.

## Secure Logging
The `logger` library provides a wrapper over the ESP-IDF logging system and introduces a `LOG_SECURE()` macro. This macro replaces sensitive variables (like API keys or passwords) with redacted stubs to prevent secrets leaking in logging outputs.

## Streaming MD5 Validation
The `md5Utils` library supports calculating hashes incrementally using ESP-IDF/MbedTLS APIs, allowing streaming data (like incoming OTA binaries) to be validated packet-by-packet rather than loading the entire file into RAM first.
