# Graveyard: PlatformIO Build Caching Issue

## Incident
During verification, the initial hardware target build `pio run -e esp32s3nano` failed with:
```
*** [.pio/build/esp32s3nano/https_server.crt.S.o] Source `.pio/build/esp32s3nano/https_server.crt.S' not found, needed by target `.pio/build/esp32s3nano/https_server.crt.S.o'.
```

## Resolution
The issue is caused by dirty caching in the build directory (`.pio/build/esp32s3nano`) when configuration or sdkconfigs change. Running a clean task:
```bash
pio run -t clean -e esp32s3nano
```
followed by a rebuild successfully resolved the issue.
