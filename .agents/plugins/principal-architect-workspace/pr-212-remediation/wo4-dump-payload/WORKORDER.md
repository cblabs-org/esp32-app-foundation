# WO4: dumpPayload() API & Structural Scrubber

## Objective
Provide a safe path for API payload debugging and catch unregistered secrets.

## Tasks
1. **API**: Add `void LogPipeline::dumpPayload(esp_log_level_t level, const char* tag, const void* data, size_t len)` to redact before chunking to sink.
2. **StructuralScrubber**: Create `StructuralScrubber` implementing `ISanitizer`. Use C-string functions (`strcasestr`, etc.) to scrub common headers (Authorization, Cookie, etc.) and JSON keys ("password", "token", etc.).
3. **Chaining**: Chain `StructuralScrubber` with `Redactor` in `LogPipeline`, gated by `CONFIG_LOG_MAXIMUM_LEVEL >= 4`.
