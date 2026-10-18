# WO2: LogPipeline Correctness

## Objective
Fix the cross-task race condition and implement the level-conditional opportunistic buffer strategy.

## Tasks
1. **Locking**: Implement a FreeRTOS static plain mutex. Derived timeout: `(bufSize * 10 / baud) * 3` with a floor. Bypass lock in early boot/ISR.
2. **Buffer Strategy**:
   - 256 B static `.bss` buffer (always on).
   - If `CONFIG_LOG_MAXIMUM_LEVEL >= 4`, perform a sizing pass using `va_copy`. If it exceeds 256 B, use a grow-and-retain heap buffer (cap ~16 KB). Do not log allocation failures.
3. **Re-entrancy**: Guard with `s_inHook`. If re-entered, fall back to the static buffer *with redaction*, do not chain to raw `vprintf`.
4. **Tail-Prefix Truncation Check**: Check if the truncated tail forms a partial secret and mask it.
5. **Sink & Sanitizer Injection**: Accept `ISanitizer*` and `Sink` in `begin()`, forcing singleton init. Remove per-line `fflush`.
