# Graveyard: Rejected Approaches for Issue #16

## Option A: Increase Outbox Size

**Idea:** Set `outbox.limit` to a much larger value (e.g. 64 KB) to accommodate the full burst.

**Why it failed:** Only moves the cliff. At some entity count (roughly entity_count × avg_payload_size > outbox_limit), it would fail again. Does not address root causes #1 (tight loop), #2 (MQTT task blocking), or #3 (no return-value checking). At 197 entities × ~400 bytes each = ~79 KB — already exceeds 64 KB. And on a device with 320 KB total DRAM, dedicating 64 KB+ to an MQTT outbox is untenable.

**Lesson:** Never "solve" a flooding problem by making the bucket bigger.

---

## Option B: Back-Pressure Retry in Callback

**Idea:** Check `esp_mqtt_client_publish()` return value in the existing `publishDiscovery()` loop. On `-2` (outbox full), add a `vTaskDelay()` and retry.

**Why it failed:** `publishDiscovery()` runs inside the `MQTT_EVENT_CONNECTED` callback, which executes on the **MQTT task's stack**. The MQTT task is the only thread that drains the outbox. While our callback is executing (even with delays), the MQTT task cannot process outbound messages. This creates a deadlock-like scenario: we fill the outbox, then wait for it to drain, but the drainer is us, and we're waiting.

Specific failure mode: `vTaskDelay(100)` in the callback yields the MQTT task's CPU time, but doesn't allow the MQTT task to run its outbox-drain loop — that loop only runs when the event handler returns. So the retry would spin indefinitely or until the outbox limit was hit.

**Lesson:** Never add retry/delay loops inside an event handler that runs on the same task context that processes the work queue.

---

## Option C: FreeRTOS Queue + Dedicated Task

**Idea:** Push entity pointers into a FreeRTOS queue from the MQTT callback, and have a dedicated task drain the queue with pacing.

**Why it failed (rejected, not technically broken):** Adds a new task (stack allocation, scheduling complexity), a queue (memory allocation), and cross-task synchronisation. The framework already provides `tick()` on the main loop, which is the idiomatic place for periodic work. Creating a dedicated task for something that `tick()` handles naturally violates the framework's Service Module Pattern and adds unnecessary complexity for a problem that has a straightforward solution within existing infrastructure.

**Lesson:** Prefer framework idioms (`tick()`) over custom threading infrastructure when the framework already provides the right execution context.

---

## Implementation Gotchas (for future reference)

1. **`getObjectId()` returns `std::string`**, not `const char*`. Using it directly in `ESP_LOGW(TAG, "...%s...", entity->getObjectId())` compiles on ESP32 (GCC is lenient) but fails on the native test build (Clang with `-Wnon-pod-varargs`). Always use `.c_str()`.

2. **`outbox` member missing from test stub.** The native build's `mqtt_client.h` stub didn't have the `outbox` struct. First attempt used `#ifndef NATIVE_BUILD` but `NATIVE_BUILD` isn't defined anywhere in the project. Fixed by adding the member to the stub instead.

3. **Environment name:** The PlatformIO project uses `esp32dev`, not `esp32s3`. First build attempt failed with `UnknownEnvNamesError`.
