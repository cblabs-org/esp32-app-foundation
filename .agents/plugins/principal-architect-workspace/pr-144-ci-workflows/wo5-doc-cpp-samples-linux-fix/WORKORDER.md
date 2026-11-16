---
schema_version: 1
wo_id: WO-144.5
pr_id: PR-144
title: Fix WifiManager.hpp header casing and add PCH acceleration to check_doc_cpp_samples.py
complexity: standard
status: PENDING
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
completion: null
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

*(Pending execution by Tier-3 Worker.)*
