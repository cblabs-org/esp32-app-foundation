# ADR: EventBus Concurrency Contract & Topic Interning

## Status
Accepted / Completed (WO1)

## Context
`EventBus` acts as the IPC bus for the microkernel. Prior to WO1:
1. `EventBus::publish` duplicated the entire `std::vector<Subscription>` targets array on every event publish call. Each `Subscription` contained a `std::string topic`.
2. This caused per-publish heap allocation overhead on the hot path.
3. No written concurrency contract existed for subscriber context, thread safety, or callback execution budgets.

## Decisions

### 1. Topic Interning with FNV-1a 32-bit Hashes
- Replaced `std::string topic` inside `Subscription` with `uint32_t topicHash`.
- Added `EventBus::hashTopic(const char* str)` using the standard FNV-1a 32-bit algorithm.
- Public interface methods (`subscribe`, `unsubscribe`, `publish`) continue accepting `const char*` externally, interning topics internally with **zero heap allocations** on publish.

### 2. In-Place Vector Dispatch & Tombstone Soft-Deletion
- `publish()` iterates in-place over `_subscribers` via integer index rather than duplicating the vector.
- Integer indexing ensures iterator safety even if `push_back()` causes vector reallocation when a callback invokes `subscribe()`.
- Re-entrant `unsubscribe()` calls (including self-unsubscription during callback execution) use tombstone soft-deletion (`active = false`) and set `_needsCleanup = true`.
- When all nested `publish()` calls complete (`_dispatchDepth == 0`), `cleanup()` compacts the vector using `std::remove_if`.

### 3. Concurrency Contract Specification
- Detailed docstring added to `EventBus.hpp`:
  - **Thread Safety**: Native dispatch is synchronous in caller thread context; target (ESP32) uses synchronous execution or `esp_event` backing.
  - **Topic Interning**: Zero allocation hot path.
  - **Re-entrancy**: Callbacks can safely sub/unsub during dispatch.
  - **Execution Budget**: Callbacks must adhere to cooperative tick budget (<= 10 ms).
  - **Payload Lifetime**: Payload pointers are valid only for the duration of the callback call.
