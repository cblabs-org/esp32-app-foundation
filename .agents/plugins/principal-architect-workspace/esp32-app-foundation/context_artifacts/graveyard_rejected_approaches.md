# Graveyard: Rejected Approaches — Chief Architect Session

**Session:** f4e779d9-4aa1-46e2-8115-e1e8edc4fa42

---

## GRAVE-CA-01: `lib_deps` Registry Model for Framework Distribution

**Proposed:** Publish `esp32-app-foundation` to PlatformIO registry; apps consume via `lib_deps = matthewmcneill/esp32-app-foundation @ ^1.0.0`.

**Why rejected:**
- Absolute path workaround (`lib_deps = file:///Users/dev/...`) breaks on every other machine and violates the repo's own portability rule
- Semver tag management for firmware libraries is operationally brutal — a breaking `ITickable` change bricks field devices unless all apps update in lockstep
- Local dev iteration requires path hacks that don't survive CI

**Use instead:** `git submodule` with SHA pinning. App `platformio.ini` uses `lib_deps = framework/` (local submodule path). Deterministic, portable, auditable.

---

## GRAVE-CA-02: Separate `esp32-modules` Catalog Repo

**Proposed:** A second GitHub repo `esp32-modules` containing all optional modules, versioned independently from the kernel.

**Why rejected:**
- Creates a two-library version compatibility matrix (kernel@v1.x must match modules@v1.x)
- Optional modules frequently share interfaces defined in the kernel — version drift causes subtle ABI breaks
- Premature abstraction for v1 with only one real app (departures-board) as reference

**Use instead:** Flat `modules/` directory in the monorepo. LDF compiles only what's `#include`d. Modules extracted to individual git submodule repos only when they mature independently (future decision).

---

## GRAVE-CA-03: `modules/optional/` Filesystem Split

**Proposed:** Separate `modules/` (kernel) from `modules/optional/` (optional) with `lib_extra_dirs` listing both.

**Why rejected:**
- Forces `lib_extra_dirs = modules, modules/optional` in every app `platformio.ini`
- The `library.json` `srcDirs` approach makes this unnecessary — one declaration covers all
- The kernel/optional distinction is architectural, not filesystem — enforced by `#include` discipline and CI include-graph lint

**Use instead:** Flat `modules/` for everything. `library.json` `srcDirs: ["lib", "modules"]`.

---

## GRAVE-CA-04: `esp32-app-template` as Separate GitHub Template Repo

**Proposed:** A dedicated GitHub Template Repository that developers click "Use this template" on.

**Why rejected:**
- When the framework interface changes, two repos need updating simultaneously
- New developers must understand the `lib_deps` version pinning before writing a single line of code
- Adds friction without benefit in v1 where the framework is not yet stable

**Use instead:** `template/` directory within the main repo. `template/README.md` shows `git submodule add` workflow. Developers clone the framework, copy the template, and go.

---

## GRAVE-CA-05: ADR-05 — Fixed Static Registration Arrays (`MAX_TICKABLES` etc.)

**Proposed:** (from earlier session ADR-05) Use compile-time `MAX_*` defines for all registration arrays to avoid heap allocation.

**Why rejected (overturned in this session):**
- User explicitly confirmed the `std::vector` two-tier model was the final decision from a dedicated strategy session
- Boot-phase `push_back` is functionally equivalent to static allocation — the heap is not yet fragmented at `begin()` time
- `shrink_to_fit()` + `_compactAndLock()` after `begin()` achieves the same post-boot memory profile as static arrays
- The spec §2.5, §14.3, §14.4 are authoritative. ADR-05 is tombstoned.

**Use instead:** `std::vector` with `reserve(expectedCount)` at registration time, `shrink_to_fit()` + pointer lock after `begin()`.
