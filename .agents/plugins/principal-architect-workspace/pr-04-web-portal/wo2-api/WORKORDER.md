---
id: pr-04-wo2-api
title: REST API
status: ready
---

## MANDATORY SKILL BOOT SEQUENCE — Execute FIRST

Before reading any code or writing any file, run these exact view_file calls in order:
1. view_file `.agents/skills/pio-manager/SKILL.md`
2. view_file `.agents/skills/embedded-systems/SKILL.md`
3. view_file `.agents/skills/house-style-docs/SKILL.md`
4. view_file `.agents/skills/oo-expert/SKILL.md`
5. view_file `.agents/skills/pio-manager/references/mcp-agent-reference.md`

Do NOT proceed past this block until all five are confirmed loaded.

---

## Objective
Implement the REST API handlers for the ESP32 App Foundation web portal.

## Requirements
1. **Branch**: `pr-04-wo2-api` (from `pr-04-web-portal`). Create this branch before making changes.
2. **Handlers**:
   - `GET /api/tabs`: Returns the list of registered tabs from `IWebContributor`s.
   - `GET /POST /api/wifi`: Manage WiFi credentials and scanning.
   - `GET /api/system`: System health and status.
   - `GET /POST /api/ota`: Manual OTA triggers.
   - `GET /POST /api/config`: View and update system/module config.
   - Use `StaticJsonDocument<N>` in handlers (No `DynamicJsonDocument`).
3. **AP Timeout**:
   - Implement an AP mode 10-minute inactivity timeout. If the ESP32 is in AP mode and no requests are received for 10 minutes, transition the state or shutdown the AP.
4. **Testing**:
   - Update `test_webServer` to cover the new endpoints.
   - Create hardware tests if necessary to verify AP timeout or config round-trip.

## Guidance
- Read `../departures-board/modules/webServer/` for ESPAsyncWebServer handler patterns. Do NOT copy wholesale.
- Conform to House Style Documentation for all new C++ files.

## Closure Gate
- Confirm tests (`test_unit` and hardware tests if any) compile and pass.
- **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/skills/principal-architect-workspace/pr-04-web-portal/wo2-api/` as the target directory. Wait for it to finish before dropping control back to the PA.
