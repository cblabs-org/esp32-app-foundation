---
title: "PR-02: WiFi Integration & Captive Portal — Session Distillation"
distilled_at: "2026-05-26T14:43:00Z"
original_plan_id: "PR-02-wifi-integration"
artifacts:
  - context_artifacts/adr_wifi_state_machine.md
  - context_artifacts/adr_webserver_deferred_start.md
  - context_artifacts/adr_captive_portal.md
  - context_artifacts/graveyard_native_build_stubs.md
  - context_artifacts/state_files_changed.md
---

# Executive Summary

PR-02 is **complete and verified on hardware**. The `WifiManager` is now a first-class member of `Application`, wired into the boot lifecycle with EventBus-driven coordination to the `WebServerManager`. On fresh flash (no credentials), the device starts an AP captive portal with DNS hijack and HTTP redirect endpoints for Android/iOS/Windows/Firefox. The AP stays up indefinitely when no credentials exist. When credentials are saved via the portal, `esp_restart()` reboots into STA mode. Flap detection (max 3 retries) prevents rapid STA↔AP cycling.

# Next Steps

The web portal itself needs significant work. The foundation has a basic portal (PROGMEM gzipped HTML/CSS/JS) but it lacks the departures-board's richer functionality:

1. **Assess the departures-board web portal** — compare `test/web/` and the portal assets to understand what WiFi configuration UI, network scanning, and system management features exist.
2. **WiFi network scanning** — the departures-board likely has a `/api/wifi/scan` endpoint that scans for available networks and presents them in a dropdown. This is critical UX for the captive portal.
3. **Portal asset pipeline** — understand how the departures-board builds/compresses portal assets (gzip PROGMEM) and whether the foundation's pipeline matches.
4. **System configuration tabs** — the foundation's portal currently has tabs contributed by `IWebContributor` modules. Evaluate what tabs the departures-board has (WiFi, System, OTA, etc.) and which should be in the foundation.
5. **WiFi status display** — real-time WiFi status (connected SSID, signal strength, IP address) on the portal.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_wifi_state_machine.md` - Architecture decisions for the WiFi state machine: states, transitions, flap detection, AP timeout guards
- `context_artifacts/adr_webserver_deferred_start.md` - Why WebServer start is deferred via EventBus and how startServer() enables AP+STA rebinding
- `context_artifacts/adr_captive_portal.md` - Captive portal detection endpoints and onNotFound catch-all for triggering phone popups
- `context_artifacts/graveyard_native_build_stubs.md` - String stub redefinition wars and EventBus callback signature mismatch
- `context_artifacts/state_files_changed.md` - Complete file manifest with line-level change descriptions
