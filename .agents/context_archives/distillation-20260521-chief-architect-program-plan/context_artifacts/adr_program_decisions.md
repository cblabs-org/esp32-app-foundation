# ADR: ESP32 App Foundation — Chief Architect Program Decisions

**Session:** f4e779d9-4aa1-46e2-8115-e1e8edc4fa42
**Distilled:** 2026-05-21T08:56:00+01:00

---

## CA-01: Monorepo — No Separate `esp32-modules` Repo

**Decision:** Everything lives in `esp32-app-foundation`. Optional modules under `modules/` (flat). No `esp32-modules` separate repo.

**Why:** Avoids a two-library version compatibility matrix in v1. Optional modules extracted to individual git submodules only when they mature independently. The `lib_deps` registry model was explicitly rejected — apps consume via `git submodule` with SHA pinning.

---

## CA-02: Flat `modules/` Directory — No `modules/optional/` Split

**Decision:** All modules (kernel + optional) live flat under `modules/`. PlatformIO's LDF compiles only what is `#include`d. No filesystem split needed.

**Why:** PlatformIO's `lib_extra_dirs` would need listing both dirs. The `library.json` `srcDirs: ["lib", "modules"]` declaration handles module discovery automatically. Apps need only `lib_deps = framework/` with no `lib_extra_dirs`.

---

## CA-03: `std::vector` Two-Tier Model — ADR-05 Tombstoned

**Decision:** No `MAX_*` compile-time ceilings. The design spec §2.5, §14.3, §14.4 are authoritative. ADR-05 (fixed static arrays) is explicitly superseded.

**Why:** The user confirmed in session that the vector model was the final decision from a dedicated strategy session. Boot-phase allocation is functionally static; `shrink_to_fit()` + `_compactAndLock()` after `begin()` achieves the same memory profile.

---

## CA-04: `esp32-app-template` Folds Into This Repo as `template/`

**Decision:** No separate GitHub Template Repo. The `template/` directory at repo root serves as the app skeleton. Apps consume via `git submodule add`.

**Why:** Eliminates repo-to-repo maintenance overhead in v1. Removes the "two repos to update" problem when the framework interface changes.

---

## CA-05: Dual Framework `arduino + espidf`

**Decision:** `framework = arduino, espidf` in all environments.

**Why:** OTA rollback requires `esp_ota_mark_app_valid_cancel_rollback()` (IDF-only). Logger uses `ESP_LOGX` macros (IDF-only). DeviceCrypto uses hardware MAC (IDF-only). Display, web server, config libs require Arduino layer. Neither alone is sufficient.

---

## CA-06: `library.json` `srcDirs` Is the Source of Truth

**Decision:** `library.json` declares `srcDirs: ["lib", "modules"]` and `includeDir: "include"`. Apps never need `lib_extra_dirs`.

**Why:** Keeps app `platformio.ini` clean. Ensures the framework is self-describing as a PlatformIO library. Verified to work with submodule local path references.

---

## CA-07: Testing Is First-Class in Every Work Order

**Decision:** Three-tier testing mandate: Tier 1 (native unit, GoogleTest), Tier 2 (hardware integration), Tier 3 (E2E boot test, gate before PR-06). WOs do not close without passing tests.

**Why:** "It compiles" is not done. The embedded-systems skill explicitly requires resource impact assessment. The pio-manager skill requires all test ops use MCP Tier 1 with `background: true` and explicit `environment`.

---

## CA-08: Mandatory Skill Boot Sequence

**Decision:** Every PA and Worker must load 5 skills as their first actions: pio-manager, embedded-systems, house-style-docs, oo-expert, pio-manager/references/mcp-agent-reference.md.

**Why:** Without these, agents write code that violates: macOS USB stability rules (pio-manager), stack/heap constraints (embedded-systems), file header standards (house-style-docs), SRP/DIP/DI patterns (oo-expert).

**Note:** `house-style-docs` was installed via `skill-manager add` (hash `3f771f09`). Do NOT re-copy from `departures-board` manually.

---

## CA-09: Git Repo Does Not Exist Yet

**Decision:** PA-01 WO1 bootstraps the git repo as its absolute first task: `git init` → `.gitignore` → initial commit of `docs/` + `.agents/` → `main` branch.

**Branch naming:**
- Feature branches: `pr-NN-name` off `main`
- WO branches: `pr-NN-name/woY-topic` off feature branch
- WO→Feature: regular merge (no squash)
- Feature→main: squash permitted

**Sequencing gates:**
- PR-01 merge → issue PA-02
- PR-02 merge → issue PA-03
- PR-03 merge → issue PA-04 + PA-05 simultaneously
- PR-04 + PR-05 both merged → issue PA-06
- PR-06 merge → issue PA-07
