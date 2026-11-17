---
schema_version: 1
wo_id: WO-329.2
pr_id: PR-329
title: Replace 11 remaining %zu format specifiers, add unit tests, and add check_newlib_printf
  CI gate
complexity: standard
status: PENDING
allowed_files:
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
forbidden_files:
- docs/design_specification.md
- README.md
- modules/U8G2Display/
- modules/U8G2GLDisplay/
- THIRD-PARTY-NOTICES.md
- test/render/
- scripts/deploy_release.py
required_skills: []
verification_command: python3 scripts/check_newlib_printf.py && pytest scripts/tests/
  && python3 scripts/doc_lint.py && python3 scripts/check_doc_cpp_samples.py
max_attempts: 5
max_structural_failures: 2
spec_hash: 9a8fa52eb46a8e7ae2789fee994c1de3a9a9d29b4a2f0774fc8e4aed65a38cc7
completion: null
---

# WO-329.2 — Replace 11 remaining %zu format specifiers, add unit tests, and add check_newlib_printf CI gate

## 1. Role & Objective

You are executing `WO-329.2` on stacked branch `pr-329-portal-mcp-newlib/wo2-newlib-nano-printf-sweep` (stacked on `WO-329.1`) for `PR-329` (`prog-public-launch`). Your objective is to eliminate the full class of Newlib-nano `%zu` / unsupported length-modifier format bugs across the firmware codebase:
1. Replace all `11` remaining `%zu` format specifiers across `7` C++ files with `%u` + `static_cast<unsigned>(...)` (with matching casts on trailing varargs).
2. Add native C++ GoogleTest coverage in `test/test_unit/test_application/test_application.cpp` (`RedactorPreconditionContributor` healthy and saturated formatting) and `test/test_unit/test_mcpServer/test_mcpServer.cpp` (`MCPServer::begin` contributor count + callable tools), and verify both suites via PlatformIO MCP `run_tests` (`env: "native"`).
3. Implement zero-dependency static linter `scripts/check_newlib_printf.py`, pytest suite `scripts/tests/test_check_newlib_printf.py`, and add the CI step in `.github/workflows/ci.yml`.

## 2. Strict Boundary Box & Constraints

- **Allowed files (`allowed_files`):** Create or modify ONLY:
  - `modules/TimeManager/TimeManager.cpp`
  - `modules/Application/RedactorPreconditionContributor.hpp`
  - `modules/ConfigManager/JsonPatch.cpp`
  - `modules/BLEManager/BLEAdvertisingBuilder.cpp`
  - `modules/WebServer/WebServer.cpp`
  - `modules/EncoderModule/EncoderModule.cpp`
  - `lib/MD5Utils/MD5Utils.cpp`
  - `test/test_unit/test_mcpServer/test_mcpServer.cpp`
  - `test/test_unit/test_application/test_application.cpp`
  - `scripts/check_newlib_printf.py`
  - `scripts/tests/test_check_newlib_printf.py`
  - `.github/workflows/ci.yml`
  - plus Part II (`status`, `completion`, and `## Completion Report`) of this `WORKORDER.md`.
- **Forbidden files (`forbidden_files`):** Never touch `docs/design_specification.md`, `README.md`, `modules/U8G2Display/`, `modules/U8G2GLDisplay/`, `THIRD-PARTY-NOTICES.md`, `test/render/`, `scripts/deploy_release.py`, or any path outside `allowed_files`.
- **Tooling & Git Discipline:**
  - Read every file inside `.agents/rules/` in filename order before making edits (pay strict attention to `04-mcp-tooling.md`).
  - **NEVER** put raw `pio`, `platformio`, or `esptool` commands in `run_command`. Use PlatformIO MCP (`call_mcp_tool` with `ServerName: "platformio-mcp_platformio"`, `ToolName: "run_tests"`) to execute native unit tests, and **ALWAYS** call `release_lock` before running `pa wo verify WO-329.2`.
  - NEVER use `*` or `**` globs in `run_command`.
  - NEVER mention protected filenames (`docs/design_specification.md` or `README.md`) in any `run_command` argument/string.
  - Do NOT create any git tag, upload firmware, or run `git push`, `git checkout`, `git switch`, `git branch`, `git rebase`, `git merge`, `gh pr`, or `gh issue`.

## 3. Line-Item Instructions

### 3.1 Replace All 11 Remaining `%zu` Format Specifiers
Replace every `%zu` specifier below with `%u` and `static_cast<unsigned>(...)` (keeping trailing vararg types cleanly aligned):
1. **`modules/TimeManager/TimeManager.cpp`:**
   - L737: `ESP_LOGI(TAG, "Receiving timezone database update (%u bytes)", static_cast<unsigned>(bodyLen));`
   - L748: `ESP_LOGI(TAG, "Timezone database updated (%u bytes)", static_cast<unsigned>(bodyLen));`
   - L882: `ESP_LOGI(TAG, "DHCP Option 42 active: %u server(s) (primary=%s)", static_cast<unsigned>(_dhcpNtpCount), _dhcpNtpServers[0]);` *(Fixes the P1 `LoadProhibited` crash where `%s` popped unconsumed `_dhcpNtpCount` under Newlib-nano.)*
