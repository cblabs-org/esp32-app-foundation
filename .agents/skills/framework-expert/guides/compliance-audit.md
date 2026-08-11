# Compliance Audit Guide

When auditing the codebase for compliance, the principles in this skill interact — a finding in one domain often implies a finding in another. This guide helps you systematically cover the full surface area.

## Audit Scope

Scan **all** of these directories — not just top-level modules:

| Directory | What to Check |
|---|---|
| `modules/*/` | All module headers and source files |
| `modules/*/boards/` | Board implementations AND sub-interfaces (e.g., `IBoardLayout`) |
| `lib/*/` | Library headers and source files |
| `include/interfaces/` | Interface definitions |
| `include/AppFoundation.h` | Facade completeness (should include all 9 interfaces) |
| `include/FrameworkConfig.hpp` | Immutability, scope |
| `src/main.cpp` | Entry point, global scope patterns |

## Cross-Referencing Checklist

These are the principle combinations that catch the most violations. When you find one issue, always check for the related issue:

### Memory × Thread Safety
- **When you find a `WiFiClientSecure`** → check the calling task's stack size. Stack allocation requires ≥12KB. Also check whether the task does flash writes (PSRAM/SPI constraint).
- **When you find a `new` without `std::nothrow`** → check ALL other `new` calls in the same module. Bare `new` tends to cluster.
- **When you find `std::function`** → check if it's invoked in a hot path (`tick()`, callbacks). SBO threshold varies by platform.

### Security × Performance
- **When you find `setInsecure()`** → ask: is the payload independently verified (signed firmware = OK) or is the transport the only integrity layer (API calls = NOT OK)?
- **When you find TLS usage** → check the task stack size AND whether concurrent connections are bounded (budget: 2 max).
- **When you find credential handling** → verify it goes through `KeyManager`, not raw NVS or plaintext LittleFS.

| TLS Security | `setInsecure()` used in reference/sample application connecting to a public API. Reference apps must demonstrate proper TLS verification — use `esp_crt_bundle_attach()` for public APIs or `KeyManager` for user-provided CA certs. `setInsecure()` is only acceptable for local network connections (e.g., self-signed MQTT broker on LAN) with an explicit comment justifying the choice. |

### Naming × Consistency
- **When you find one naming violation** → scan ALL members of that module. Naming violations cluster — a module with one `suffix_` member usually has many.
- **When you find a TAG definition** → check if it uses `constexpr` and matches the class name.

### Architecture × DI
- **When you find `Application*` as a member** → this is the Orchestrator Anti-Pattern. Check what specific services it accesses and whether narrow DI would suffice.
- **When you find a sub-interface** → check whether it also takes `Application*`. Sub-interfaces should take even narrower dependencies than their parent.

### Registration × Lifecycle
- **When you find runtime `push_back`** → check if the container is subject to compact-and-lock. Post-lock pushes cause heap reallocation during steady-state.
- **When you find a FreeRTOS task** → check whether it registers with the Task Watchdog Timer (TWDT).

## Common Blind Spots

Things that audits consistently miss:

1. **Sub-interfaces in `boards/interfaces/`** — these are not top-level modules but still contain DI patterns
2. **Committed secrets** — check the repo root for `.pem`, `.key`, `.env` files that shouldn't be tracked
3. **Facade completeness** — `AppFoundation.h` should include ALL documented interfaces
4. **Test stubs** — Code in `test/stubs/platform/` may use mock abstractions but must still comply with naming conventions
5. **Task stack sizes** — the default 4-8KB is often insufficient for TLS; verify with `uxTaskGetStackHighWaterMark()`
