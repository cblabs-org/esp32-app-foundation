---
schema_version: 1
charter:
  pr_id: PR-329
  programme_id: prog-public-launch
  slug: pr-329-portal-mcp-newlib
  title: 'fix(portal,core): slugify non-alnum tab IDs and eliminate Newlib-nano %zu
    format bugs'
  branch: fix/portal-tab-slug-and-mcp-log-format
  base_branch: main
  depends_on: []
  allowed_modules:
  - web/portal/app.js
  - test/web/tests/navigation.spec.ts
  - .agents/skills/framework-expert/systems/web-portal/html-js-fragments.md
  - .agents/skills/framework-expert/systems/web-portal/overview.md
  - docs/developer/extending-the-web-portal.md
  - modules/MCPServer/MCPServer.cpp
  - modules/MCPServer/MCPBuiltinTools.cpp
  - modules/TimeManager/TimeManager.cpp
  - modules/Application/RedactorPreconditionContributor.hpp
  - modules/ConfigManager/JsonPatch.cpp
  - modules/BLEManager/BLEAdvertisingBuilder.cpp
  - modules/WebServer/WebServer.cpp
  - modules/EncoderModule/EncoderModule.cpp
  - lib/MD5Utils/MD5Utils.cpp
  - test/test_unit/test_mcpServer/test_mcpServer.cpp
  - test/test_unit/test_application/test_application.cpp
  - scripts/check_newlib_printf.py
  - scripts/tests/test_check_newlib_printf.py
  - .github/workflows/ci.yml
  required_skills: []
  verification_floor:
  - python3 scripts/check_newlib_printf.py && pytest scripts/tests/ && python3 scripts/doc_lint.py
    && python3 scripts/check_doc_cpp_samples.py && python3 scripts/run_web_tests.py
  charter_locked: true
  charter_hash: 8a4d86cecc1acf4b3eb23c76a62950230468e5a5b0928bbc2d61c391989ddae2
status: APPROVED
approved_at: '2026-11-17T07:23:05.000071Z'
github_pr: null
work_orders:
- id: WO-329.1
  slug: wo1-portal-slug-and-docs
  title: Harden findTabByHash empty-slug guard, pill-btn DOM lookup, Playwright hash
    tests, and portal slug docs
  complexity: standard
  status: COMPLETED
  attempts: 1
  commit_sha: 05463b42efce7a6f1b17abee44da2947e6f7621e
- id: WO-329.2
  slug: wo2-newlib-nano-printf-sweep
  title: Replace 11 remaining %zu format specifiers, add unit tests, and add check_newlib_printf
    CI gate
  complexity: standard
  status: PENDING
  attempts: 0
  commit_sha: null
---

# PR-329 — fix(portal,core): slugify non-alnum tab IDs and eliminate Newlib-nano %zu format bugs

## 1. PR Charter: Scope, Goals, & Non-Goals

### Scope & Goals
- Incorporate and harden PR `#329` (`fix/portal-tab-slug-and-mcp-log-format`, base commit `c0df554f`) upstreaming two consumer-discovered fixes (`cblabs-org/echenon`) and eliminating the full class of Newlib-nano `%zu` format bugs across the firmware codebase:
  1. **Base PR `#329` changes already committed in `c0df554f`:**
     - `web/portal/app.js`: extracted `slugifyTabName(name)` (`replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '')`) across 12 tab/pill slugification sites.
     - `test/web/tests/navigation.spec.ts`: Playwright test verifying `'Intervals.icu'` DOM IDs, pill click + `tab-changed`, `/#tab-intervals-icu`, and `/#intervals.icu`.
     - `modules/MCPServer/MCPServer.cpp` & `modules/MCPServer/MCPBuiltinTools.cpp`: captured `builtinCount` after `mcpBuiltinTools::registerAll()` and replaced `%zu` with `%u` (`static_cast<unsigned>(...)`).
  2. **WO-329.1 (Portal & Slug Documentation Hardening):**
     - Guard `findTabByHash()` against empty `normSlug` (e.g. punctuation-only hashes like `#...`) and replace raw `candidateId` CSS selector string interpolation in Step 3 DOM fallback with an attribute comparison over `document.querySelectorAll('.pill-btn')`.
     - Extend `test/web/tests/navigation.spec.ts` to cover `/#tab-intervals.icu` and `/#...` fallback behavior.
     - Synchronize the tab container ID slugification contract (`'tab-' + name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '')`) in `.agents/skills/framework-expert/systems/web-portal/html-js-fragments.md`, `.agents/skills/framework-expert/systems/web-portal/overview.md`, and `docs/developer/extending-the-web-portal.md`.
  3. **WO-329.2 (Repo-Wide Newlib-Nano `%zu` Sweep, Unit Tests, & CI Guard):**
     - Replace all `11` remaining `%zu` format specifiers across `7` C++ firmware files with `%u` + `static_cast<unsigned>(...)` (plus matching `%u` cast alignment on trailing varargs where needed):
       - `modules/TimeManager/TimeManager.cpp` (L737, L748, L882 — fixes P1 `LoadProhibited` crash where `%s` popped unconsumed `_dhcpNtpCount`)
       - `modules/Application/RedactorPreconditionContributor.hpp` (L43, L47 — fixes `/api/preconditions` `"zu/zu secrets"` and saturated `%u rejected` vararg shift)
       - `modules/ConfigManager/JsonPatch.cpp` (L310, L383)
       - `modules/BLEManager/BLEAdvertisingBuilder.cpp` (L222, L225)
       - `modules/WebServer/WebServer.cpp` (L602)
       - `modules/EncoderModule/EncoderModule.cpp` (L45)
       - `lib/MD5Utils/MD5Utils.cpp` (L90)
     - Add C++ unit tests in `test/test_unit/test_application/test_application.cpp` (`RedactorPreconditionContributor` healthy and saturated messages) and `test/test_unit/test_mcpServer/test_mcpServer.cpp` (`MCPServer::begin` contributor count).
     - Add zero-dependency static checker `scripts/check_newlib_printf.py`, pytest suite `scripts/tests/test_check_newlib_printf.py`, and CI job step in `.github/workflows/ci.yml` (`lint-and-secrets`).

