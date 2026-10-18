---
name: lvgl-display-wo10
description: Handoff instructions for Work Order 10 — on-device hardware bring-up & verification
complexity: architect
---

# lvgl-display / WO10 — Hardware Bring-Up & Verification (on device)

## Provenance & Authorization
- **Parent project/PR:** PR-47 — [`../PROJECT.md`](../PROJECT.md)
- **Base branch:** `lvgl-display/wo9-crowpanel-adapter`
- **Your branch:** `lvgl-display/wo10-hardware-bringup` (created for you — **do not branch**)
- **Stack position:** WO10 of 11. **Requires physical CrowPanel 1.28" hardware — human-in-the-loop.**
- **Authorization:** WORKORDER + `.agents/rules/`, valid once PR-47 `PROJECT.md` records the human's decomposition approval. Verify; if absent, **stop and report**.

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below — **and hardware flashing must follow the version flash-gate and be coordinated with the human.**

## Objective
Bring up the LVGL module + CrowPanel128Adapter on real hardware: verify display, touch, encoder, backlight, frame rate, and memory headroom, and reconcile the GPIO map against the actual PCB.

## Context You Need
- Design §4.2 (GPIO map — **revision-dependent**, OQ-1: verify against the physical board; touch INT may be 0 or 5; encoder 45/42/41), §5.4 (memory budget: ~65 KB free at peak TLS+LVGL target), §8.3 (FPS budget: 50–60 at 80 MHz SPI).
- **FLASH GATE — `.agents/rules/03-versioning.md`:** before flashing firmware with new functionality, you MUST: check the highest tag (`git describe --tags --abbrev=0`), create+push the next `dev` tag (`vX.Y.Z-dev.(N+1)`), **rebuild** so `inject_version.py` bakes it in, then flash. Dev tags are autonomous (no human approval needed) but flashing is a hardware action — coordinate with the human.
- Use the `device-mcp-client` skill to query the running device (`system_info`, `display_status`, `capture_display`) after boot.

## Strict Boundary Box — do NOT:
- Make architectural changes. If bring-up reveals a code defect, make the **minimal** pin/config-flag or adapter fix within WO9/WO1's scope and report it; anything larger → **stop and report** to the PA.
- Create beta/stable tags (dev tags only, per rule 03). Never run `deploy_release.py`.
- Change the partition scheme (deferred upstream — issue #40).

## Tooling Constraints
`platformio` MCP tools for build/flash/monitor (`build_project`, `upload_firmware`, and the monitor/verify tooling) via the `pio-manager` skill — never raw `pio`/`esptool`. Use `device-mcp-client` for device queries. Coordinate the physical flash with the human.

## Line-Item Instructions
- [ ] Verify the GPIO map against the actual PCB revision (OQ-1); correct any pin build flags in `platformio.ini` if they differ (document the revision).
- [ ] Tag + push the next `dev` version, rebuild, then flash `env:crowpanel-128-rotary` (flash gate).
- [ ] Verify on device: display renders (colours correct with `invert_color`), touch coordinates track, encoder rotation direction + step fidelity (PCNT), push-button, backlight PWM sweep, boot splash appears immediately (Stage 1).
- [ ] Measure frame rate (target 50–60 FPS) and free heap under load (`curl /api/system` / `system_info`); confirm ~65 KB headroom target at TLS+LVGL peak (§5.4). Record numbers.
- [ ] Verify `capture_display` MCP tool + portal screenshot against the live screen.
- [ ] `curl /api/system` (or `system_info`) — confirm `firmware_version` matches the new dev tag (flash-gate post-check).

## Verification Requirement
- [ ] All bring-up checks pass on hardware; numbers (FPS, heap, firmware size) recorded in the completion report.
- [ ] `run_tests` (native) still green; `build_project` `env:esp32s3nano` still green.

## Completion Protocol
1. Validate on hardware, then **STOP and message the Principal Architect** with the full bring-up results + measured numbers + PCB revision. Surface any out-of-boundary defects; do not file issues.
2. **Do not distill until the human approves.**
3. On approval: `@distillery` with target `.agents/skills/principal-architect-workspace/pr-47-lvgl-display/wo10-hardware-bringup/`.
4. Append a status update to this `WORKORDER.md` and terminate.
