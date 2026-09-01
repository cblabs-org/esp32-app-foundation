# WO4: dumpPayload() API & Structural Scrubber (Completed)

## Tasks Executed
1. **API**: Added `LogPipeline::dumpPayload(esp_log_level_t, const char*, const void*, size_t)` to `LogPipeline`. This function chunks large binary/text payloads (e.g. 512 bytes) directly to the sink to avoid intermediate buffer constraints or truncation, redacting each chunk in-flight via the injected `ISanitizer`. It is gated on `CONFIG_LOG_MAXIMUM_LEVEL >= 4`.
2. **StructuralScrubber**: Created `StructuralScrubber` implementing `ISanitizer`. It uses zero-allocation C-string scanning to identify and mask values for common sensitive structures (e.g., `Authorization`, `Cookie`, `Set-Cookie`, `"password"`, `"token"`).
3. **Chaining**: Created `CompositeSanitizer` to chain multiple `ISanitizer` instances. In `Application.cpp`, `StructuralScrubber` is chained with `Redactor` when `CONFIG_LOG_MAXIMUM_LEVEL >= 4`.
