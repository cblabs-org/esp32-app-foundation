---
name: pr-225-ble-manager-wo6-dis
description: Handoff instructions for built-in BLE Device Information Service (DIS) implementation
complexity: standard
---

# Work Order — Built-in BLE Device Information Service (DIS)

## Role Definition
> You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Implement a built-in `IBLEServiceProvider` that exposes a standard Bluetooth SIG Device Information Service (UUID 0x180A) with 6 read-only characteristics. The DIS auto-registers inside `BLEManager::begin()` before consumer providers, so every BLE-enabled device has device identity discoverable via GATT.

## Parent Project
PR #225 (BLE Manager), branch `ble-manager/wo6-validation`.
Human approval of this work order was given explicitly in the current session.

## Strict Boundary Box
- **IN SCOPE:** `modules/BLEManager/`, `modules/Application/Application.hpp`, `include/framework_version.h` (new), `docs/developer/bluetooth.md`, `.agents/skills/framework-expert/SKILL.md`
- **OUT OF SCOPE:** All other modules, `platformio.ini`, any `sdkconfig*` files, `src/main.cpp` (DIS auto-registers — no consumer wiring needed), `sample-app/`, any web portal files.
- **DO NOT** modify `IBLEServiceProvider.hpp` (the interface is sufficient as-is).
- **DO NOT** modify any `.ini`, `.py`, or build system files.
- **DO NOT** add DIS UUID to advertising (DIS is discovered after connection, not advertised).

## Tooling Constraints
- Read `.agents/rules/` files first (mandatory).
- Read `.agents/skills/house-style-docs/SKILL.md` and its `references/cpp-embedded.md` for documentation and naming standards.
- Use PlatformIO MCP tools (`build_project`) for compilation verification — do NOT use raw `pio` CLI commands.
- Tag before flash: use `v0.0.12-dev.66` per rule 03.
- Use PIO MCP `upload_firmware` for flashing (never `esptool`).

## DIS Field Mapping (approved by user)

| Characteristic | UUID | Value Source |
|---------------|------|-------------|
| Manufacturer Name | 0x2A29 | `app.getManufacturer()` — defaults to `"CB Labs"` |
| Model Number | 0x2A24 | `app.getAppName()` — defaults to `"ESP32 App Foundation"` |
| Serial Number | 0x2A25 | Last 3 bytes of WiFi MAC as `"XX:YY:ZZ"` |
| Firmware Revision | 0x2A26 | `BUILD_VERSION` (consumer's app version from git tags) |
| Hardware Revision | 0x2A27 | `ESP.getChipModel()` e.g. `"ESP32-S3"` |
| Software Revision | 0x2A28 | `"App Foundation " + FRAMEWORK_VERSION` |

## Line-Item Instructions

- [ ] **Create `include/framework_version.h`** — New file. Single `#define FRAMEWORK_VERSION "0.0.12"`. Add house-style header block. This is the framework's own version, independent of consumer `BUILD_VERSION`.

- [ ] **Modify `modules/Application/Application.hpp`** — Add manufacturer identity API:
  - Add member: `const char* _manufacturer = nullptr;`
  - Add setter: `void setManufacturer(const char* name) { _manufacturer = name; }`
  - Add getter: `const char* getManufacturer() const { return _manufacturer ? _manufacturer : "CB Labs"; }`
  - Place near existing `setAppName()`/`getAppName()` (around line 168-171).

- [ ] **Create `modules/BLEManager/DeviceInfoServiceProvider.hpp`** — New `IBLEServiceProvider` implementation:
  - Constructor takes 6 `const char*` params (manufacturer, model, serial, firmwareRev, hardwareRev, softwareRev). All must have static or long-lived lifetime.
  - `getProviderId()` returns `"dis"`.
  - `registerServices(NimBLEServer& server)`: Creates service UUID 0x180A, adds 6 read-only `NimBLECharacteristic`s with appropriate UUIDs, sets their values from constructor params. Returns true.
  - No `contributeAdvertising()` override (DIS is not advertised).
  - No notification support — all characteristics are read-only.
  - Include house-style file header.
  - Use `#include <NimBLEService.h>` and `#include <NimBLECharacteristic.h>`.
  - NimBLE characteristic property: `NIMBLE_PROPERTY::READ`.

- [ ] **Modify `modules/BLEManager/BLEManager.hpp`** — Add DIS provider member:
  - `#include "DeviceInfoServiceProvider.hpp"`
  - Add members in the private section:
    ```cpp
    std::unique_ptr<DeviceInfoServiceProvider> _disProvider;
    char _disSerial[12];
    char _disSoftwareRev[40];
    ```

- [ ] **Modify `modules/BLEManager/BLEManager.cpp`** — Auto-register DIS in `begin()`:
  - `#include "framework_version.h"` at top
  - In `begin(Application& app)`, BEFORE the existing body, add:
    ```cpp
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(_disSerial, sizeof(_disSerial), "%02X:%02X:%02X", mac[3], mac[4], mac[5]);
    snprintf(_disSoftwareRev, sizeof(_disSoftwareRev), "App Foundation %s", FRAMEWORK_VERSION);

    _disProvider = std::make_unique<DeviceInfoServiceProvider>(
        app.getManufacturer(),
        app.getAppName(),
        _disSerial,
        BUILD_VERSION,
        ESP.getChipModel(),
        _disSoftwareRev
    );
    registerServiceProvider(_disProvider.get());
    ```
  - Include `<esp_mac.h>` for `esp_read_mac`.

- [ ] **Modify `docs/developer/bluetooth.md`** — Add DIS documentation:
  - New subsection under §3 (Peripheral Role) or as §3.5: "Built-in Device Information Service"
  - Document: the 6 fields, where values come from, that it's automatic, how consumers set manufacturer via `app.setManufacturer()`.

- [ ] **Modify `.agents/skills/framework-expert/SKILL.md`** — Add quick answer:
  - "What BLE services does the framework provide?" → DIS (0x180A) auto-registers with device identity from `Application` APIs.

- [ ] **Build verification** — Use PIO MCP `build_project` for environment `esp32s3nano_ble`. Must compile with zero errors and no new warnings.

- [ ] **Tag and flash** — Tag `v0.0.12-dev.66`, push tag, flash to device using PIO MCP `upload_firmware`.

- [ ] **Verify on device** — After flashing, use serial monitor to confirm boot with no crashes. Report `dram` and `dram_min` values from the health endpoint. Confirm DIS appears in BLE GATT table (if nRF Connect is not available, check serial logs for GATT service registration).

## Verification Requirement
The worker must verify the build compiles cleanly via PIO MCP tools. Flash to the attached device and confirm stability. Report DRAM values to confirm DIS adds negligible memory overhead.

## Issue Surfacing
If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it in your Completion Report below — do not file issues directly. The Principal Architect will triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
(pending)

### Decisions Made
(pending)

### Dead Ends & Warnings
(pending)

### Out-of-Scope Findings
(pending)

### Blockers
(pending)
