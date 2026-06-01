---
title: "PR-04a Web Portal Enhancement"
status: "wip"
---
# PR-04a Web Portal Enhancement

## Overview
Migrate proven, framework-generic portal features from the departures-board's mature web portal into the esp32-app-foundation. Builds on top of PR-04 (base SPA) and PR-11 (ConfigManager gateway). Focuses on WiFi scanning UX, system diagnostics, config backup/restore, and UI polish.

## Git Branches
Feature Branch: `pr-04a-portal-enhancement`
Base: `pr-12-state-machine` (contains PR-11 ConfigManager gateway + all prerequisites)

## Work Orders
- [ ] **[WO1](wo1-wifi-api/WORKORDER.md):** WiFi API Redesign & Scanning UX — RESTful endpoints, network scan dropdown, test/forget
- [ ] **[WO2](wo2-system-mgmt/WORKORDER.md):** System Management Hardening — Enhanced /api/system, heap guard, RAG diagnostic bars, restart/reset
- [ ] **WO3:** Config Backup/Restore & OTA Enhancement (depends on WO2)
- [ ] **WO4:** Portal UI Polish & Shared Utilities (depends on WO1 + WO2)

## Parallelism Advisory
WO1 and WO2 are **SAFE to run in parallel** — they touch completely different files:
- WO1: `wifiManager.cpp/hpp`, `wifi.js`
- WO2: `webServer.cpp`, `system.js`

WO3 and WO4 are sequential — WO3 depends on WO2, WO4 depends on both.

## Session
- PA Session: 2510209a-c169-4f4a-a156-c4f94eb8fd5c
- Prior Research: a9c36bb4-efe4-4b53-b51c-7ec5449e3ca8
