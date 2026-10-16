# lib/LogPipeline

## Decisions

### [seed-116dece8] Explicitly documented the deferred status and architectural reasoning for `ILogSink` and RAM ring buffer in §11A.5, §11A.6, D29, and D30.
`decision · kind: episodic · anchors: lib/LogPipeline, lib/Redactor`

### [seed-7d8ff9ad] Stored native stubs in `test/stubs/platform/esp_log.h` (the platform stub header on the native include path) rather than modifying `include/platform/LogShim.h`, maintaining the clean `#include "esp_log.h"` forwarding in `LogShim.h`.
`decision · kind: episodic · anchors: lib/LogPipeline/LogPipeline.h, lib/LogPipeline/LogPipeline.cpp, lib/LogPipeline/library.json, test/stubs/platform/esp_log.h, modules/Application/Application.cpp`

### [seed-ae4a818b] Replaced all historical mentions of `Logger` and `LogRouter` with `LogPipeline` and `Redactor` across living documentation.
`decision · kind: episodic · anchors: lib/LogPipeline, lib/Redactor`

### [seed-eaf44e3f] Set 2 KB static buffer (`s_formatBuf`) in `.bss` as designed to zero stack and heap overhead during log formatting.
`decision · kind: episodic · anchors: lib/LogPipeline/LogPipeline.h, lib/LogPipeline/LogPipeline.cpp, lib/LogPipeline/library.json, test/stubs/platform/esp_log.h, modules/Application/Application.cpp`
