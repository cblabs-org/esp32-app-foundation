# Logging & Redaction System Design

## Architecture
The ESP32 App Foundation employs a global logging interception architecture (the `LogPipeline`) leveraging ESP-IDF's `esp_log_set_vprintf` hook. This guarantees that all log statements—including those from third-party libraries and underlying SDK code—are funneled through a central redaction engine before reaching standard output.

## Thread Safety and Buffer Management
The interception hook is fully thread-safe and interrupt-safe:
- **Production (`CONFIG_LOG_MAXIMUM_LEVEL <= 3`)**: Uses a single, lock-free (if re-entered) 256 B static `.bss` buffer. No allocations occur. This handles all production log lines safely.
- **Debug/Verbose (`CONFIG_LOG_MAXIMUM_LEVEL >= 4`)**: Supports opportunistic dynamic sizing. A dynamically growing (up to 16 KB) but retained heap buffer accommodates large debug dumps. This branch utilizes `va_copy` and a `FreeRTOS` static mutex (`xSemaphoreCreateMutexStatic`) with variable timeout based on baud rate capacity `(bufSize * 300) / 1152`.

## Redactor
The `Redactor` singleton maintains a bounded array of registered secrets (`MAX_SECRETS = 16`, `MAX_SECRET_LEN = 192`).
- Storage is fixed (`char[16][192]`).
- Registration runs an insertion sort to maintain descending string length, preventing partial masking of nested secrets (e.g. masking "password" before "pass").
- `findBounded()` safely traverses non-NUL terminated buffers without out-of-bounds reads.

## Structural Scrubber
To catch unregistered secrets and structured data spillage (like HTTP headers or JSON bodies), `StructuralScrubber` acts as a second-pass interceptor:
- Searches for keys such as `Authorization`, `Cookie`, `"password"`, `"token"`.
- Zero-allocation C-string scanning to overwrite the corresponding value.
- Chained in `Application.cpp` via a `CompositeSanitizer` exclusively for `CONFIG_LOG_MAXIMUM_LEVEL >= 4` runs.

## dumpPayload() API
For extremely large binary or text payloads (like full HTTP request bodies), the standard `ESP_LOGD` mechanism truncates or fails to allocate. Use `LogPipeline::dumpPayload(level, tag, data, length)`. This function slices the payload into manageable chunks, redacts each chunk independently, and sends it directly to the output sink, avoiding truncation while ensuring redaction compliance.
