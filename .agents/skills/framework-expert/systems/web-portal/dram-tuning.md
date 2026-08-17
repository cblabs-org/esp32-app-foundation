# Web Server DRAM Tuning

## The Problem

The framework's `WebServerManager` and its underlying PsychicHttp library
have default configuration values that can exhaust internal DRAM on boards
with high static memory usage (>75% DRAM at link time).

### Default Configuration

| Setting | Default | Source |
|---------|---------|--------|
| `config.stack_size` | **16,384** (16 KB) | Framework (`WebServerManager` constructor, for PNG screenshot compression) |
| `ASYNC_WORKER_COUNT` | **8** | PsychicHttp (`async_worker.h`, hardcoded `#define`, no `#ifndef` guard) |
| `config.max_open_sockets` | **9** (workers + 1) | PsychicHttp (`PsychicHttpServer.cpp` constructor) |
| Worker task stack | **4,096** (4 KB) | PsychicHttp (`ASYNC_WORKER_TASK_STACK_SIZE`) |

### Total Internal DRAM Cost

- **8 async worker tasks** × 4 KB = 32 KB (allocated at startup, cannot be reduced via config)
- **HTTP server task** × 16 KB = 16 KB
- **9 socket descriptors** + scratch buffers ≈ varies
- **Total**: ~144 KB of internal DRAM for HTTP infrastructure alone

On a board like CrowPanel-128-rotary (ESP32-S3), static DRAM usage is
80.16% — only **67 KB remains**. The HTTP stacks consume it all, starving
the lwIP TCP stack of `pbuf` allocation space.

## Symptoms

When internal DRAM is exhausted:

- All gzip+PROGMEM portal assets return **HTTP 200 with correct
  Content-Length but 0 bytes delivered** — the client times out
- Small JSON API responses (`/api/system`, MCP JSON-RPC) still work
  because they fit in the initial TCP segment
- `heap_min_free` collapses to < 500 bytes under any HTTP load
- The portal appears completely non-functional

The root cause is that `lwIP` allocates `pbuf` structures from internal
DRAM to fill the TCP send window. When all internal DRAM is consumed by
HTTP task stacks, no pbufs can be allocated and response bodies never
reach the socket — even though headers were sent during the initial
handshake.

## Fix: Override Config Before `app.begin()`

Consuming apps on DRAM-constrained boards must override the HTTP config
after the `Application` object is constructed but before `begin()`:

```cpp
// In setup(), before app.begin():
auto& httpServer = app.getWebServerManager().getServer();
httpServer.config.stack_size      = 8192;  // down from 16384
httpServer.config.max_open_sockets = 4;    // down from 9

if (!app.begin(config)) { ... }
```

### Why These Values

- **8,192 stack**: Sufficient for all normal HTTP handlers (JSON APIs,
  gzip asset serving, config endpoints). Only the PNG screenshot
  compression path (uses `tdefl`) needs 16 KB.
- **4 max_open_sockets**: Matches a realistic concurrent connection count
  for a device portal. The 8 async workers still exist but only 4 sockets
  can be open simultaneously, reducing peak DRAM pressure.

### Measured Results

| Metric | 16 KB / 9 sockets | 8 KB / 4 sockets |
|--------|-------------------|-------------------|
| `heap_free` | 14,523 | **23,411** (+61%) |
| `heap_min_free` | 484 (broken) | **10,060** |
| `heap_max_alloc` | 7,680 | **13,312** (+73%) |
| Asset delivery | 0 bytes (timeout) | **< 70 ms** |

## Important Caveats

1. **`ASYNC_WORKER_COUNT` cannot be overridden.** It is a hardcoded
   `#define` in PsychicHttp with no `#ifndef` guard. The 8 worker tasks
   (32 KB total) are always allocated at startup. Only
   `config.max_open_sockets` and `config.stack_size` can be tuned.

2. **Connection pool exhaustion is sticky.** Each timed-out HTTP request
   holds a PsychicHttp socket slot until the TCP keepalive expires
   (~120 s). Rapid-fire requests during debugging can saturate all slots,
   making the device appear unresponsive. Power-cycle to recover.

3. **Screenshot compression may fail at 8 KB stack.** If the screenshot
   API is needed on a DRAM-constrained board, the framework should spawn
   a dedicated task with a larger stack on-demand rather than inflating
   the global HTTP stack. See framework issue
   [#115](https://github.com/matthewmcneill/esp32-app-foundation/issues/115).

4. **The gzip+PROGMEM pipeline is zero-DRAM.** `serveGzipAsset()` serves
   directly from flash — the gzipped bytes are never copied to a heap
   buffer. The only DRAM cost is the HTTP response framing (~200 bytes).

## Framework Issue

[#115 — feat(web): make HTTP stack_size and max_open_sockets board-configurable](https://github.com/matthewmcneill/esp32-app-foundation/issues/115)
