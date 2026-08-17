# Web Server Stack & DRAM Tuning Architecture

> ESP32 App Foundation (c) 2026 CB Labs
>
> Operational guide and reference for the HTTP server task stack sizing, PSRAM deep-stack offloading, DRAM optimization, and OTA flash-safety invariants.

---

## 1. Overview & Root-Cause Model

The `WebServerManager` manages the HTTP (port 80) and HTTPS (port 443) web servers using the PsychicHttp library on top of ESP-IDF `esp_http_server`.

### Debunked Myths & Correct Concurrency Model

Earlier documentation and PR drafts (#116) assumed PsychicHttp spawned an async worker pool. Empirical toolchain analysis and codebase audit established the correct model:

- **`ENABLE_ASYNC` is NOT defined**: PsychicHttp compiles out its async-worker pool entirely. There is **no pool of 8 async workers** and no 32 KB pool overhead.
- **Single Synchronous `httpd` Task**: There is exactly **one** httpd server task running all HTTP handlers synchronously.
- **Socket Allocation**: Default maximum open sockets is **7** (`HTTPD_DEFAULT_CONFIG`), not 9.
- **Footprint**: The total static HTTP server infrastructure footprint is ~8–10 KB (task stack + socket control structures), NOT ~144 KB.

### Why the HTTP Stack Was 16 KB

Historically, `WebServerManager` explicitly configured `config.stack_size = 16384` (16 KB) on the HTTP server task. This large stack was required by a **single** route: the screenshot capture endpoint (`DisplayServiceHelper` → `IDisplayModule::capturePNG` → `tdefl` compression). `tdefl` (ROM Miniz deflate compressor) and `BufferStream` call chains require a deep call stack (~12–14 KB peak).

Sizing the shared `httpd` task stack for this worst-case single route consumed 16 KB of contiguous **internal DRAM** on every board. On DRAM-constrained boards (e.g. CrowPanel 1.28" rotary with ~80% static DRAM usage), this starves lwIP of network pbufs, leading to transient HTTP-200 / 0-byte truncated response delivery.

---

## 2. Architecture: Deep-Stack Offloading

To reclaim internal DRAM without sacrificing real-time PNG screenshot generation, deep-stack operations are decoupled from the shared web server task using the `DeepStackExecutor` service.

```
                   +------------------------+
                   |   PsychicHttp Task     |
                   |  (httpd stack: 8 KB)   |
                   +-----------+------------+
                               |
                               | (Screenshot GET /api/display/screenshot.png)
                               v
                   +------------------------+
                   |  DisplayServiceHelper  |
                   +-----------+------------+
                               |
         +---------------------+---------------------+
         | (PSRAM available +                        | (No PSRAM or
         |  SPIRAM_ALLOW_STACK)                      |  OTA Active)
         v                                           v
+----------------------------------+       +-------------------+
|       DeepStackExecutor          |       | Stored-PNG        |
| Persistent Worker Task           |       | Shallow Fallback  |
| Stack: 16 KB in PSRAM (SPIRAM)   |       | Path              |
| Runs: tdefl PNG Compression      |       +-------------------+
+----------------------------------+
```

### DeepStackExecutor

`DeepStackExecutor` ([lib/DeepStackExecutor/DeepStackExecutor.h](../../../../lib/DeepStackExecutor/DeepStackExecutor.h)) is a reusable framework service that:
1. Spawns a persistent worker task whose stack resides in **external PSRAM** using `xTaskCreateWithCaps(..., MALLOC_CAP_SPIRAM)` (matching the pattern used by `OTAUpdateManager`).
2. Provides a synchronous, serialized `run(callable, timeoutMs)` interface to execute heavy callables on the PSRAM worker thread.
3. Automatically falls back to `available() == false` if PSRAM stack allocation is unsupported or fails at boot.

By offloading the 16 KB call stack to PSRAM, the HTTP server task stack size is reduced from 16 KB to **8 KB** (`BOARD_HTTP_STACK_SIZE`), immediately returning **8 KB of internal DRAM** to lwIP and the system heap.

---

## 3. Board Preconditions & Capability Matrix

### Hardware & Toolchain Preconditions

For `DeepStackExecutor` to allocate worker task stacks in external PSRAM, the board and SDK configuration must meet the following criteria:

1. **Physical PSRAM**: The microcontroller must have PSRAM available and enabled (e.g., `-DBOARD_HAS_PSRAM`).
2. **Kconfig Flag**: `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY=y` must be enabled in `sdkconfig` (or PlatformIO build flags). Without this flag, ESP-IDF rejects external memory task stack allocations.

If either condition is absent, `DeepStackExecutor::begin()` fails gracefully (`available() == false`), and screenshot requests automatically serve the shallow **stored-PNG** fallback asset.

### Capability Matrix

| Board Capability | Screenshot Path | httpd Stack Size | Internal DRAM Saving |
|---|---|---|---|
| PSRAM + `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY=y` | Real-time `tdefl` on PSRAM worker (`DeepStackExecutor`) | 8 KB (`BOARD_HTTP_STACK_SIZE`) | **−8 KB** internal DRAM |
| No PSRAM or external stacks disabled | Stored-PNG shallow fallback path | 8 KB (`BOARD_HTTP_STACK_SIZE`) | **−8 KB** internal DRAM |

---

## 4. OTA Flash-Safety Invariants (Mandatory Rules)

> [!CAUTION]
> **SPI Flash Cache Invalidation Rules**
>
> During SPI flash write operations (OTA firmware updates, LittleFS config saves), the ESP32 hardware disables the SPI flash/PSRAM cache controller. Any attempt to read or execute code from external PSRAM while the cache is disabled causes an immediate, unrecoverable **Fatal CPU Cache Fault crash**.

All deep-stack offloading and web server configurations MUST strictly obey the following three flash-safety invariants:

### Rule 1: The `httpd` Task Stack MUST Remain Internal DRAM

The HTTP server task stack **MUST NEVER** be placed in PSRAM (`task_caps` must remain `MALLOC_CAP_INTERNAL`). Direct-upload OTA (`Update.write`) and LittleFS configuration saves execute directly on the `httpd` task thread. Placing the HTTP server task stack in PSRAM would crash the CPU during an OTA upload.

### Rule 2: The OTA Worker Task is Internal-DRAM Stacked

`OTAUpdateManager` creates its persistent background worker task with `MALLOC_CAP_INTERNAL` (`OTA_TASK_STACK_BYTES = 16384`). Flash write routines execute exclusively on this internal-DRAM thread.

### Rule 3: Mutual Exclusion Gate During OTA

`DisplayServiceHelper` checks `OTAUpdateManager::getState() != OTAState::Idle` prior to dispatching screenshot compression. If an OTA update is in progress, the screenshot route (and MCP `capture_display` tool) immediately returns **HTTP 503 Service Unavailable** ("OTA update in progress"). For direct-upload OTA (httpd task), this is a hard guarantee since both the screenshot route and the upload handler share the single httpd task — they cannot execute concurrently. For scheduled background OTA (running on the `ota_worker` task), the gate prevents overlap with the manifest-fetch and download phases; the practical gap between OTA state transitioning to non-Idle and the actual flash write makes concurrent PSRAM access extremely unlikely but not formally impossible via lock.

> [!IMPORTANT]
> **Standing Architecture Principle for Future Offloading**
> Any future feature offloading deep stack work to PSRAM (e.g. custom image generation, TLS handshake acceleration, heavy cryptography) MUST enforce a mutual-exclusion check against `OTAUpdateManager` state to prevent PSRAM task execution during flash writes.

---

## 5. WebServerManager Configuration API

### Board Contract Defaults

Default HTTP server task settings are defined in [hardware/_common/board_contract.h](../../../../hardware/_common/board_contract.h):

```cpp
#ifndef BOARD_HTTP_STACK_SIZE
#define BOARD_HTTP_STACK_SIZE 8192  // Safe for all standard routes (compression offloaded)
#endif

#ifndef BOARD_HTTP_MAX_OPEN_SOCKETS
#define BOARD_HTTP_MAX_OPEN_SOCKETS 7  // Matches HTTPD_DEFAULT_CONFIG default
#endif
```

### Modern `setHttpConfig()` API

Downstream applications or custom board definitions can override these defaults prior to starting the server:

```cpp
// Call before app.begin() or webServerManager.begin()
app.getWebServerManager().setHttpConfig(8192, 7);
```

### Retiring Legacy Overrides

> [!NOTE]
> Downstream applications no longer need to manually access `getServer().config.stack_size` or `getServer().config.max_open_sockets`. The legacy manual configuration pattern can be **retired** in favor of `setHttpConfig()` and standard `board.h` contract definitions.

---

## 6. See Also

- [DeepStackExecutor Header](../../../../lib/DeepStackExecutor/DeepStackExecutor.h)
- [WebServerManager Header](../../../../modules/WebServer/WebServerManager.hpp)
- [Performance & Memory Guide](../../../performance/overview.md)
- [OTA Security & Lifecycle](../../security/ota-security.md)
