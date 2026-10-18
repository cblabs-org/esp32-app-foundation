---
name: pr28-wo8
description: Handoff instructions for Work Order 8 — naming convention normalization
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

# Objective

Normalise all member variable naming to the framework's leading `_` prefix
convention in the two modules that violate it:
1. `OTAUpdateManager` — trailing underscore (`state_`, `app_`) → leading (`_state`)
2. `WifiManager` — no prefix (`wifiSsid`, `isAPMode`) → leading (`_wifiSsid`)

# Strict Boundary Box

- **DO** modify OTAUpdateManager (hpp + cpp) and WifiManager (hpp + cpp).
- **DO NOT** change any logic, APIs, or behaviour.
- **DO NOT** touch TAG definitions, `String` usage, or other code.
- This is a **pure rename** operation — find-and-replace within each module.

# Checklist

## A. OTAUpdateManager — Trailing `_` → Leading `_`

- [ ] **`modules/OTAUpdateManager/OTAUpdateManager.hpp`** — Rename all trailing-
  underscore private members to leading underscore. After WO7, some members
  may already use leading `_` (the new narrow DI pointers). Only rename the
  legacy members:

  | Current (trailing `_`) | Target (leading `_`) |
  |------------------------|---------------------|
  | `expectedSize_` | `_expectedSize` |
  | `uploadSigBuf_` | `_uploadSigBuf` |
  | `uploadSigLen_` | `_uploadSigLen` |
  | `uploadBinSize_` | `_uploadBinSize` |
  | `uploadAborted_` | `_uploadAborted` |
  | `uploadVerifier_` | `_uploadVerifier` |
  | `state_` | `_state` |
  | `lastOtaError_` | `_lastOtaError` |
  | `lastProgress_` | `_lastProgress` |
  | `taskHandle_` | `_taskHandle` |

  Note: `app_` will have been removed by WO7. If any other trailing-underscore
  members exist that weren't listed, rename them too.

- [ ] **`modules/OTAUpdateManager/OTAUpdateManager.cpp`** — Update every
  reference to the renamed members. Use find-and-replace carefully — some names
  like `state_` may appear in string literals or comments (don't rename those).

## B. WifiManager — No Prefix → Leading `_`

- [ ] **`modules/WifiManager/WifiManager.hpp`** — Rename all unprefixed private
  members:

  | Current (no prefix) | Target (leading `_`) |
  |---------------------|---------------------|
  | `wifiSsid` | `_wifiSsid` |
  | `wifiPass` | `_wifiPass` |
  | `currentHostname` | `_currentHostname` |
  | `isAPMode` | `_isAPMode` |
  | `currentState` | `_currentState` |
  | `stateTimer` | `_stateTimer` |
  | `connectionRetries` | `_connectionRetries` |
  | `wifiDisconnectTimer` | `_wifiDisconnectTimer` |
  | `myUrl` | `_myUrl` |
  | `lastApiRequestTime` | `_lastApiRequestTime` |
  | `dnsServer` | `_dnsServer` |

  Note: Members already using `_` prefix (`_reconnectRequested`,
  `_restartRequested`, `_configManager`, `_keyManager`, `_defaultHostname`)
  are already correct — **do not touch them**.

- [ ] **`modules/WifiManager/WifiManager.cpp`** — Update every reference to the
  renamed members. Be careful with:
  - String literals in ESP_LOG calls (don't rename)
  - Local variables with similar names (don't rename)
  - Method parameters (don't rename)

# Important Notes

- Use your editor's find-and-replace with **whole word matching** to avoid
  partial renames (e.g., don't rename `stateTimer` inside `lastApiRequestTime`).
- After renaming, grep for the old names to verify zero remaining references:
  ```
  grep -rn "expectedSize_\|uploadSigBuf_\|state_\|lastOtaError_" modules/OTAUpdateManager/
  grep -rn "\bwifiSsid\b\|bisAPMode\b\|currentState\b" modules/WifiManager/
  ```
- Test compilation immediately after each module's rename to catch typos early.

# Verification

Use the PlatformIO MCP server tools (not the CLI directly):
1. `build_project` with `environment: "native"` — compiles cleanly
2. `build_project` with `environment: "esp32dev"` — compiles cleanly
3. `run_tests` with `environment: "native"` — all tests pass

If any build fails, use `agent_build_diagnose` for structured error analysis.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
