---
name: pr-25-wo6
description: Handoff instructions for Work Order 6 — Phase 1b deferred developer guides
complexity: standard
---

# WO6 — Phase 1b Deferred Guides + Skill References

You are an expert senior software engineer and technical writer, and an
execution-focused worker agent. You are NOT the Principal Architect. Do not
make high-level architectural decisions, do not branch, and do not plan.
Your task is strictly limited to executing the checklist below.

---

## Step 0 — Mandatory Rules Load (DO THIS FIRST)

**Before doing anything else**, read every file inside `.agents/rules/` in
filename order using file-read tools. These are unconditional operational
constraints. Do not skip this step. Do not assume they were auto-loaded.

---

## Prerequisite Context

WO1–WO5 have already landed. Before writing, read:

**Existing developer docs** (to match tone and style):
- `docs/developer/architecture-overview.md`
- `docs/developer/creating-a-module.md`
- `docs/developer/README.md` — you will update this to add the new guides

**Source files for accuracy** (read before writing each guide):
- `modules/DataManager/DataManager.hpp` — for scheduling guide
- `include/interfaces/IDataSource.hpp` — for data source contract
- `modules/U8G2Display/U8G2DisplayModule.hpp` — for optional module example
- `modules/EventBus/EventBus.hpp` — for EventBus interaction patterns
- `platformio.ini` — for build system guide
- `scripts/inject_version.py` — for version injection
- `scripts/deploy_release.py` — for release process
- `docs/OTADeveloperGuide.md` — for OTA cross-reference

**Existing skill** (to add new references):
- `.agents/skills/framework-expert/SKILL.md`
- `.agents/skills/framework-expert/skill_manifest.json`

---

## Objective

Create 3 additional developer guides (Phase 1b topics deferred from WO4) and
add corresponding reference files to the framework-expert skill.

---

## Strict Boundary Box

- **DO** create: 3 new files in `docs/developer/`
- **DO** modify: `docs/developer/README.md` (add new guides to reading order)
- **DO** create: 3 new reference files in `.agents/skills/framework-expert/references/`
- **DO** modify: `.agents/skills/framework-expert/SKILL.md` (update routing table)
- **DO** modify: `.agents/skills/framework-expert/skill_manifest.json` (update timestamp)
- **DO NOT** modify any source code files (`.hpp`, `.cpp`, `.h`, `.js`)
- **DO NOT** modify any files in `modules/`, `template/`, `include/`, or `src/`
- **DO NOT** create branches or make git commits — the PA handles branching.

---

## Line-Item Checklist

### Part A — `docs/developer/using-optional-modules.md`

- [ ] Create `docs/developer/using-optional-modules.md`
- [ ] Read `modules/U8G2Display/U8G2DisplayModule.hpp` and its sub-objects for
  reference — this is the most complex optional module.
