# WO2: LogPipeline Correctness (Completed)

## Tasks Executed
1. **Locking**: Implemented a FreeRTOS static plain mutex via `xSemaphoreCreateMutexStatic`. Timeout is derived using the `(bufSize * 300) / 1152` formula, with a floor of 50ms. Bypass lock if `xPortInIsr()` or the scheduler is not running.
2. **Buffer Strategy**:
   - Production uses a strictly 256 B static `.bss` buffer (allocation-free).
   - If `CONFIG_LOG_MAXIMUM_LEVEL >= 4`, we dynamically size using `va_copy`. If it exceeds 256 B, a grow-and-retain heap buffer (`s_heapBuf`) capped at 16KB is used.
   - Allocation failures do NOT log (to prevent recursive lockouts).
3. **Re-entrancy**: Guarded with `s_inHook`. If re-entered (e.g. `malloc` logs internally), we fall back to the 256 B static buffer, perform redaction, and output directly without taking locks or chaining back to `vprintf`.
4. **Tail-Prefix Truncation**: Replaced the lossy guard-zone check with the tail-prefix verification, passing the `truncated` boolean through to the `ISanitizer::sanitize` interface.
5. **Sanitizer Injection**: Updated `LogPipeline::begin(ISanitizer*, Sink)` to force singleton injection.
6. **`dumpPayload()` Skeleton**: Added the `dumpPayload` signature to support the WO4 work.

Verified via compilation for ESP32 hardware (`pio run -e esp32dev`).