2. **`modules/Application/RedactorPreconditionContributor.hpp`:**
   - L43: `snprintf(buf, len, "Redactor registry saturated (%u/%u secrets, %u rejected)", static_cast<unsigned>(Redactor::instance().secretCount()), static_cast<unsigned>(Redactor::MAX_SECRETS), static_cast<unsigned>(Redactor::instance().saturationCount()));`
   - L47: `snprintf(buf, len, "Redactor healthy (%u/%u secrets)", static_cast<unsigned>(Redactor::instance().secretCount()), static_cast<unsigned>(Redactor::MAX_SECRETS));`
3. **`modules/ConfigManager/JsonPatch.cpp`:**
   - L310: `ESP_LOGW(TAG, "replace — index %u out of range (size=%d)", static_cast<unsigned>(idx), (int)arr.size());`
   - L383: `ESP_LOGW(TAG, "test — index %u out of range (size=%d)", static_cast<unsigned>(idx), (int)arr.size());`
4. **`modules/BLEManager/BLEAdvertisingBuilder.cpp`:**
   - L222: `ESP_LOGE(TAG, "Required advertising element '%s' (%u B) fits in neither PDU (avail: prim=%u, scan=%u)", elem.description.c_str(), static_cast<unsigned>(elemSize), static_cast<unsigned>(availPrimary), static_cast<unsigned>(availScan));`
   - L225: `ESP_LOGW(TAG, "Optional advertising element '%s' (%u B) dropped due to PDU overflow", elem.description.c_str(), static_cast<unsigned>(elemSize));`
5. **`modules/WebServer/WebServer.cpp`:**
   - L602-603: `ESP_LOGI(TAG, "Global body-size middleware registered (default=%u, %u overrides)", (unsigned)DEFAULT_MAX_POST_BODY, static_cast<unsigned>(s_bodyLimits.size()));`
6. **`modules/EncoderModule/EncoderModule.cpp`:**
   - L45-46: `ESP_LOGE(TAG, "Cannot add encoder '%s': MAX_ENCODERS (%u) reached", encoder.id ? encoder.id : "?", static_cast<unsigned>(MAX_ENCODERS));`
7. **`lib/MD5Utils/MD5Utils.cpp`:**
   - L90: `ESP_LOGE(TAG, "base64ToHex: allocation failed (%u bytes)", static_cast<unsigned>(decodedLen));`

### 3.2 Native C++ Unit Tests
1. **`test/test_unit/test_application/test_application.cpp`:**
   - Add a `RedactorPreconditionContributorTest` fixture (calling `Redactor::instance().clearSecrets()` in both `SetUp()` and `TearDown()` so singleton state is isolated even on assertion failure).
   - Register `RedactorPreconditionContributor` into a `PreconditionEngine` instance (`contributor.registerPreconditions(engine);`).
   - Verify healthy formatting:
     - With `0` secrets: `engine.tick(100)`, fetch `const PreconditionState* st = engine.getStateByName(PreconditionNames::REDACTOR_HEALTH);`, verify `st != nullptr`, `st->rawStatus.met == true`, `st->rawStatus.cause == Cause::Met`, and `st->describe(st->rawStatus.cause, buf, sizeof(buf), st->context)` formats `"Redactor healthy (0/16 secrets)"`.
     - After registering 2 secrets (>= `Redactor::MIN_SECRET_LEN` chars, e.g. `"secret-alpha"`, `"secret-bravo"`): tick `engine`, verify `rawStatus.met == true`, and `describe(...)` formats `"Redactor healthy (2/16 secrets)"`.
   - Verify saturated formatting:
     - Register `Redactor::MAX_SECRETS` (16) unique secrets (`>= 4` chars each), then attempt to register 3 additional unique secrets (`>= 4` chars each, asserting `RegisterResult::Full`), tick `engine`, verify `st->rawStatus.met == false`, `st->rawStatus.cause == Cause::FallbackEngaged`, and `describe(...)` formats `"Redactor registry saturated (16/16 secrets, 3 rejected)"`.
2. **`test/test_unit/test_mcpServer/test_mcpServer.cpp`:**
   - Add a test for `MCPServer::begin` with a mock `IMCPContributor` registering 2 custom tools on top of built-in tools:
     - Measure `builtinCount` dynamically via a baseline `MCPServer baselineServer; std::vector<IMCPContributor*> emptyContributors; baselineServer.begin(emptyContributors, nullptr); const size_t builtinCount = baselineServer.getRegistry().size();`.
     - Create a mock `IMCPContributor` registering 2 custom tools (e.g. `"custom_tool_one"` and `"custom_tool_two"`).
     - Call `server.begin(contributors, nullptr);`.
     - Assert `server.getRegistry().size() == builtinCount + 2`, both custom tools are non-null via `server.getRegistry().findTool(...)`, and both tools are callable via `processAndCapture(server, ...)` JSON-RPC `tools/call` returning their expected payloads.

