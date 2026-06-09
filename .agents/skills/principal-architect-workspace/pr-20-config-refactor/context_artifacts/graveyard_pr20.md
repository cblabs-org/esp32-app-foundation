# Graveyard — PR-20 dead-ends, gotchas, and traps for future agents

## 1. `otaQuietHour` is a DEAD field — don't "move" it
The bridge prompt said "move `otaQuietHour` → OTAUpdateManager::resetConfig()". **There was nothing to move.** Zero code readers anywhere (`grep` confirmed). It was a designed-but-never-implemented "suppress checks at hour X" feature; the real, already-persisted `checkHour` covers OTA scheduling. → **Deleted** from FrameworkConfig + 3 stale doc refs corrected. Lesson: validate the bridge's premise against the code before authoring the WORKORDER.

## 2. Pre-existing native build breakage blocked the baseline (NOT introduced by PR-20)
`pio test -e native` was RED before WO2 started: 4 suites ERRORED (`test_application`, `test_mcpServer`, `test_otaUpdateManager`, `test_u8g2Display`) — all from one root cause in `OTAUpdateManager`, introduced by **merged commit `505b9da`**:
```
OTAUpdateManager.hpp:426: error: unknown type name 'String'   (Arduino String in a native-compiled header)
OTAUpdateManager.cpp:926: out-of-line definition of 'handleSignedUpload' does not match any declaration  (cascade of the String error)
```
Fixed on `main` (separate agent): `f437a92`, `9f2b3cd`, `a377718`. The WO1 branch was then **rebased onto green main**. Lesson: always establish a green baseline before a refactor whose gate is "all tests pass" — otherwise you can't distinguish your regressions from pre-existing red.

## 3. Subagents soft-refuse writing report `.md` files (Claude Code)
A worker told (only in its WORKORDER) to write `FINDINGS.md` **refused** — its system-prompt "don't write report files" guideline outweighed the WORKORDER. Verified it is a **soft guideline, not a hard block**: no settings/hooks exist, and a probe subagent given a **forceful explicit override** wrote a report-named `.md` cleanly. Fix encoded in the claude-code adapter: spawn prompts MUST carry an artifact-write authorization block. Graceful fallback: worker returns text, PA captures it. UNVERIFIED: whether `@distillery` invoked *inside* a subagent writes its files — fall back to PA-run distillation if needed.

## 4. Worker sandbox can't run `pio run` / `python` build scripts
Background subagents could run `pio test -e native` but were **denied** `pio run -e esp32dev`, `pio check`, and `python3 scripts/portalBuilder.py` (the portal-asset gzip builder). Consequence: workers can't verify the **device build** or regenerate **portal assets** — the PA (main session) must close those out. `portalBuilder.py` also can't run standalone (`Import("env")` — it's a PlatformIO/SCons build hook); it runs only inside `pio run`.

## 5. Git ref collision — no bare parent branch in a slash-namespaced stack
`git checkout -b pr-20-config-refactor` then `pr-20-config-refactor/wo1-...` **fails**: git stores refs as files, so a branch can't be both a file and a directory. The SKILL's Step 2 (dash names off a bare parent) conflicts with STACKING_REFERENCE (slash names, true stack). Resolution: **true stack, no bare parent** — `wo1` off `main`, `wo2a` off `wo1`, `wo2b` off `wo2a`.

## 6. Stash index drift
The bridge referenced `stash@{2}` for `portal-tab-wip`; it had drifted to `stash@{1}` (`580f2f5`). Always re-resolve stashes **by hash**, not index. (It was redundant — superseded by landed `ef7be1e` — and dropped with approval.)

## 7. Multi-agent shared-checkout surprise
The OTA-fix agent worked in the **same working tree** and committed straight to `main` + committed the PA's uncommitted WO1 work (`FINDINGS.md`) as `17dd1e6`. Nothing was lost, but: when handing off to another agent, prefer a **separate worktree/clone** or park (commit/stash) uncommitted work first.