### Non-Goals
- Do **NOT** create any git tag (including `v1.0.1-dev.1`) or upload firmware to hardware on this PR branch (`v1.0.0-beta.2` will be cut by the user on `main` after PR `#329` merges).
- Do **NOT** touch open PR `#292`'s exclusion zone (`modules/U8G2Display/`, `modules/U8G2GLDisplay/`, `THIRD-PARTY-NOTICES.md`, `test/render/`), protected files (`docs/design_specification.md`, `README.md`), or `scripts/deploy_release.py`.

## 2. Architectural Requirements, Cross-PR Interfaces, & Required Skills/Tools

### Portal Tab Slug Contract (`web/portal/app.js`, docs, & tests)
- `slugifyTabName(name)` converts any tab/pill name to `String(name || '').toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '')`.
- In `findTabByHash(hash)`:
  - Top-level and child loops must require `(normSlug && topSlug === normSlug)` and `(normSlug && childSlug === normSlug)` respectively so punctuation-only hashes (e.g. `#...`, yielding `normSlug === ""`) never match empty slugs.
  - Step 3 DOM lookup must avoid string-interpolated `[data-target="..."]` CSS selectors (use `document.querySelectorAll('.pill-btn')` and compare `btn.getAttribute('data-target') === candidateId`), preventing DOM `SyntaxError` on unescaped characters while keeping lookup safe.

### Newlib-Nano Format Contract (`CONFIG_NEWLIB_NANO_FORMAT=y`) & Static Linter
- Because `sdkconfig.defaults`, `sample-app/sdkconfig.defaults`, and `template-app/sdkconfig.defaults` all enable `CONFIG_NEWLIB_NANO_FORMAT=y`, Newlib-nano `vfiprintf` does **not** parse `%z`, `%j`, `%t`, `%ll`, or `PRI*64` and prints literal format characters without advancing `va_list`.
- All firmware printf/log format strings in `src/`, `include/`, `lib/`, `modules/`, `adapters/`, `hardware/`, `sample-app/{src,include,lib}`, and `template-app/{src,include,lib}` MUST avoid C99/64-bit length modifiers (`%z*`, `%j*`, `%t*`, `%ll*`, `PRI*64`); `size_t` counts/sizes use `%u` with `static_cast<unsigned>(...)`.
- `scripts/check_newlib_printf.py`:
  - Uses standard CB Labs Apache-2.0 header; zero third-party dependencies.
  - Scans `.c`, `.cpp`, `.h`, `.hpp`, `.ipp` under the production source roots above (skipping any `test` or `.pio` subdirectories).
  - Strips `//` and `/* ... */` comments (preserving line numbers) and inspects C/C++ string literals (`"..."` and `R"delim(...)delim"`) for `%(?!%)[-+#0 ]*(?:\*|\d+)?(?:\.(?:\*|\d+))?(?:ll|[zjt])[diouxX]` and non-string code tokens for `\bPRI[diouxX](?:64|LEAST64|FAST64|MAX)\b`. Note: literal `%%zu` is allowed; `PRIu32` (used in `Application.cpp`/`OTAUpdateManager.cpp`) is allowed.
  - Exits `0` cleanly when zero violations exist, or `1` printing `relpath:line: <detail>` on stdout/stderr when violations are found.
