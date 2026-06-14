---
name: pr-24-portal-mcp-auth-wo9
description: Handoff instructions for Work order 9 (PsychicHttp Migration)
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Complete the migration of the ESP32 App Foundation from `ESPAsyncWebServer` to `hoeken/PsychicHttp`. The Principal Architect has already branched, added the dependency, configured the ESP-IDF WebSockets flag, and updated the `IWebContributor` interface and `WebServerManager` mocks/static routes. You must finish migrating the dynamic API routes across all modules and ensure tests pass.

# Strict Boundary Box
- Do NOT modify `platformio.ini` or `sdkconfig.defaults` (already done).
- Do NOT modify the static file routes (app.js, index.html, etc.) in `WebServerManager.cpp` (already done).
- Do NOT introduce new REST endpoints; strictly adapt the existing ones.
- When working with JSON responses, ensure you return `ESP_OK` using `request->reply(200, "application/json", json_string.c_str())` or `response->send(...)`.

# Checklist

- `[ ]` **WebServerManager API Routes:** In `WebServerManager.cpp`, update the lambdas for `/api/system`, `/api/app`, `/api/tabs`, `/api/system/restart`, and `/api/system/factory-reset` to the `PsychicHttpRequestCallback` signature: `[](PsychicRequest* request, PsychicResponse* response)`. Replace `request->send(...)` with `response->send(...)`.
- `[ ]` **WebServerManager Captive Portal Redirect:** Implement the captive portal `onNotFound` handler in `WebServerManager.cpp` to redirect to port 443 if a password/cert is active, otherwise to port 80.
- `[ ]` **AuthManager Routes:** Update `/api/auth/status`, `/api/auth/login`, `/api/auth/logout`, `/api/auth/password` to use `PsychicHttp` signatures.
- `[ ]` **WiFiManager Routes:** Update `/api/wifi/status`, `/api/wifi/scan`, `/api/wifi/connect`, `/api/wifi/forget`. Use the `AUTH_PORTAL_GUARD` macro correctly if already present.
- `[ ]` **OTAUpdateManager Routes:** Update `/api/ota/status`, `/api/ota/action`.
- `[ ]` **ConfigBackupRestore Routes:** Update `/api/backup/export`, `/api/backup/import`, `/api/backup/restore-default`.
- `[ ]` **DataManager Routes:** Update `/api/keys`, `/api/keys/*` handlers.
- `[ ]` **TimeManager Routes:** Update `/api/time`.
- `[ ]` **U8g2DisplayModule Routes:** Update `/api/display/status`, `/api/display/settings`, `/api/display/screenshot`.
- `[ ]` **MCPServer Route:** Update the JSON-RPC handler at `/api/mcp` and `/api/mcp/schema`.
- `[ ]` **Verification:** Run native tests (`pio test -e native`) and verify they pass. Wait to compile for the target until native tests are green. Run `pio run -e esp32s3nano` to verify target compilation.

# Verification Requirement
You must verify your changes compile and tests run successfully.
**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.
