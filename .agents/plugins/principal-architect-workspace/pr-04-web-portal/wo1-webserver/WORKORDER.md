---
id: pr-04-wo1-webserver
title: WebServerManager and IWebContributor Dispatch
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
Implement the `WebServerManager` using ESPAsyncWebServer and the `IWebContributor` dispatch pattern.

## Requirements
1. **Branch**: `pr-04-wo1-webserver` (from `pr-04-web-portal`). Create this branch before making changes.
2. **Files**: 
   - Create `modules/webServer/webServer.hpp` and `modules/webServer/webServer.cpp`.
   - Implement `WebServerManager` which manages an `AsyncWebServer`.
   - Wire into `Application` hub (similar to other kernel services, if applicable, or provide an accessor).
3. **Core Logic**:
   - `AsyncWebServer::setMaxBodySize(4096)` MUST be called during initialization to cap global POST bodies to 4KB.
   - Implement support for the `IWebContributor` interface so that other modules can register their web tabs and API handlers.
   - Setup basic routing for static files (`/`, `/app.css`, `/app.js`, etc.) to be served from PROGMEM (the PROGMEM headers will be created in WO4, so just mock or setup the infrastructure for it now, e.g., stub out serving index.html).
4. **Testing**:
   - Create `test/test_unit/test_webServer/` and add native tests for basic routing, dispatching to a mocked `IWebContributor`, and verifying the POST cap logic (if testable natively).
   - Ensure tests pass locally on native env.

## Guidance
- Read `../departures-board/modules/webServer/` for ESPAsyncWebServer handler patterns. Do NOT copy wholesale, rewrite for the new architecture.
- Follow the Hollywood Principle.
- Conform to House Style Documentation for all new C++ files.

## Closure Gate
- Confirm tests (`test_unit`) compile and pass.
- **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/skills/principal-architect-workspace/pr-04-web-portal/wo1-webserver/` as the target directory. Wait for it to finish before dropping control back to the PA.