- PlatformIO native unit tests (`test_unit/test_application`, `test_unit/test_mcpServer`) MUST be run via PlatformIO MCP `run_tests` (never raw `pio` CLI in `verification_command` or shell), followed by `release_lock`.

## 3. Implementation Plan & Work Order Breakdown

`PR-329` is decomposed into two sequential single-writer Work Orders stacked from `fix/portal-tab-slug-and-mcp-log-format` (base branch `main`, starting from `7984d8d5`):

| WO ID | Slug | Title | Complexity | Dependencies |
| --- | --- | --- | --- | --- |
| `WO-329.1` | `wo1-portal-slug-and-docs` | Harden findTabByHash empty-slug guard, pill-btn DOM lookup, Playwright hash tests, and portal slug docs | `standard` | Charter locked |
| `WO-329.2` | `wo2-newlib-nano-printf-sweep` | Replace 11 remaining %zu format specifiers, add unit tests, and add check_newlib_printf CI gate | `standard` | `WO-329.1` |

### Sequencing & Architectural Notes
1. **Portal Hash Guard, Selector Hardening, Playwright Coverage, & Slug Docs — `WO-329.1`** (`pr-329-portal-mcp-newlib/wo1-portal-slug-and-docs` from `fix/portal-tab-slug-and-mcp-log-format`):
   - **`web/portal/app.js`:** In `findTabByHash()`, require `(normSlug && topSlug === normSlug)` and `(normSlug && childSlug === normSlug)` so punctuation-only hashes yielding `normSlug === ""` (such as `#...`) never match empty slugs; in Step 3 DOM fallback, replace string-interpolated `document.querySelector('.pill-btn[data-target="' + candidateId + '"]')` with iterating `document.querySelectorAll('.pill-btn')` and checking `btn.getAttribute('data-target') === candidateId`.
   - **`test/web/tests/navigation.spec.ts`:** Extend the `non-alphanumeric tab names (e.g. Intervals.icu)` test (and/or add sibling checks) verifying `/#tab-intervals.icu` activates `#tab-intervals-icu` and `#pill-intervals-icu`, and `/#...` falls back cleanly to the default active tab without console/page errors or matching empty-slug tabs.
   - **Documentation:** Update container ID slug derivation to `'tab-' + name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '')` (e.g. `'Intervals.icu'` -> `'tab-intervals-icu'`) in `.agents/skills/framework-expert/systems/web-portal/html-js-fragments.md`, `.agents/skills/framework-expert/systems/web-portal/overview.md`, and `docs/developer/extending-the-web-portal.md`.
2. **Newlib-Nano `%zu` Sweep, C++ Unit Tests, & Static CI Linter — `WO-329.2`** (`pr-329-portal-mcp-newlib/wo2-newlib-nano-printf-sweep` stacked on `pr-329-portal-mcp-newlib/wo1-portal-slug-and-docs`):
   - **Firmware `%zu` sweep:** Replace all 11 remaining `%zu` format specifiers across 7 files (`modules/TimeManager/TimeManager.cpp`, `modules/Application/RedactorPreconditionContributor.hpp`, `modules/ConfigManager/JsonPatch.cpp`, `modules/BLEManager/BLEAdvertisingBuilder.cpp`, `modules/WebServer/WebServer.cpp`, `modules/EncoderModule/EncoderModule.cpp`, `lib/MD5Utils/MD5Utils.cpp`) with `%u` + `static_cast<unsigned>(...)`, keeping trailing varargs (`availPrimary`, `availScan`, `saturationCount()`) type-aligned.
   - **Native unit tests (verified via PlatformIO MCP `run_tests` on `native`):** Add `RedactorPreconditionContributor` healthy (`0/16`, `2/16`) and saturated (`16/16` + `3 rejected`) message formatting tests in `test/test_unit/test_application/test_application.cpp` (clearing `Redactor::instance().clearSecrets()` in setup/teardown), and add a `MCPServer::begin` test with a 2-tool `IMCPContributor` asserting `builtinCount + 2` and callable tool lookup/dispatch in `test/test_unit/test_mcpServer/test_mcpServer.cpp`.
   - **Static linter & CI:** Add `scripts/check_newlib_printf.py`, `scripts/tests/test_check_newlib_printf.py`, and the `Check Newlib-nano printf format specifiers` step in `.github/workflows/ci.yml` (`lint-and-secrets` right after `Run documentation lint`).

## 4. Outcome & Session Log
