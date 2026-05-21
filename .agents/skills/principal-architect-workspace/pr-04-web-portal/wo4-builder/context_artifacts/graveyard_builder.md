# Graveyard: Portal Assets Builder

## Serving Assets from LittleFS
Initially, storing the raw portal files directly in the LittleFS partition was considered. However, this required separate file upload steps during programming, increased filesystem write overhead, and risked corruption if the partition was modified at runtime.
- **Lesson Learned**: Compiling compressed assets directly into the firmware binary as `PROGMEM` headers ensures they are immutable, read-only, and automatically flashed alongside the main binary.
