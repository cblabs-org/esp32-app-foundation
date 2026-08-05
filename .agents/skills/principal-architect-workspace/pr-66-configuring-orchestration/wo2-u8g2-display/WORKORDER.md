---
name: pr-66-wo2
description: Handoff instructions for Work Order 2 — U8G2 Display module dispatch + boards
complexity: standard
---

# WO2: U8G2 Display — ConfiguringStatusBoard Upgrade + Modal Dispatch

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Upgrade `ConfiguringStatusBoard` with per-gate status rows and animated progress. Create `GenericGateHelperBoard` for the generic fallback. Add `gateHelperBoard(const AppConfiguringPayload*)` to `IU8G2SystemScreens`. Rewrite `U8G2DisplayModule::_onAppConfiguring()` with three-tier modal dispatch and explicit modal teardown.

## Parent Project

`pr-66-configuring-orchestration` — [PROJECT.md](../PROJECT.md)

## Strict Boundary Box

- ✅ ALLOWED: Files listed in the checklist below (all within `modules/U8G2Display/` and `include/interfaces/IU8G2SystemScreens.hpp`)
- ❌ DO NOT modify `Application.cpp`, `Application.hpp`, `IConfigGate.hpp`, or `AppConfiguringPayload.hpp` (WO1 territory)
- ❌ DO NOT modify any `main.cpp` files (WO4 territory)
- ❌ DO NOT modify LVGL files (WO3 territory)
- ❌ DO NOT modify `WifiManager` files

## Tooling Constraints

- Use PlatformIO MCP tools for building and testing (read `.agents/skills/pio-manager/SKILL.md`)
- Verify compilation with: `pio run -e esp32s3nano`
- Run unit tests with: `pio test -e native --filter "test_unit/test_u8g2Display"`

## Checklist

### 1. `IU8G2SystemScreens.hpp` — Add `gateHelperBoard()`

File: `include/interfaces/IU8G2SystemScreens.hpp`

- [ ] Add `#include "events/AppConfiguringPayload.hpp"`

- [ ] Add virtual method:
  ```cpp
  /// Custom helper modal for a specific config gate.
  ///
  /// Called when a gate's actionStatus is NeedsUserAction. The returned
  /// board pointer must be non-owning — the board must outlive the display
  /// module (typically a static or member object of the consumer's
  /// IU8G2SystemScreens implementation). The same pointer may be returned
  /// for multiple gates if the board supports reconfiguration.
  ///
  /// Return nullptr to use the built-in mapping or generic helpText fallback.
  ///
  /// @param payload Full configuring event payload with gate name, help text,
  ///                action status, hostname, and device IP.
  /// @return Non-owning pointer to a custom board, or nullptr.
  virtual IDisplayBoard* gateHelperBoard(const AppConfiguringPayload* payload) {
      (void)payload;
      return nullptr;
  }
  ```

### 2. `GenericGateHelperBoard` — New Board

Create: `modules/U8G2Display/boards/GenericGateHelperBoard.hpp` and `.cpp`

- [ ] Subclass `MessageBoardBase` (or `IDisplayBoard` if MessageBoardBase is not suitable)
- [ ] `configure(const char* gateName, const char* helpText, const char* deviceIP)` — formats:
  - Title row: gate name (e.g., "Board Configuration")
  - Body: help text (e.g., "Add a departure board")
  - Footer: `"http://<deviceIP>"` — always the IP, never hostname
- [ ] `getBoardName()` returns `"SYS: Gate Helper"`
- [ ] Truncate help text if it exceeds the display width — use ellipsis or word-wrap as appropriate for the U8G2 display size

### 3. `BoardCatalog` — Add GenericGateHelperBoard Member

File: `modules/U8G2Display/BoardCatalog.hpp` (or equivalent board registry)

- [ ] Add `GenericGateHelperBoard genericHelper` as a member, constructed with the display dimensions

### 4. `ConfiguringStatusBoard` — Per-Gate Status Rows + Animated Progress

Files: `modules/U8G2Display/boards/ConfiguringStatusBoard.hpp` and `.cpp`

- [ ] Add `GateActionStatus _activeGateStatus` member and `setActiveGateStatus(GateActionStatus)` setter

- [ ] Replace the current single-gate-name rendering (~L119–143) with per-gate status rows:
  - `*` prefix = satisfied
  - `>` prefix = actionable (current gate)
  - `-` prefix = blocked (dependencies unmet)
  - When actionable gate has `GateActionStatus::InProgress`, consider using `~` or alternating `> / ~` for a simple animation effect

- [ ] Add animated progress bar: lerp `_currentPercent` toward `_targetPercent` each `render()` call (e.g., 2% per frame step). Target = `satisfiedCount / totalGates * 100`.

- [ ] Render `deviceIP` in the footer instead of hostname — use the value from `setConnectionInfo()`

- [ ] Remove the duplicated dependency-checking logic that was previously in this board (now handled by `Application::getActionableGate()` via the payload)

### 5. `U8G2DisplayModule::_onAppConfiguring()` — Three-Tier Dispatch

File: `modules/U8G2Display/U8G2DisplayModule.cpp`

- [ ] Rewrite `_onAppConfiguring()` with the following flow:
  1. Suspend carousel
  2. Show base (consumer override or `configuringStatus`)
  3. **`clearModal()` — always tear down previous modal first**
  4. Parse `AppConfiguringPayload*` from event payload
  5. Update `configuringStatus` with `setActiveGateStatus(p->actionStatus)` and connection info
  6. If `p->actionStatus != GateActionStatus::NeedsUserAction` → return (no modal)
  7. **Tier 1:** `_screens->gateHelperBoard(p)` — consumer override
  8. **Tier 2:** Built-in — `strcmp(p->gateName, GateNames::WIFI_STATION) == 0` → WiFi wizard (configure with `p->hostname` + `p->deviceIP`)
  9. **Tier 3:** Generic fallback — `genericHelper.configure(p->gateName, p->helpText, p->deviceIP)`
  10. Push resolved modal via `showModal()`

### 6. Unit Tests

- [ ] Test `GenericGateHelperBoard::configure()` sets correct content
- [ ] Test `ConfiguringStatusBoard` renders per-gate rows correctly for mixed gate states
- [ ] Test three-tier dispatch in `_onAppConfiguring`: consumer override wins, then built-in, then generic
- [ ] Test `clearModal()` is called on every `_onAppConfiguring` invocation

## Verification

1. `pio test -e native --filter "test_unit/test_u8g2Display"` — all tests pass
2. `pio test -e native` — full suite passes
3. `pio run -e esp32s3nano` — hardware target compiles
4. No warnings

## Completion Protocol

When verified, **STOP and send a message to the PA**. Do NOT distill or commit until approved. Surface any out-of-boundary concerns.
