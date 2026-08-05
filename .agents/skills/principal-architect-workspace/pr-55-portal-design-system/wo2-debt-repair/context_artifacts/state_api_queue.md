# State: ApiQueue

## Priority Queue Architecture
The `ApiQueue` protects the ESP32 socket pool by strictly serializing fetch requests. It now supports two lanes:
1. **highQueue (UI):** Triggered by standard `window.apiFetch`. Processed immediately.
2. **lowQueue (Background):** Triggered by `window.apiFetch.enqueueTask`. Processed only when highQueue is empty. Followed by a configurable delay (`lowDelayMs`) to give the system breathing room.

## Pre-emption
If a UI request arrives while a background task delay is ticking down, the `_delayTimer` is explicitly cancelled and the `processNext()` loop resumes immediately to service the `highQueue`.
