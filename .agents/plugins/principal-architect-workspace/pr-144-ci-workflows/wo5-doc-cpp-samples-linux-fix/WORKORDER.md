---
schema_version: 1
wo_id: WO-144.5
pr_id: PR-144
title: Fix WifiManager.hpp header casing and add PCH acceleration to check_doc_cpp_samples.py
complexity: standard
status: COMPLETED
allowed_files:
- scripts/check_doc_cpp_samples.py
forbidden_files:
- docs/design_specification.md
- modules/U8G2Display/
- modules/U8G2GLDisplay/
required_skills: []
verification_command: pytest scripts/tests/ && python3 scripts/doc_lint.py && python3
  scripts/check_doc_cpp_samples.py
max_attempts: 5
max_structural_failures: 2
spec_hash: ed4e3578d14f7acd1e4925fb32262791f807465a3f9555b8e68f5baae6cf4a50
completion:
  status: COMPLETED
  attempt_num: 1
  receipt_id: rcpt_e11b6225025e
  commit_sha: 5fb4210d646d9c7abf9f877af0a8cb8d4bc6b905
  files_modified:
  - scripts/check_doc_cpp_samples.py
  verification_summary: pytest 93/93, doc_lint exit 0, check_doc_cpp_samples.py 49
    contracts + 41 samples PASSED, 130 SKIPPED, 0 FAILED (7.89s default, 7.33s clang++,
    7.97s g++), --dump 401 & fallback verified; unstaged for Coordinator receipt stamping.
  out_of_scope_findings: []
  escalation_request: null
  decisions_made:
  - 'Fixed #include "WiFiManager.hpp" -> #include "WifiManager.hpp" in BASE_PREAMBLE
    to match modules/WifiManager/WifiManager.hpp on case-sensitive Linux.'
  - 'Parameterized synthesize_contract_candidate, synthesize_method_candidate, synthesize_runner_candidate,
    and check_single_block with preamble: str = BASE_PREAMBLE so --dump preserves
    full preamble output.'
  - Added one-time doc_preamble.hpp PCH precompilation in main() inside tempfile.TemporaryDirectory(),
    stripping ["-x", "c++", "-"] and omitting -fsyntax-only during precompile for
    clang++ (.pch/-include-pch) and g++ (.gch/-include) with safe fallback to BASE_PREAMBLE.
  dead_ends: []
---

# WO-144.5 — Fix WifiManager.hpp header casing and add PCH acceleration to check_doc_cpp_samples.py

## 1. Objective & Scope

Resolve the Linux CI failure and slow compile-check execution surfaced in GitHub Actions run `43877922271` on `ubuntu-24.04` (`Compile-check documentation C++ samples`):
1. `BASE_PREAMBLE` at `scripts/check_doc_cpp_samples.py:100` uses `#include "WiFiManager.hpp"` (capital `F`), which passes on case-insensitive macOS APFS but fails on case-sensitive Linux (`modules/WifiManager/WifiManager.hpp` has lowercase `f`).
2. `scripts/check_doc_cpp_samples.py` re-parses `<ArduinoJson.h>`, `<lvgl.h>`, and all framework headers across every block and candidate (`838.66s` in Linux CI). Precompiling `BASE_PREAMBLE` once per invocation into a temporary `.pch` (`clang++`) or `.gch` (`g++`) with clean fallback eliminates redundant header parsing without altering `--dump` output or skip/pass counts.

Scope is strictly bounded to `scripts/check_doc_cpp_samples.py`.

## 2. Detailed Specification

### 2.1 `BASE_PREAMBLE` Include Casing (`scripts/check_doc_cpp_samples.py:100`)

- Change `#include "WiFiManager.hpp"` to `#include "WifiManager.hpp"` (matching `modules/WifiManager/WifiManager.hpp`).

### 2.2 Precompiled Header (`PCH`) Acceleration with Safe Fallback (`scripts/check_doc_cpp_samples.py`)

1. **Imports:**
   - Import `tempfile` in the stdlib imports block.
2. **Candidate Synthesis & `check_single_block` Signature:**
   - Add optional parameter `preamble: str = BASE_PREAMBLE` to:
     - `synthesize_contract_candidate(code: str, preamble: str = BASE_PREAMBLE) -> str`
     - `synthesize_method_candidate(code: str, preamble: str = BASE_PREAMBLE) -> Optional[str]`
     - `synthesize_runner_candidate(code: str, preamble: str = BASE_PREAMBLE) -> str`
     - `check_single_block(block: CodeBlock, compiler_cmd: list[str], strict: bool = False, preamble: str = BASE_PREAMBLE) -> CheckResult`
   - Inside all three `synthesize_*` helpers and the `"SAMPLE"` candidate in `check_single_block` (`f"{preamble}\n{code}\n"`), interpolate `{preamble}` instead of `{BASE_PREAMBLE}`. Keeping `BASE_PREAMBLE` as the default preserves `--dump` output and standalone caller compatibility.
