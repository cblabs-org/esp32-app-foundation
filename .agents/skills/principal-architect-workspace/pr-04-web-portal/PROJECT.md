---
title: "PR-04 Web Portal"
status: "completed"
---
# PR-04 Web Portal

## Overview
Build the Web Portal for the ESP32 App Foundation framework.
Includes WebServerManager (ESPAsyncWebServer), REST API, SPA frontend, and portalBuilder.py.

## Git Branches
Feature Branch: `pr-04-web-portal`
Base: `main`

## Work Orders
- [x] **WO1**: `WebServerManager` skeleton + `IWebContributor` dispatch. global POST body cap 4096.
- [x] **WO2**: REST API handlers (wifi, system, ota, config, tabs). AP mode 10-minute inactivity timeout.
- [x] **WO3**: SPA frontend (HTML, CSS, JS — all tabs). SPA serialises API requests (no concurrent XHR).
- [x] **WO4**: `portalBuilder.py` + PROGMEM header generation.
