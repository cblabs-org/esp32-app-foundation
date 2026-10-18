# Graveyard: WO3 UI Widgets & Application Assembly

## Native Testing
We attempted to run `run_tests` natively to verify compilation comprehensively. However, the native test environment for `esp32-app-foundation` encountered compilation errors inside `OTAUpdateManager.cpp` (undefined `millis()`), which is framework code outside of the `sample-app` boundary box.
**Resolution**: We ignored the native test failures and verified the `esp32s3nano` target build instead, which compiled `firmware.bin` successfully without errors.
