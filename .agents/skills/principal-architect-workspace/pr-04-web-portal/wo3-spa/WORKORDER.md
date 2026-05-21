---
id: pr-04-wo3-spa
title: SPA Frontend
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
Build the Single Page Application (SPA) frontend for the web portal.

## Requirements
1. **Branch**: `pr-04-wo3-spa` (from `pr-04-web-portal`). Create this branch before making changes.
2. **Files**:
   - Work in `web/portal/`.
   - Create `index.html`, `app.css`, `app.js`, `wifi.js`, `system.js`, `ota.js`.
3. **Core Logic**:
   - The frontend must be a hybrid PROGMEM shell + LittleFS app fragments.
   - **CRITICAL**: The SPA MUST serialize API requests. No concurrent XHR or fetch requests are allowed. This protects the ESP32 socket pool.
   - Implement the tabs for wifi, system, ota, and config.
4. **Testing**:
   - Ensure the frontend files are valid and the serialization logic works. (You may use native test frameworks for JS if set up, or just manual code inspection / validation).

## Guidance
- Read `../departures-board/web/index.html` for Portal UX patterns. Do NOT copy wholesale.
- Follow the Web Testing Strategy for any UI portal changes if applicable (view `.agents/workflows/web-testing-strategy.md`).
- Ensure the UX looks clean, responsive, and robust.

## Closure Gate
- **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/skills/principal-architect-workspace/pr-04-web-portal/wo3-spa/` as the target directory. Wait for it to finish before dropping control back to the PA.