### 3.3 Static Linter, Pytest Suite, & CI Gate
1. **`scripts/check_newlib_printf.py`:**
   - Create a zero-dependency Python 3 script (`#!/usr/bin/env python3`) with standard CB Labs Apache-2.0 header (`ESP32 App Foundation (c) 2026 CB Labs`, `https://github.com/cblabs-org/esp32-app-foundation`, `SPDX-License-Identifier: Apache-2.0`).
   - Scan `.c`, `.cpp`, `.h`, `.hpp`, `.ipp` files under existing production roots:
     `("src", "include", "lib", "modules", "adapters", "hardware", "sample-app/src", "sample-app/include", "sample-app/lib", "template-app/src", "template-app/include", "template-app/lib")`,
     skipping missing roots and skipping any `test` or `.pio` subdirectories (`parts` check / `os.walk` pruning).
   - Use a state-machine C/C++ scanner (preserving exact 1-based line numbers) that:
     - Strips `// ...` and `/* ... */` comments outside string/char literals (preserving embedded `\n` counts so line numbers never drift).
     - Tracks character literals `'...'` (with escape handling) so `'"'` or `'/'` inside char literals does not mis-trigger string/comment state.
     - Extracts C/C++ string literals (`"..."` with optional `u8`/`u`/`U`/`L` prefix and backslash escapes, AND raw string literals `R"delim(...)delim"` with optional `u8`/`u`/`U`/`L` prefix) and scans their interior with an escaped-`%%`-safe pattern: `r"%%|(%(?:[-+#0 ]*(?:\*|\d+)?(?:\.(?:\*|\d+))?(?:ll|[zjt])[diouxX]))"`, flagging whenever group 1 matches (so literal `%%zu` is allowed, while `%zu`, `%zd`, `%llu`, `%08zu`, `%.*llu`, `%jd`, `%td`, etc. are flagged with the exact 1-based line number).
     - Scans non-string, non-comment code tokens for `\bPRI[diouxX](?:64|LEAST64|FAST64|MAX)\b` (while allowing `PRIu32`, `PRId32`, etc.).
   - Expose clean importable helpers (`scan_text(text: str, relpath: str = "<string>")`, `scan_repo(repo_root: Path)`, `main(argv=None) -> int`) for pytest.
   - Exit `0` when clean, `1` when violations exist (printing `relpath:line: <detail>` diagnostics).
2. **`scripts/tests/test_check_newlib_printf.py`:**
   - Follow the house header/style of `scripts/tests/test_next_version.py` and `scripts/tests/test_run_web_tests.py`.
   - Test `%zu`, `%zd`, `%llu`, `%lld`, `%llx`, `%jd`, `%td`, width/precision forms (`%08zu`, `%.*llu`), `PRIu64` / `PRId64` / `PRIuLEAST64` / `PRIuFAST64` / `PRIuMAX` in non-string code, string containing `//` or `/*` before `%zu`, raw string literals `R"(...)"` / `R"raw(...)raw"`, escaped `%%zu` (allowed), `PRIu32` (allowed), `%zu` and `PRIu64` inside `//` and `/* ... */` comments (allowed, plus line-number accuracy after multi-line comments), `scan_repo` exclusion of `test/` and `.pio/` subdirs, CLI exit code `1` on violations, and `scan_repo(REPO_ROOT) == []` + CLI exit `0` on the real repository.
3. **`.github/workflows/ci.yml`:**
   - In job `lint-and-secrets`, immediately after `- name: Run documentation lint` (lines 51–52), add:
     ```yaml
           - name: Check Newlib-nano printf format specifiers
             run: python3 scripts/check_newlib_printf.py
     ```

## 4. Verification, Cryptographic Receipt & Completion Report

1. Run native C++ unit tests via PlatformIO MCP (`call_mcp_tool` -> `ServerName: "platformio-mcp_platformio"`, `ToolName: "run_tests"`):
   - `environment: "native"`, `testFilter: "test_unit/test_application"`
   - `environment: "native"`, `testFilter: "test_unit/test_mcpServer"`
   - Call `release_lock` on `platformio-mcp_platformio` immediately after tests complete!
2. Run `python3 scripts/check_newlib_printf.py && pytest scripts/tests/ && python3 scripts/doc_lint.py && python3 scripts/check_doc_cpp_samples.py` and confirm exit `0` with all tests passing.
3. Run `/Users/mcneillm/.gemini/config/plugins/pa-daemon/bin/pa wo verify WO-329.2` to verify boundary hygiene, re-run `verification_command`, stamp the cryptographic `tree_hash` receipt, update `WORKORDER.md` / `PROJECT.md`, and commit to `pr-329-portal-mcp-newlib/wo2-newlib-nano-printf-sweep` with `PA-WorkOrder` / `PA-Verified-Hash` trailers.
4. Ensure Part II (`status`, `completion`, and `## Completion Report`) is accurate, and report the commit SHA, receipt ID, modified files, PlatformIO MCP native test counts, and `verification_command` counts back to the Principal Architect via `send_message`.

## Completion Report
<!-- Written by the Tier 3 Worker at task completion -->

_Pending execution._
