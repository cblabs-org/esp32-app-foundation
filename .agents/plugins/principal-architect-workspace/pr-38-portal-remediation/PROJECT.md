---
name: pr-38-portal-remediation
description: Full PR-35 Deep Review remediation — portal UX (P1–P12), C++ hardening (H1–H3, M1–M4, D4), and framework-expert skill gaps (G1–G5).
status: ACTIVE
parent_pr: pr-35-weather-station
---

# PR-38: PR-35 Deep Review Remediation

> **Source:** PR-35 Deep Review (Parts A–F)  
> **Branch:** `pr-38-portal-remediation` (off `pr-35-weather-station`)  
> **Scope:** All 24 findings — H1–H3, M1–M4, P1–P12, G1–G5, D4

---

## Architectural Overview

PR-35 delivered the Weather Station and Home Assistant modules with functional backends,
but the deep review identified defects across 4 categories: critical portal CSS/JS bugs,
missing information depth, C++ safety violations, and framework-expert skill documentation gaps.
This PR remediates all findings in 5 focused work orders:

| WO | Scope | Findings | Complexity |
|----|-------|----------|------------|
| [WO1](wo1-css-js-fixes/WORKORDER.md) | Critical CSS/JS fixes + lifecycle alignment | P1, P2, P3, P6, P7, P9 | standard |
| [WO2](wo2-information-depth/WORKORDER.md) | Instruction boxes, status badges, eye-btn, toasts | P4, P5 | standard |
| [WO3](wo3-branding-icons/WORKORDER.md) | App name branding + custom tab icons | P10, P11, P12 | standard |
| [WO4](wo4-cpp-hardening/WORKORDER.md) | C++ hardening — std::function, bare new, thread safety, facade, atomics | H1, H2, H3, M1, M2, M3, M4, D4 | architect |
| [WO5](wo5-skill-gap-docs/WORKORDER.md) | Framework-expert documentation enhancements | G1, G2, G3, G4, G5 | mechanical |

> **P8 (weather tab delivery mechanism)** is deferred — the inline C++ raw literal is
> the correct sample-app pattern (matching `MyApp/tab.js`). The inconsistency with
> `homeassistant.js` being in the portal pipeline reflects a legitimate architectural
> difference (framework module vs sample-app module), not a bug.

---

## Work Order Ledger

- **[/] [wo1-css-js-fixes](wo1-css-js-fixes/WORKORDER.md):** Fix undefined CSS classes, remove duplicate JS event listeners, fix `--danger`→`--error`, align lifecycle, wire `apiFetch`, fix hardcoded colours. Flash & verify.
- **[ ] [wo2-information-depth](wo2-information-depth/WORKORDER.md):** Add instruction boxes, MQTT status badge, API key indicator, `.eye-btn` password toggle, consistent toast notifications. Flash & verify.
- **[ ] [wo3-branding-icons](wo3-branding-icons/WORKORDER.md):** Add `appName` to `Application`, wire portal title from `/api/system`, add `getTabIcon()` overrides for HA (house) and Display (monitor) modules. Flash & verify.
- **[ ] [wo4-cpp-hardening](wo4-cpp-hardening/WORKORDER.md):** Eliminate `std::function` from 8 entity types, fix bare `new`, fix `std::string` data race, fix `delay(1)`, replace `setInsecure()`, fix include hygiene, make `_mqttConnected` atomic. Flash & verify.
- **[ ] [wo5-skill-gap-docs](wo5-skill-gap-docs/WORKORDER.md):** Update framework-expert skill: broaden `std::function` prohibition (G1), add JsonListener thread safety warning (G2), add entity callback guidance (G3), flag `setInsecure()` in compliance audit (G4), strengthen include hygiene docs (G5). Documentation only.

---

## Verification Strategy

Each WO includes a mandatory hardware verification step:
1. **Build** via PlatformIO MCP `build_project`
2. **Flash** via PlatformIO MCP `upload_firmware` (preserves config — no filesystem upload)
3. **Serial monitor** — verify clean boot, no JS errors logged
4. **MCP server** — call `system_info` to confirm firmware version
5. **Web portal** — visually verify tabs render correctly, dark mode works, icons display

---

## Dependencies

- Builds on top of `pr-35-weather-station` branch (all WO0–WO6 merged)
- No external dependencies
- Serial device must be connected for hardware verification

---

*Created: 2026-06-21*
