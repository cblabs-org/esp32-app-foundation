# Graveyard — PR-16 Failed/Rejected Approaches

## Dead End 1: Initial Misdiagnosis — JsonDocument Capacity

**What we thought:** The downloaded backup file was empty because `JsonDocument` was silently truncating fields due to an insufficient capacity limit.

**Why it was wrong:** The actual downloaded content was `{"namespaces":[...],"count":4}` — this is not a truncated backup, it is the *wrong endpoint's response entirely*. The `/api/config` namespace list handler was swallowing the request. The real issue was route registration order, not document capacity.

**What to do instead:** Always compare the *shape* of an unexpected response against all registered route handlers before assuming a serialization bug.

---

## Dead End 2: Fixed 4096-byte JsonDocument Cap

**Proposed:** Set `JsonDocument<4096>` as the backup document capacity.

**Rejected because:** This project uses ArduinoJSON v7, where `JsonDocument` allocates dynamically from the heap with no fixed limit. There is no silent truncation — the document grows until heap is exhausted. Imposing a fixed cap would introduce an artificial failure mode that doesn't exist in the current library version. The user explicitly rejected a fixed limit.

---

## Dead End 3: `FIRMWARE_VERSION` Macro (Undefined)

**What happened:** WO1 worker used `#ifdef FIRMWARE_VERSION` / `meta["firmware_version"] = FIRMWARE_VERSION` in `buildBackupMeta()`. This macro is never defined anywhere in the project, so the `#else` branch (`"unknown"`) always fired.

**Discovery method:** Live device validation — MCP `system_info` returned `firmware_version: "0.1.0"` while the backup `_meta.firmware_version` showed `"unknown"`. Traced to the wrong macro name via grep.

**Resolution:** Replace `FIRMWARE_VERSION` with `APP_FOUNDATION_VERSION`, which is defined in `platformio.ini` line 29 and used consistently by all other version-reporting code in the project.

---

## Dead End 4: Stale x86_64 Native Build Cache

**Symptom:** `pio test -e native` reported all 17 test suites as `ERRORED` (not failed — errored, meaning the test binary couldn't even load).

**Root cause:** The `.pio/build/native` directory contained stale object files compiled for `x86_64` from a prior build on a different machine (or architecture). The arm64 linker rejected them silently. The MCP `clean_project` tool did not clear the native environment's build artifacts.

**Fix:** Manual `rm -rf .pio/build/native` before the first test run. After a clean arm64 rebuild, all 17 suites passed (404 test cases).

**For future agents:** If `pio test -e native` ERRORs on all suites immediately, always try `rm -rf .pio/build/native` first before investigating test code.

---

## Dead End 5: WO2 Modifying `server.js` (WO3 Scope Overlap)

**What happened:** WO2 worker updated `MOCK_CONFIG_BACKUP` in `test/web/server.js` to include a partial `_meta` block (module_count: 4) so they could verify the metadata summary UX. This was technically in WO3's scope.

**Impact:** WO3 then updated the same file with the correct 5-module format. Since all workers shared the same workspace (no branched workspaces), the last writer (WO3) won. The final `server.js` state is correct.

**Lesson:** When running parallel workers on overlapping files, either use branched workspaces (more expensive) or sequence the workers that share a file. In this case the overlap was harmless because the files were disjoint enough that WO3's write was authoritative and complete.
