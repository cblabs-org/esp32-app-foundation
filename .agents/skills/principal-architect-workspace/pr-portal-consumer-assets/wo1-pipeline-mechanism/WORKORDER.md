---
name: portal-consumer-assets-wo1
description: Handoff instructions for Work Order 1 — pipeline mechanism (consumer scan + shared serveGzipAsset helper + framework migration)
complexity: standard
---

# WO1 — Pipeline Mechanism

**Parent project:** `pr-portal-consumer-assets` — decomposition approved by the
human on 2026-08-16 (see
`.agents/skills/principal-architect-workspace/pr-portal-consumer-assets/PROJECT.md`,
§Human Approval). This is your authorization to implement this WORKORDER
autonomously within its Strict Boundary Box. Closes the mechanism half of
issue #111.

**Branch:** you are already on `portal-consumer-assets/wo1-pipeline-mechanism`
(base `main`). Commit here. Do NOT create other branches.

### Role Definition
You are an expert senior C++/embedded + Python build engineer and
execution-focused worker agent. You are NOT the Principal Architect. Do not make
high-level architectural decisions, do not branch, and do not plan beyond the
checklist. Your task is strictly limited to executing the checklist below.

### Objective
Make the framework's gzip+PROGMEM asset pipeline reachable by consuming apps and
eliminate the hand-rolled serve boilerplate: (1) teach `portalBuilder.py` to also
scan the consuming project's `web/` directory; (2) add one shared inline helper
`serveGzipAsset()`; (3) migrate every framework serve site onto it.

### Background (why)
Today `scripts/portalBuilder.py` scans only framework dirs (`web/portal`,
`web/display`, `modules/*/web`), so consuming apps cannot use the gzip pipeline
and are forced into `response->send(200, "…", rawliteral)` which copies the whole
body into internal DRAM (near-OOM). Separately, all 7 framework modules that serve
gzipped JS hand-roll the same 3-line `setContent`/`setContentType`/`addHeader`
incantation — no shared helper exists. This WO fixes both. (The consumer-facing
demonstration + docs are WO2 — not your concern.)

### Strict Boundary Box — DO NOT TOUCH
- **No** `docs/**`, **no** `.claude/skills/**` / `.agents/skills/**` markdown
  (docs are WO2). The ONLY markdown you write is your own Completion Report in
  this WORKORDER.md.
- **No** `sample-app/**` (WeatherModule migration is WO2).
- Do **not** change the generated output directory (`include/portal_assets/`) or
  the asset variable/length naming convention (`<name>_gz`, `<name>_gz_len`).
- Do **not** change route paths, `getTabScript()` values, tab metadata, or any
  behaviour other than the mechanical serve-call substitution.
- Do **not** alter the existing basename-collision detection in portalBuilder.py.
- Do **not** create a standalone feature branch or open a PR.

### Tooling Constraints
- **Builds MUST go through the `platformio` MCP server** (project rule
  `.agents/rules/04-mcp-tooling.md`) — use `build_project` / `agent_build_diagnose`,
  NOT raw `pio` CLI. Acquire/release the lock as the `pio-manager` skill dictates
  if needed.
- Temporary files (e.g. a throwaway consumer `web/` dir used to smoke-test the
  scan) go under a `tmp/` dir at repo root (rule `.agents/rules/02-tmp_workspace.md`)
  and must be removed before you report done.
- Follow the project house style for the new header (license header block,
  Doxygen-style comments) — match the style of existing files under `include/`.

### Line-Item Instructions

