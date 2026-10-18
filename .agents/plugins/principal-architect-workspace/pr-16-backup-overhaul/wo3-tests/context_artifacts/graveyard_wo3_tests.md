# Graveyard: WO3 Failed Attempts & Dead Ends

## Issue 1: Stale `.pio/build/native` x86_64 cache

**Symptom:** After first `pio test -e native`, all 17 tests showed `ERRORED` with messages like:
```
ld: warning: ignoring file '.pio/build/native/lib613/libgoogletest.a(gtest.o)': found architecture 'x86_64', required architecture 'arm64'
Undefined symbols for architecture arm64: "_main"
ld: symbol(s) not found for architecture arm64
```

**Root cause:** The `.pio/build/native/` directory contained cached `.o` and `.a` files compiled for x86_64 (from a prior build on a different machine or Rosetta session). The MCP `clean_project` tool did NOT remove these cached objects — it only cleaned the primary build artifacts, not the test build cache.

**Fix:** `rm -rf .pio/build/native` — full manual deletion. After this, the fresh arm64 build succeeded and all tests passed.

**PA Note:** Add `rm -rf .pio/build/native` to the WO wrap-up notes for future agents inheriting this repo on a new Apple Silicon machine.

---

## Issue 2: `git commit --amend` collision with WO2 staged files

**Symptom:** After committing `test_configManager.cpp` as WO3, the `server.js` fix needed a follow-up commit. Used `git add test/web/server.js && git commit --amend --no-edit` but this accidentally amended a WO2 distillation commit (`chore(wo2): distill session...`) that had been staged before WO3 began. Result: `server.js` changes ended up in the WO2 commit.

**Root cause:** There were pre-existing staged-but-uncommitted WO2 distillation files from a prior session that had not yet been committed. When `git commit --amend` ran, it amended the wrong prior commit.

**Fix:** `git reset HEAD~1` to undo the bad amend, then `git add` + `git commit` the WO2 distillation files separately, then `git add test/web/server.js` + `git commit` the WO3 server.js fix separately.

**Residual state:** The server.js WO3 fix ultimately ended up included in the WO2 distillation commit (`24bc69d`) due to the git rebase squash also colliding. The content is correct and fully committed; only the attribution is slightly impure. The PA approved accepting this state.

---

## Issue 3: MCP `check_task_status` returns "Task ID not found" immediately

**Symptom:** Background `run_tests` tasks would return `{ status: "running", taskId: "..." }` but polling `check_task_status` with that ID immediately returned `"Task ID not found"`.

**Root cause:** The native test suite (pure CPU, no hardware I/O) completes in ~7-10 seconds — faster than the poll cycle. The task ID is cleaned up from the MCP server's in-memory registry before the first poll.

**Fix/Workaround:** Read the log file directly via `query_logs` with the `logPath` from the initial `run_tests` response. Always prefer this for fast-completing native test runs.