3. **One-Time PCH Precompilation in `main()`:**
   - Wrap the `ThreadPoolExecutor` compilation phase in `with tempfile.TemporaryDirectory() as tmp_dir:`:
     - Write `preamble_path = Path(tmp_dir) / "doc_preamble.hpp"` with content `"#pragma once\n" + BASE_PREAMBLE` (`encoding="utf-8"`).
     - Note that `build_compiler_cmd()` appends 3 trailing tokens `["-x", "c++", "-"]` (not 2) and includes `"-fsyntax-only"`:
       - Strip the trailing `["-x", "c++", "-"]` (`base_flags = compiler_cmd[:-3]` when `compiler_cmd[-3:] == ["-x", "c++", "-"]`, else `list(compiler_cmd)`).
       - Build `pch_flags = [f for f in base_flags if f != "-fsyntax-only"]` for the header precompilation step (`-fsyntax-only` suppresses emitting `.pch`/`.gch` output files on `clang++`/`g++`).
     - Initialize fallback defaults: `effective_cmd = compiler_cmd` and `effective_preamble = BASE_PREAMBLE`.
     - Determine `is_clang = "clang" in Path(compiler_path).name.lower()` and run a single precompile (`timeout=30`, catching `(subprocess.SubprocessError, OSError)` so any failure falls back cleanly):
       - If `is_clang`: build `pch_file = f"{preamble_path}.pch"` via `[*pch_flags, "-x", "c++-header", str(preamble_path), "-o", pch_file]`. When `proc.returncode == 0 and Path(pch_file).is_file()`, set `effective_cmd = [*base_flags, "-include-pch", pch_file, "-x", "c++", "-"]` and `effective_preamble = ""`.
       - Else (`g++`): build `gch_file = f"{preamble_path}.gch"` via `[*pch_flags, "-x", "c++-header", str(preamble_path), "-o", gch_file]`. When `proc.returncode == 0 and Path(gch_file).is_file()`, set `effective_cmd = [*base_flags, "-include", str(preamble_path), "-x", "c++", "-"]` and `effective_preamble = ""`.
     - Submit `executor.submit(check_single_block, b, effective_cmd, args.strict, effective_preamble)` across all extracted blocks.

## 3. Verification Requirements

1. **Mandatory Verification Command:**
   - `pytest scripts/tests/ && python3 scripts/doc_lint.py && python3 scripts/check_doc_cpp_samples.py`
   - Confirm `check_doc_cpp_samples.py` reports exact expected counts: `49` contracts PASSED, `41` samples PASSED, `130` SKIPPED, `0` FAILED (and verify `--dump` still emits `BASE_PREAMBLE` cleanly).
2. **Boundary & Handoff Hygiene:**
   - Keep all edits strictly inside `allowed_files` (`scripts/check_doc_cpp_samples.py`) plus `WORKORDER.md` Part II.
   - Leave edits unstaged (`status: ESCALATED_TO_PA`, `completion.verification_summary <= 280` chars) for Coordinator verification and central receipt stamping.

## Completion Report

Implemented all `WO-144.5` changes strictly inside `scripts/check_doc_cpp_samples.py` and left all edits unstaged for Coordinator verification and central receipt stamping (`pa wo verify` and `git add`/`git commit` not run per dispatch instructions):

- **`scripts/check_doc_cpp_samples.py` (`L26`, `L101`):** Added `import tempfile` and fixed `#include "WiFiManager.hpp"` -> `#include "WifiManager.hpp"` in `BASE_PREAMBLE` to match `modules/WifiManager/WifiManager.hpp` on case-sensitive Linux.
- **`scripts/check_doc_cpp_samples.py` (`L348`, `L393`, `L435`, `L468-L545`):** Added `preamble: str = BASE_PREAMBLE` to `synthesize_contract_candidate`, `synthesize_method_candidate`, `synthesize_runner_candidate`, and `check_single_block`, replacing hardcoded `{BASE_PREAMBLE}` with `{preamble}` while keeping `BASE_PREAMBLE` as default so `--dump` output is unchanged.
- **`scripts/check_doc_cpp_samples.py` (`L669-L716`):** Added one-time `doc_preamble.hpp` PCH precompilation inside `with tempfile.TemporaryDirectory() as tmp_dir:` in `main()`, stripping trailing `["-x", "c++", "-"]` (`base_flags`) and omitting `-fsyntax-only` (`pch_flags`) during precompile (`timeout=30`), using `.pch` + `-include-pch` for `clang` and `.gch` + `-include` for `g++`, verifying `proc.returncode == 0 and Path(pch_out).is_file()`, and falling back safely to `compiler_cmd` + `BASE_PREAMBLE` on any failure.

### Verification Results
- `pytest scripts/tests/`: `93 passed in 0.32s` (exit `0`).
- `python3 scripts/doc_lint.py`: exit `0`.
- `python3 scripts/check_doc_cpp_samples.py`: `Scanned files: 34`, `Total C++ blocks: 220`, `Contracts verified: 49 PASSED`, `Samples verified: 41 PASSED`, `Skipped blocks: 130 SKIPPED`, `Failed blocks: 0 FAILED`, `Elapsed time: 7.89s` (baseline before PCH: `22.27s`).
- `python3 scripts/check_doc_cpp_samples.py --compiler clang++`: `49` contracts + `41` samples PASSED, `130` SKIPPED, `0` FAILED (`7.33s`).
- `python3 scripts/check_doc_cpp_samples.py --compiler g++`: `49` contracts + `41` samples PASSED, `130` SKIPPED, `0` FAILED (`7.97s`).
- `python3 scripts/check_doc_cpp_samples.py --dump 401 docs/design_specification.md`: emitted full `BASE_PREAMBLE` including `#include "WifiManager.hpp"` and `FrameworkConfig`.
- Simulated precompile failure fallback check (`docs/developer/creating-a-module.md`): `PCH precompilation: fallback to BASE_PREAMBLE`, `4 PASSED`, `0 FAILED` (exit `0`).
- `git diff --cached --name-only`: empty (all changes unstaged).