**Layer 1 — build scan**
- [ ] `scripts/portalBuilder.py` — after the `modules/*/web` loop that populates
      `scan_dirs` (around line 44, before `seen_files = {}`), append a scan of the
      consuming project's `web/` directory:
  ```python
  # Also scan the consuming project's web directory (downstream apps).
  # _FW is the framework root; PROJECT_DIR is the consuming app. In an
  # in-tree framework build they coincide and $FW/web holds only subdirs
  # (portal/, display/) with no top-level files, so the isfile() filter
  # below makes this a safe no-op there.
  try:
      project_web = os.path.join(env.subst("$PROJECT_DIR"), "web")
      if os.path.isdir(project_web) and project_web not in scan_dirs:
          scan_dirs.append(project_web)
  except NameError:
      pass  # direct Python invocation — no env/PROJECT_DIR
  ```
      Keep the existing collision guard intact (it now also protects consumer
      files against framework-name clashes — desirable).

**Layer 2 — shared serve helper**
- [ ] `include/web/serveGzipAsset.hpp` (NEW) — an inline free function in a new
      lightweight header. Signature (adapt return/param types to what PsychicHttp's
      `PsychicResponse` API actually exposes — inspect an existing serve site such
      as `modules/MCPServer/MCPServer.cpp:369`):
  ```cpp
  inline esp_err_t serveGzipAsset(PsychicResponse* response,
                                  const uint8_t* data, size_t len,
                                  const char* contentType) {
      response->setContent(data, len);
      response->setContentType(contentType);
      response->addHeader("Content-Encoding", "gzip");
      return response->send();
  }
  ```
      Include the correct PsychicHttp header, a proper include guard, the standard
      project license header comment block, and Doxygen documentation. Place it so
      it is reachable from modules that already `-I include` (it is on the include
      path). Verify the actual `send()` return type and match it.

**Layer 3 — migrate framework serve sites**
- [ ] Migrate every hand-rolled gzip serve site in these 7 files to call
      `serveGzipAsset(response, <name>_gz, <name>_gz_len, "<contentType>")`,
      adding `#include "web/serveGzipAsset.hpp"` to each:
  - [ ] `modules/WebServer/WebServerManager.cpp` (index.html, app.css, app.js,
        system.js, wifi.js, … — the largest cluster)
  - [ ] `modules/MCPServer/MCPServer.cpp`
  - [ ] `modules/OTAUpdateManager/OTAUpdateManager.cpp`
  - [ ] `modules/TimeManager/TimeManager.cpp`
  - [ ] `modules/HomeAssistant/HomeAssistantModule.cpp`
  - [ ] `modules/U8G2Display/U8G2DisplayModule.cpp`
  - [ ] `modules/LVGLDisplay/LVGLDisplayModule.cpp`
      Find them with: `grep -rn "Content-Encoding" modules/`. Preserve each site's
      exact content-type string and asset variable names. This is a pure
      substitution — no behaviour change.

### Verification Requirement
1. Confirm portalBuilder.py is a no-op for the in-tree framework build (framework
   root has no top-level `web/` files) — you may smoke-test by creating a
   throwaway `tmp/consumer-web-test/` and reasoning about the scan, but the real
   consumer validation is WO2's job.
2. Via the `platformio` MCP: build the framework root `native` environment
   (fast compile sanity) AND at least one ESP32 target that exercises the widest
   module set — **`esp32dev_ha`** (pulls in HomeAssistant) — plus
   **`crowpanel-128-rotary`** (LVGLDisplay). All must compile cleanly. Report the
   exact environments you built and their results.
3. **CRITICAL — before committing, STOP and ask the human (via the PA) if they
   are happy with the changes.** Then commit with a clear conventional-commit
   message referencing #111 (e.g. `feat(portal): scan consumer web/ + shared
   serveGzipAsset() helper (#111, WO1)`), with the trailer
   `Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>`.

### Issue Surfacing
If you find a defect or concern OUTSIDE this boundary box (e.g. the shared
`include/portal_assets/` output dir causing cross-consumer collisions, or a
module whose serve pattern doesn't fit the helper), do NOT file an issue and do
NOT fix it — record it under Out-of-Scope Findings below. The PA will triage.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
-

### Decisions Made
-

### Dead Ends & Warnings
-

### Out-of-Scope Findings
-

### Blockers
(none)
