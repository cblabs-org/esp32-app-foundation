---
name: pr-38-portal-remediation
description: Portal UX remediation against PR-35 Deep Review findings (P1–P12). Fixes critical CSS/JS bugs, adds information depth, and implements branding/icon support.
status: ACTIVE
parent_pr: pr-35-weather-station
---

# PR-38: Portal UX Remediation

> **Source:** [PR-35 Deep Review — Part F](../../../../../../.gemini/antigravity/brain/67356e63-7c1b-4480-a363-12480e1acb25/pr35_deep_review.md)  
> **Branch:** `pr-38-portal-remediation` (off `pr-35-weather-station`)  
> **Scope:** Findings P1–P12 from the portal/styling deep review

---

## Architectural Overview

PR-35 delivered the Weather Station and Home Assistant modules with functional backends,
but the portal tabs have critical CSS, JS, and UX deficiencies identified in the deep review.
This PR remediates all 12 portal findings in 3 focused work orders:

| WO | Scope | Findings | Complexity |
|----|-------|----------|------------|
| [WO1](wo1-css-js-fixes/WORKORDER.md) | Critical CSS/JS fixes + lifecycle alignment | P1, P2, P3, P6, P7, P9 | standard |
| [WO2](wo2-information-depth/WORKORDER.md) | Instruction boxes, status badges, eye-btn, toasts | P4, P5 | standard |
| [WO3](wo3-branding-icons/WORKORDER.md) | App name branding + custom tab icons | P10, P11, P12 | standard |

> **P8 (weather tab delivery mechanism)** is deferred — the inline C++ raw literal is
> the correct sample-app pattern (matching `MyApp/tab.js`). The inconsistency with
> `homeassistant.js` being in the portal pipeline reflects a legitimate architectural
> difference (framework module vs sample-app module), not a bug.

---

## Work Order Ledger

- **[ ] [wo1-css-js-fixes](wo1-css-js-fixes/WORKORDER.md):** Fix undefined CSS classes, remove duplicate JS event listeners, fix `--danger`→`--error`, align lifecycle, wire `apiFetch`, fix hardcoded colours. Flash & verify.
- **[ ] [wo2-information-depth](wo2-information-depth/WORKORDER.md):** Add instruction boxes, MQTT status badge, API key indicator, `.eye-btn` password toggle, consistent toast notifications. Flash & verify.
- **[ ] [wo3-branding-icons](wo3-branding-icons/WORKORDER.md):** Add `appName` to `FrameworkConfig`, wire portal title from `/api/system`, add `getTabIcon()` overrides for HA (house) and Display (monitor) modules. Flash & verify.

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
