---
title: "PR-36 Manifest-First OTA: Integration Testing Continuation"
distilled_at: "2026-06-20T17:18:00Z"
original_plan_id: "pr-36-manifest-first-ota"
artifacts:
  - context_artifacts/adr_two_stream_releases.md
  - context_artifacts/graveyard_ota_testing.md
  - context_artifacts/state_current.md
---

# Context Bridge: OTA Integration Testing

PR-36 (manifest-first OTA) firmware implementation is complete (WO1-WO6). The deploy script has been refined with tag auto-detection, two-stream releases (binaries vs manifest), and the `manifest-latest` floating tag. Server-side verification passed (manifest sig, firmware sig, SHA256, size all verified OK). **Integration testing is blocked by a missing PAT on the device.**

## Current Blocker

The device gets **HTTP 404** fetching `manifest-latest` from the private GitHub repo because `OTA_GITHUB_TOKEN` is not defined as a build flag. Fix this, re-flash, and OTA should work.

## Next Steps (exact sequence)

1. **Configure the PAT for builds.** Read `.agents/skills/framework-expert/security/pat-management.md` for the canonical approach. The `main.cpp` already has `#ifdef OTA_GITHUB_TOKEN` / `config.otaToken = OTA_GITHUB_TOKEN` — the macro just needs to be defined as a build flag.

2. **Rebuild and re-flash the baseline** (device is on `v0.0.12-dev.2`, manifest has `v0.0.12-dev.3`):
   ```
   MCP: build_project  (environment: esp32dev)
   MCP: upload_firmware (environment: esp32dev, projectDir: /Users/mcneillm/Documents/Projects/esp32-app-foundation)
   ```

3. **Start the serial monitor and trigger OTA:**
   ```
   MCP: start_monitor (port: /dev/cu.usbserial-5AA70033491)
   ```
   Then either wait for boot auto-check or call:
   ```bash
   curl -sk https://<device-ip>/api/mcp -H 'Content-Type: application/json' \
     -d '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"ota_check_now","arguments":{}}}'
   ```

4. **Watch the serial log for the manifest-first flow:**
   - Gate 1: Manifest signature verified
   - Gate 2: Firmware signature verified
   - Gate 3: Binary version header checked
   - OTA flash + reboot

5. **Post-OTA verification:**
   - Device MCP `system_info` → `firmware_version` should be `v0.0.12-dev.3`
   - Re-trigger OTA → should report "no update available"
   - Check `/mod/ota.json` for learned manifest URL

6. **Negative tests** (if time permits):
   - Tamper with `manifest.json` on GitHub → device should reject (sig mismatch)
   - Delete `manifest.json.sig` → device should fail-closed

## Critical Agent Rules

- **NEVER execute `deploy_release.py`** via terminal tools. Always give the user the CLI command to run. See `deployment.md` line 38.
- **Always use PIO MCP** for build/flash/monitor — never raw `pio` CLI.
- **`unset GITHUB_TOKEN`** before any `gh` CLI commands (the env var has an expired token).

## Key Files

| File | Purpose |
|------|---------|
| `scripts/deploy_release.py` | Build → sign → deploy pipeline (user runs manually) |
| `scripts/generate_manifest.py` | Manifest generation with `--merge-into` support |
| `src/main.cpp:47` | Compiled-in manifest URL (`manifest-latest`) |
| `src/main.cpp:50-52` | `OTA_GITHUB_TOKEN` ifdef guard |
| `modules/OTAUpdateManager/OTAUpdateManager.cpp` | `fetchAndVerifyManifest_()` — manifest-first flow |
| `lib/OTAManifest/OTAManifest.hpp` | v2 manifest struct |
| `.agents/skills/framework-expert/security/pat-management.md` | PAT setup guide |
| `.agents/skills/framework-expert/systems/ota/deployment.md` | Deploy workflow + agent rules |

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_two_stream_releases.md` — Architecture decision: why binary and manifest releases are separated
- `context_artifacts/graveyard_ota_testing.md` — Six specific failures from this session (deploy script execution, wrong URL, 404, wrong tag, extra passphrase, gh auth)
- `context_artifacts/state_current.md` — Exact git state, GitHub releases, manifest content, device state
