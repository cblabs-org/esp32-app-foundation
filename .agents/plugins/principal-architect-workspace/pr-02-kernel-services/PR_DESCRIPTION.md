ESP32 App Foundation - PR-02 Kernel Services

Why:
The ESP32 application required foundational, "always-present" kernel services to handle low-level device operations securely and reliably. These services must be decoupled, modular units wired into the central Application hub, avoiding monolithic design and ensuring maintainable, testable code for cryptography, logging, OTA updates, WiFi management, and device configuration.

What:
This PR implements the core kernel services for the ESP32 App Foundation:
- deviceCrypto: Hardware MAC key derivation and AES-256 encryption.
- logger: ESP-IDF wrapper with secure logging macros.
- md5Utils: Streaming MD5 validation for firmware updates.
- githubClient: HTTPS client with strict certificate validation.
- configManager: LittleFS-backed configuration registry (`etc/config.json`).
- keyManager: Encrypted credential registry.
- wifiManager: Captive portal AP and AppState transitions.
- timeManager: Non-blocking NTP synchronization with POSIX timezones.
- otaUpdateManager: Secure streaming OTA updates with rollback protection.

All services adhere strictly to the project's Object-Oriented paradigms and House Style Documentation standards.
Note: Integration tests for wifiManager and otaUpdateManager have been scaffolded but are pending physical hardware verification.