- [ ] Content MUST cover:

  1. **What "optional" means** — modules that add platform-specific features
     (display, sensors, actuators) that not every application needs
  2. **The `#ifndef NATIVE_BUILD` guard pattern** — why it exists, how to use it
     correctly, what happens if you forget it
  3. **Display module as worked example** — how it implements 5 interfaces,
     how its sub-objects (SettingsTabStub, ScreenshotTabStub, boards) are
     registered via `registerModule()`
  4. **Adding your own optional module** — the full recipe:
     - Create your module class implementing the needed interfaces
     - Guard hardware includes with `#ifndef NATIVE_BUILD`
     - Register in `main.cpp` (or `Application::begin()` if it's framework-level)
     - Provide `NATIVE_BUILD` stubs or mocks for testing
  5. **Cross-reference**: link to `creating-a-module.md` for the base pattern

### Part B — `docs/developer/scheduling-data-fetches.md`

- [ ] Create `docs/developer/scheduling-data-fetches.md`
- [ ] Read `include/interfaces/IDataSource.hpp` and
  `modules/DataManager/DataManager.hpp` for accuracy
- [ ] Content MUST cover:

  1. **`IDataSource` interface** — full contract with method signatures:
     - `fetchData()` — called by DataManager on schedule
     - `getPollingIntervalMs()` — how often to poll
     - `getSourceName()` — identifier for logging
  2. **DataManager** — the background scheduler that manages `IDataSource` instances:
     - `registerSource()` / `unregisterSource()`
     - `requestPriorityFetch()` — for immediate out-of-schedule refresh
     - `setAppRunning()` — gating (only fetches when app is RUNNING)
  3. **Registration** — via `registerModule()` (auto-detected if implementing `IDataSource`)
     or via `app.getDataManager().registerSource()` directly
  4. **FreeRTOS task** — DataManager runs on its own task. Explain that
     `fetchData()` executes on the DataManager's task, NOT the main loop.
     Thread safety implications for shared state.
  5. **Example** — skeleton IDataSource implementation with a timed HTTP fetch

### Part C — `docs/developer/build-system.md`

- [ ] Create `docs/developer/build-system.md`
- [ ] Read `platformio.ini`, `scripts/inject_version.py`, and
  `scripts/deploy_release.py` for accuracy
- [ ] Content MUST cover:

  1. **PlatformIO environments** — explain the environments in `platformio.ini`:
     - `esp32dev` — hardware target
     - `native` — desktop build for unit testing
     - Any other environments present
  2. **Dual-framework support** — Arduino + ESP-IDF, how they coexist
  3. **Version injection** — `inject_version.py` reads git tags at build time.
     Reference `.agents/rules/03-versioning.md` for the versioning policy.
  4. **Building and testing** — how to build and run tests:
     - Using the PlatformIO MCP server tools (preferred for AI agents)
     - Using CLI fallback (`pio run`, `pio test`)
  5. **LittleFS filesystem** — how to build and upload the filesystem image
     for web portal assets
  6. **Cross-reference**: link to `docs/OTADeveloperGuide.md` for the release
     pipeline (build → sign → deploy)

### Part D — Update `docs/developer/README.md`

- [ ] Open `docs/developer/README.md`
- [ ] Add the 3 new guides to the reading order:
  ```markdown
  6. [Using Optional Modules](using-optional-modules.md) — platform-specific features
  7. [Scheduling Data Fetches](scheduling-data-fetches.md) — background data workers
  8. [Build System](build-system.md) — PlatformIO, environments, version injection
  ```

### Part E — Add Skill References

- [ ] Create `.agents/skills/framework-expert/references/optional-modules.md`
  - Distillation of `docs/developer/using-optional-modules.md`
  - Focus on: guard pattern, registration of sub-objects, NATIVE_BUILD stubs

- [ ] Create `.agents/skills/framework-expert/references/data-fetches.md`
  - Distillation of `docs/developer/scheduling-data-fetches.md`
  - Focus on: IDataSource method signatures, DataManager API, thread safety

- [ ] Create `.agents/skills/framework-expert/references/build-system.md`
  - Distillation of `docs/developer/build-system.md`
  - Focus on: environment names, build/test commands, version injection

### Part F — Update Skill Routing and Manifest

- [ ] Open `.agents/skills/framework-expert/SKILL.md`
- [ ] Add 3 new rows to the reference routing table:
  ```markdown
  | Optional modules, NATIVE_BUILD guards, display sub-objects | `references/optional-modules.md` |
  | IDataSource, DataManager, background data fetching | `references/data-fetches.md` |
  | PlatformIO, build environments, version injection, LittleFS | `references/build-system.md` |
  ```

- [ ] Open `.agents/skills/framework-expert/skill_manifest.json`
- [ ] Update `lastUpdated` to today's date
- [ ] Add the 3 new references to the `references` array

---

## Verification Requirement

After completing all checklist items, verify:
1. All 3 new docs exist in `docs/developer/`
2. `docs/developer/README.md` lists all 8 guides in reading order
3. All 3 new reference files exist in the skill's `references/`
4. `SKILL.md` routing table has all 8 reference entries
5. `skill_manifest.json` is updated
6. All code examples verified against actual source files
7. No source code was modified

**STOP** after verification and send a message back to the Principal Architect
reporting your results. Do not distill or commit until the PA approves.
