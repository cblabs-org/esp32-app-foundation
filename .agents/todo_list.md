# Project Todo List

> Last updated: 2026-06-05

---

## Architecture

- [ ] **Slim down `FrameworkConfig`** — Currently conflates two different things. Should only hold genuinely app-specific compile-time constants that the framework can't know (`otaPublicKey`, `otaGithubRepo`, `otaGithubToken`). Everything else is a module default masquerading as framework config and should move into each module's own `resetConfig()`:
  - `hostname` → `WiFiManager::resetConfig()`
  - `timezone` → `TimeManager::resetConfig()`
  - `firmwareUpdatesEnabled` → `OTAUpdateManager::resetConfig()`
  - `otaQuietHour` → `OTAUpdateManager::resetConfig()`
  - `mcpReadOnly` → `MCPServer` default
  This removes false coupling between `main.cpp` and module internals.

---

## OTA

- [x] **Quiet hour / scheduled daily check** — Implemented: `tick()` added to `OTAUpdateManager`, daily check fires at `_checkHour` (0–23, default 03:00). Config schema bumped to v3 with migration. Portal Settings card updated with "Daily Check Time" selector.
- [x] **`_autoCheck` boot trigger** — Implemented: first `tick()` after entering Running state fires `performUpdate()` once (`_hasBootChecked` flag). `_otaManager` registered in `_tickables` in `Application.cpp`.
- [ ] **Password-protect private key** — `generate_keys.sh` updated to use `-aes256`. User still needs to run: `openssl pkey -in private_key.pem -aes256 -out private_key_enc.pem && mv private_key_enc.pem private_key.pem`).

---

## Web Portal — UI consistency

- [ ] **Standardise "Advanced" sections** — The OTA Advanced card is correctly yellow and expanded. The Time module Advanced section is collapsed but blue. Rule: all Advanced sections across the portal should use the yellow warning style (`--warning`) AND be collapsed by default. Audit all tabs for Advanced sections and apply consistent treatment.
- [ ] **Screensaver toggle switches always grey** — The on/off toggle switches for screensaver settings appear permanently grey and do not reflect state correctly. Investigate toggle styling and state binding in `display.js` / relevant CSS.

---

## Web Portal — Navigation

- [ ] **Create a 'Display' primary tab** — Move brightness, rotation and screensaver controls out of the current System tab into a new dedicated 'Display' bottom-nav primary tab. Leave clock-size in the App tab. Consider impact on `IWebContributor` tab registration and portal navigation.

---

## Release process

- [ ] **Publish v0.0.5** — This session adds new features (auto-check boot trigger, daily scheduler, check-hour config). Binaries built. Run `python3 scripts/deploy_release.py --tag v0.0.5 --local --private-key private_key.pem` from your own terminal after tagging.
- [ ] **OTA update test** — After v0.0.5 is published, verify end-to-end OTA from portal: Check for Updates → downloads v0.0.5 → ECDSA verification passes → device reboots onto v0.0.5 → portal shows correct version.
