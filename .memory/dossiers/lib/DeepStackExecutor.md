# lib/DeepStackExecutor

## Invariants & contracts

### [docseed-16f31f8c] `DeepStackExecutor::run()` (`lib/DeepStackExecutor/DeepStackExecutor.h` — one-shot task submission)
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/coding-patterns.md · evidence: doc:docs/developer/coding-patterns.md`

### [docseed-215519db] **Pre-allocate Buffers** — For repeated I/O operations (serial parsing, HTTP responses), allocate the buffer once at initialisation and reuse it.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-2260d98f] **`heap_caps_get_free_size()` ≠ allocatable memory.** Total free heap can report 60 KB, but if the largest contiguous block is only 20 KB, a 30 KB TLS allocation will still fail. Always check `heap_caps_get_largest_free_block()` for TLS readiness.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-23975b58] [Coding Patterns](coding-patterns.md) — Modules vs Libraries, SOLID, anti-patterns
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-27d302d5] [Architecture Overview](architecture-overview.md) — System design and lifecycle
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-2b132d36] Only reconnect when the connection drops or the server closes it.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-32320f82] **DeepStackExecutor**: Stack-intensive worker tasks (such as JSON parsing or cryptographic operations) that do not perform flash writes may utilize PSRAM task stacks via `DeepStackExecutor` (`lib/DeepStackExecutor/`) to preserve internal DRAM.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-63bb10f9] If PSRAM is unavailable, `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)` returns `nullptr`. Always check the return value.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-86c946fb] Disable unused network protocols (PPP, SLIP).
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-99ce8d63] Setting `DEFAULT_NONE` while keeping `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y` preserves the bundle API — downstream code can still call `esp_crt_bundle_attach()`. It only removes the default CA certificates. If you disable the bundle entirely (`CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=n`), any code calling the bundle API will fail to link.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-a239b3cf] A stack in PSRAM is significantly slower. Function calls and local variable access have higher latency due to SPI bus overhead. Profile your task performance after moving to PSRAM.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-a91258c4] **Avoid Arduino `String`** — Arduino's `String` class uses hidden dynamic allocation and concatenation operations that fragment the heap quickly. Use `std::string`, `const char*`, or fixed-size `char` arrays. If `String` is unavoidable, call `reserve()` upfront.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-aefdc157] **Initial Reserve:** The framework reserves capacity upfront for the anticipated number of modules.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-b6d9cc9c] **Payload:** `SystemHeapPayload` containing `free_heap_bytes` and `minimum_free_heap_bytes`
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-bd471edb] Reducing `MBEDTLS_SSL_MAX_CONTENT_LEN` to 4096 requires the server to support TLS fragment sizes ≤ 4096. Most modern servers do, but verify before deploying.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-c9eefea6] These numbers vary significantly based on which modules are registered, how many web routes exist, and whether TLS connections are active. Always measure your specific configuration.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-d859021f] **No Heap Allocation in `tick()` Loops** — Code inside `tick()` executes dozens of times per second. `new`, `malloc`, `String` concatenation, and `std::vector::push_back()` inside `tick()` will fragment the heap within hours. Pre-allocate buffers as class members during `begin()`.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-ddb9fc2f] [Embedded Safety](embedded-safety.md) — Thread safety, memory rules, power management
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-e935dde1] Reuse the connection across fetch cycles where the server supports HTTP keep-alive.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-fba352b0] The 16 KB RX buffer (`CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN=16384`) **cannot be reduced**. CDNs (Fastly, Cloudflare) send full-size TLS records. A smaller buffer causes `MBEDTLS_ERR_SSL_BUFFER_TOO_SMALL` failures.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-fcf16081] Keep the `WiFiClientSecure` as a module member variable.
`invariant · kind: semantic · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

## Ruled out, failure signatures and gotchas

### [docseed-3eb4d130] The handshake requires a **contiguous** block of memory. Even if total free heap is 60 KB, if fragmentation has reduced the largest contiguous block to 25 KB, the handshake will fail with an `mbedtls` allocation error. Use `heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)` to check contiguous availability before initiating TLS connections.
`failure-signature · kind: procedural · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

## Procedures / Recipes

### [docseed-511e7305] [Build System](build-system.md) — PlatformIO configuration
`recipe · kind: procedural · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`

### [docseed-e4326b4b] The returned value is in **words** (4 bytes each on ESP32), not bytes. Multiply by 4 to get the byte count. Check ESP-IDF documentation for your specific port.
`recipe · kind: procedural · anchors: lib/DeepStackExecutor/DeepStackExecutor.h · source_doc: docs/developer/performance-guide.md · evidence: doc:docs/developer/performance-guide.md`
