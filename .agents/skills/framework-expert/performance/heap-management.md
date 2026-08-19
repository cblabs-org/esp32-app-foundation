# Heap Management Strategy

Efficient heap management is critical to prevent fragmentation and Out of Memory (OOM) crashes in the ESP32 App Foundation.

## Two-Tier Vector Model

To prevent continuous reallocation and fragmentation as the module vector grows during boot:

1. **Initial Reserve:** The framework reserves capacity upfront for anticipated modules.
2. **Compact and Lock:** At the end of `AppFoundation::begin()`, the framework calls `shrink_to_fit()` on all major module vectors and locks them, converting them to a read-only list for the duration of runtime.

This guarantees that vectors use the exact amount of memory needed and no more, returning unused reserved memory to the heap.

## Monitoring the Heap

The framework publishes real-time heap telemetry via the EventBus:

- **Topic:** `system.heap`
- **Payload:** `SystemHeapPayload` (contains `free_heap_bytes` and `minimum_free_heap_bytes`)

### API Functions
- `heap_caps_get_free_size(MALLOC_CAP_INTERNAL)`: Returns the total free internal heap.
- `heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)`: Essential for TLS, as TLS needs a *contiguous* block.

## Anti-Fragmentation Rules

**Goal:** Prevent heap **fragmentation from repeated alloc/free cycles**, not ban all transient memory use. Fixed-size stack allocations (`char buf[N]`) and pre-allocated containers are safe in `tick()`. `JsonDocument` used in request handlers is safe because `getMaxPostBody()` caps input size before parsing.

**Avoid in `tick()`:** `new`, `malloc`, `push_back` (may reallocate), Arduino `String` concatenation.

**Safe in `tick()`:** Stack-allocated `char[]` buffers, pre-allocated `std::vector` (populated at boot, read-only at runtime). `JsonDocument` is safe in request handlers (not `tick()`) when input is bounded by `getMaxPostBody()`.

*Full rules:* See [Memory Rules](../embedded-safety/memory-rules.md).

- **Avoid `Arduino::String`:** Use `std::string` or `const char*` where possible. If `String` is necessary, use `reserve()`.
- **Preallocate Buffers:** For repeated I/O operations, allocate the buffer once at initialization.

## Resource Lifecycle Exception

Heap-intensive services with bounded lifecycle (created for a task, destroyed when done) MAY be instantiated locally when their permanent allocation would be wasteful.

Example: `WiFiClientSecure` in an OTA download task is created on-demand via `std::unique_ptr`, used for the duration of the download, and destroyed when the task completes. Permanently holding a `WiFiClientSecure` as a class member would waste ~40KB of heap when OTA is not active.

**Rule:** Document the heap impact of any locally instantiated heavy object. Use `heap_caps_get_free_size()` before and after to verify the allocation and deallocation are balanced.

> **See also:** [PSRAM — LV_CONF_PATH Override Warning & PSRAM-Preference Audit](psram.md)
> — if your project overrides `LV_CONF_PATH`, framework LVGL memory changes do not
> apply to you. The PSRAM-Preference Audit checklist provides a systematic review
> for any module's memory allocation strategy.

## See Also
- [TLS Tradeoffs](../security/tls-tradeoffs.md)
- [PSRAM Utilisation](psram.md)
