# State — PR-20 end state (branches, config schema, verification)

## FrameworkConfig (`include/FrameworkConfig.hpp`) — FINAL
Exactly 3 compile-time OTA secrets:
```cpp
struct FrameworkConfig {
    const char* otaPublicKey   = nullptr;
    const char* otaGithubRepo  = nullptr;
    const char* otaGithubToken = nullptr;
};
```
Removed: `hostname`, `timezone`, `mcpReadOnly` (WO2a), `firmwareUpdatesEnabled`, `otaQuietHour` (WO2b).

## Where the moved defaults live now
| Was (FrameworkConfig) | Now | Mechanism |
|---|---|---|
| `hostname` | WiFiManager | persisted `currentHostname`; `setDefaultHostname()` (app default, framework fallback `"esp32-device"`); `getHostname()` |
| `timezone` | TimeManager | persisted `_posixTz`; `resetConfig()` → `"GMT0"` |
| `mcpReadOnly` | MCPServer | persisted `readOnly`; `resetConfig()` → `true` |
| `firmwareUpdatesEnabled` | OTAUpdateManager | persisted `updatesEnabled` (default `true`) |
| `otaQuietHour` | — | DELETED (dead) |

## OTA config schema (`/mod/ota.json`) — now v4
Keys: `channel`, `autoCheck`, `updatesEnabled` (v4), `checkHour` (v3), `lastCheckedChannel` (v2). Migrations: v1→v2 `lastCheckedChannel`; v2→v3 `checkHour`; v3→v4 `updatesEnabled`(=true). `updatesEnabled` is the master kill-switch read at `performUpdateTask()`; fully toggle-able via OTA tab (POST `/api/ota` + status + `ota.js`).

## Branch stack (all pushed to origin; main NOT pushed; NO PRs)
```
origin/main = 67a53cd   (local main = a377718, AHEAD by 6 — unpushed, user holding)
local main a377718
 └─ pr-20-config-refactor/wo1-appconfig-clarity   07e41f7  (FINDINGS + planning)
     └─ pr-20-config-refactor/wo2a-persisted-moves ca4801f
         └─ pr-20-config-refactor/wo2b-ota-new-persistence 5b7f313  ← top of stack
```
The 6 unpushed `main` commits: `dba23bf`,`9687286`,`3eada29` (harness/skill + PR-20 planning) and `f437a92`,`9f2b3cd`,`a377718` (OTA native-build fix).

## Verification
- Native: **525/525 PASSED, 0 ERRORED** (was 519 pre-fix; +3 WO2a, +3 WO2b new tests; 522→525).
- `esp32dev` device build: **SUCCESS** (~47s), portal asset regenerated, web-test hook passed.
- Device image flash usage ~89.5% (pre-existing; `pr-10-firmware-optimization` queued separately).

## Tooling reality
- `gh` authed (matthewmcneill); **no `gt`, no `gh-stack`** extension.
- Submission via plain `gh` only would need manual stacked-PR creation (wo1→main, wo2a→wo1, wo2b→wo2a) + manual rebase cascade on merge.
