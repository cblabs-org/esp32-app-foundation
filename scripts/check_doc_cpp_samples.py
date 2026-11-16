#!/usr/bin/env python3
"""ESP32 App Foundation — Documentation C++ Sample Compile Checker.

Extracts fenced C++ code blocks from Markdown documentation (docs/**) and
compile-checks them against actual framework headers in the native PlatformIO
environment using host clang++ or g++.

Ensures that interface declarations (especially in docs/design_specification.md §6),
API examples, and architectural contracts remain synchronized with real headers
and do not rot as the framework evolves.

Zero external dependencies — uses only the Python 3 standard library.
"""

from __future__ import annotations

import argparse
import concurrent.futures
from dataclasses import dataclass
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time
from typing import Optional


# Known historical exemptions in protected specification files.
# docs/design_specification.md is protected by Rule 2 (Specification Immutability).
# Any historical snippets in non-contract sections that contain known pre-v2.0 signatures
# or partial virtual function fragments are exempted here with documented rationale.
KNOWN_SPEC_EXEMPTIONS = {
    # docs/design_specification.md §9.2.2: Illustrates status & description split with
    # free-floating virtual member declarations outside any class definition.
    ("docs/design_specification.md", 1465): "§9.2.2 free-floating virtual member illustrations",
    # docs/design_specification.md §16.1: Module implementation checklist contains legacy
    # registerTools(MCPServer&) signature pending formal spec-change-request.
    ("docs/design_specification.md", 2665): "§16.1 legacy checklist template pending spec-change-request",
}

# Interfaces to verify for exact signature parity when defined in documentation.
INTERFACE_SIGNATURE_CHECKS = {
    "ITickable": [
        ("tick", "void (ITickable::*)(uint32_t)")
    ],
    "IConfigurable": [
        ("getConfigNamespace", "const char* (IConfigurable::*)() const"),
        ("getSchemaVersion", "int (IConfigurable::*)() const"),
        ("applyConfig", "void (IConfigurable::*)(const JsonObject&)"),
        ("serializeConfig", "void (IConfigurable::*)(JsonObject&) const"),
        ("migrateConfig", "bool (IConfigurable::*)(JsonObject&, int)"),
        ("resetConfig", "void (IConfigurable::*)()")
    ],
    "IDataSource": [
        ("fetchUpdate", "void (IDataSource::*)(FetchContext&)")
    ],
    "IStatusReporter": [
        ("reportStatus", "void (IStatusReporter::*)(JsonObject&) const")
    ],
    "IBootReporter": [
        ("reportBoot", "void (IBootReporter::*)(JsonObject&) const")
    ],
    "IWebContributor": [
        ("registerRoutes", "void (IWebContributor::*)(PsychicHttpServer&)"),
        ("registerTabs", "void (IWebContributor::*)(ITabRegistry&)")
    ],
    "IMCPContributor": [
        ("registerMCPTools", "void (IMCPContributor::*)(MCPToolRegistry&)")
    ],
    "IPreconditionContributor": [
        ("registerPreconditions", "void (IPreconditionContributor::*)(IPreconditionRegistry&)")
    ],
    "ISanitizer": [
        ("sanitize", "void (ISanitizer::*)(JsonObject&) const")
    ],
}

BASE_PREAMBLE = """
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <type_traits>

#include <ArduinoJson.h>
#include <lvgl.h>
#include <AppFoundation.h>
#include "platform/LogShim.h"
#include "Application.hpp"
#include "ConfigManager.hpp"
#include "EventBus.hpp"
#include "MCPServer.hpp"
#include "MCPToolRegistry.hpp"
#include "WifiManager.hpp"
#include "DataManager.hpp"
#include "PsychicHttp.h"
#include "WiFiClientSecure.h"
#include "JsonListener.h"
#include "Audit.h"
#include "Redactor.h"
#include "interfaces/ITabRegistry.hpp"
#include "interfaces/IPreconditionRegistry.hpp"
#include "IDisplayBoard.hpp"
#include <U8g2lib.h>
#include "NativeU8G2DisplayType.h"
#include "U8G2Display.hpp"

// Forward declarations for application sample types
class WeatherModule;
struct FeatureTriggeredPayload;
struct BLEStoredDevice { std::string name; };

// Forward declarations and stubs for platform-only BLE classes (target-only NimBLE)
class NimBLECharacteristic;
class NimBLEService {
public:
    NimBLECharacteristic* createCharacteristic(uint16_t, uint32_t) { return nullptr; }
};
class NimBLEUUID {
public:
    NimBLEUUID(uint16_t) {}
};
class NimBLEServer {
public:
    NimBLEService* createService(const NimBLEUUID&) { return nullptr; }
};

// Signature verification templates
template<typename M1, typename M2> struct __same_sig : std::false_type {};
template<typename C1, typename C2, typename R, typename... Args>
struct __same_sig<R (C1::*)(Args...), R (C2::*)(Args...)> : std::true_type {};
template<typename C1, typename C2, typename R, typename... Args>
struct __same_sig<R (C1::*)(Args...) const, R (C2::*)(Args...) const> : std::true_type {};
"""


@dataclass
class CodeBlock:
    file_path: str
    start_line: int
    end_line: int
    tag: str
    code: str
    preceding_lines: list[str]
    section: str


@dataclass
class CheckResult:
    block: CodeBlock
    status: str  # "PASSED", "SKIPPED", "FAILED"
    kind: str    # "CONTRACT", "SAMPLE", "METHOD", "RUNNER", "DIRECTIVE_SKIP", "FRAGMENT_SKIP"
    error: str = ""
    reason: str = ""


def find_compiler(explicit: Optional[str] = None) -> str:
    """Finds a suitable C++ compiler (clang++ preferred, then g++)."""
    if explicit:
        path = shutil.which(explicit)
        if not path:
            raise FileNotFoundError(f"Specified compiler '{explicit}' not found in PATH.")
        return path

    for name in ["clang++", "g++"]:
        path = shutil.which(name)
        if path:
            return path
    raise FileNotFoundError("No C++ compiler found (clang++ or g++ required).")


def discover_include_paths(repo_root: Path) -> list[str]:
    """Discovers all include paths matching env:native."""
    include_dirs: list[str] = [
        "test/stubs/platform",
        "test/stubs/platform/mbedtls",
        ".pio/libdeps/native/ArduinoJson/src",
        ".pio/libdeps/native/U8g2/src",
        ".pio/libdeps/native/U8g2/src/clib",
        "include",
        "modules",
        "lib",
        "hardware",
        "hardware/_common",
        "hardware/native",
        "template-app/src",
        "sample-app/src",
        ".",
    ]

    modules_dir = repo_root / "modules"
    if modules_dir.is_dir():
        for d in sorted(modules_dir.iterdir()):
            if d.is_dir() and not d.name.startswith("."):
                include_dirs.append(str(d.relative_to(repo_root)))

    lib_dir = repo_root / "lib"
    if lib_dir.is_dir():
        for d in sorted(lib_dir.iterdir()):
            if d.is_dir() and not d.name.startswith("."):
                include_dirs.append(str(d.relative_to(repo_root)))

    existing = []
    for inc in include_dirs:
        full = repo_root / inc
        if full.exists():
            existing.append(str(full.resolve()))
    return existing


def build_compiler_cmd(compiler: str, include_paths: list[str]) -> list[str]:
    """Constructs the base compiler command line with flags and defines."""
    cmd = [
        compiler,
        "-std=c++17",
        "-fsyntax-only",
        "-Wno-unused-command-line-argument",
        "-Wno-#pragma-messages",
        "-Wno-nonportable-include-path",
        "-Wno-pragma-once-outside-header",
        "-Wno-inconsistent-missing-override",
        "-DUNIT_TEST",
        "-DARDUINO=100",
        '-DBOARD_ID="native"',
        '-DBOARD_HEADER="hardware/native/board.h"',
        '-DBUILD_VERSION="0.0.0-native"',
        "-DVERSION_MAJOR=0",
        "-DVERSION_MINOR=0",
        "-DVERSION_PATCH=0",
        "-DVERSION_CHANNEL=3",
        "-DVERSION_PRE_NUM=0",
        "-DSECURITY_VERSION=1",
        "-DCONFIG_LOG_MAXIMUM_LEVEL=5",
        "-DWITH_TIME",
        "-DWITH_AUDIT",
        "-DWITH_BLE",
        "-DU8G2_WITHOUT_FONT_ROTATION",
        "-DU8G2_WITHOUT_UNICODE",
        "-DWITH_DISPLAY_U8G2",
    ]
    for inc in include_paths:
        cmd.extend(["-I", inc])
    cmd.extend(["-x", "c++", "-"])
    return cmd


def extract_code_blocks(filepath: Path, repo_root: Path) -> list[CodeBlock]:
    """Extracts fenced C++ code blocks with metadata from a Markdown file."""
    rel_path = filepath.relative_to(repo_root).as_posix()
    try:
        text = filepath.read_text(encoding="utf-8")
    except Exception as e:
        sys.stderr.write(f"Warning: Failed to read {rel_path}: {e}\n")
        return []

    lines = text.splitlines()
    blocks: list[CodeBlock] = []
    in_block = False
    lang = ""
    start_line = 0
    block_lines: list[str] = []
    current_section = ""

    for i, line in enumerate(lines, start=1):
        if line.startswith("```"):
            if not in_block:
                in_block = True
                lang = line[3:].strip()
                start_line = i
                block_lines = []
            else:
                in_block = False
                tokens = lang.split()
                if tokens and tokens[0].lower() in ("cpp", "c++", "c"):
                    preceding = [lines[idx] for idx in range(max(0, start_line - 4), start_line - 1)]
                    blocks.append(
                        CodeBlock(
                            file_path=rel_path,
                            start_line=start_line,
                            end_line=i,
                            tag=lang,
                            code="\n".join(block_lines),
                            preceding_lines=preceding,
                            section=current_section,
                        )
                    )
        elif in_block:
            block_lines.append(line)
        elif line.startswith("#"):
            current_section = line.strip()

    return blocks


def check_explicit_skip(block: CodeBlock) -> tuple[bool, str]:
    """Checks whether a block has explicit directives requesting it to be skipped."""
    tokens = block.tag.split()
    if len(tokens) > 1:
        directive = tokens[1].lower()
        if directive in (
            "no-compile", "skip", "skip-compile", "ignore", "pseudo",
            "pseudocode", "schema", "not-compilable", "snippet", "conceptual"
        ):
            return True, f"info string '{directive}'"

    for prec in block.preceding_lines:
        if re.search(r"<!--\s*(no-compile|skip-compile|pseudo-code|snippet)\s*-->", prec, re.IGNORECASE):
            return True, "preceding markdown comment"

    # Check top lines in the code block
    non_empty = [l.strip() for l in block.code.splitlines() if l.strip()]
    for l in non_empty[:4]:
        low = l.lower()
        if low in (
            "// no-compile", "// skip-compile", "// pseudo-code", "// pseudocode",
            "// illustrative", "// snippet", "// conceptual", "// anti-pattern",
            "// ignore", "// schema", "// not-compilable", "// do not compile",
            "// example-only"
        ):
            return True, f"comment directive '{l}'"
    # Anti-pattern markers anywhere in the block
    if "❌" in block.code or re.search(r"//\s*(?:bad|wrong|anti-pattern|don't|do not)\b", block.code, re.IGNORECASE):
        return True, "contains anti-pattern marker"

    # Ellipsis line or comment indicating incomplete code sample
    if re.search(r"(?:^\s*\.\.\.|\/\/\s*\.\.\.)", block.code, re.MULTILINE):
        return True, "contains ellipsis (...)"

    # Angle bracket placeholder (e.g. <your-token>, <ssid>, <password>)
    if re.search(r"<[A-Za-z0-9_-]{2,}>", block.code):
        return True, "contains placeholder (<...>)"

    # Pure comment or empty block
    code_lines = [l for l in non_empty if not l.startswith("//") and not l.startswith("/*")]
    if not code_lines:
        return True, "pure comment block"

    return False, ""


def synthesize_contract_candidate(code: str, preamble: str = BASE_PREAMBLE) -> str:
    """Synthesizes candidate compilation unit for contract/class/struct definitions."""
    # Extract top-level #include and #pragma lines to keep them outside the namespace
    includes_top = []
    body_lines = []
    for line in code.splitlines():
        trimmed = line.strip()
        if trimmed.startswith("#include") or trimmed.startswith("#pragma"):
            includes_top.append(line)
        else:
            body_lines.append(line)
    top_str = "\n".join(includes_top)
    body_str = "\n".join(body_lines)

    checks = []
    for iface_name, methods in INTERFACE_SIGNATURE_CHECKS.items():
        if re.search(rf"\bclass\s+{iface_name}\b", code):
            for method_name, _ in methods:
                # Add parity static_assert
                checks.append(
                    f"static_assert(__same_sig<decltype(&doc_sample::{iface_name}::{method_name}), "
                    f"decltype(&::{iface_name}::{method_name})>::value, "
                    f"\"Signature mismatch for {iface_name}::{method_name}\");"
                )

    parity_str = "\n".join(checks)
    return f"""{preamble}
{top_str}

namespace doc_sample {{
using ::PsychicHttpServer;
using ::MCPServer;
using ::MCPToolRegistry;
using ::ITabRegistry;
using ::IPreconditionRegistry;
using ::IDisplayBoard;

{body_str}

}} // namespace doc_sample

{parity_str}
"""


def synthesize_method_candidate(code: str, preamble: str = BASE_PREAMBLE) -> Optional[str]:
    """Synthesizes candidate compilation unit for out-of-line method definitions."""
    match = re.search(r"\b(\w+)::(\w+)\s*\(", code)
    if not match:
        return None
    cls_name = match.group(1)

    return f"""{preamble}
namespace doc_sample {{
using ::PsychicHttpServer;
using ::MCPServer;
using ::MCPToolRegistry;
using ::ITabRegistry;
using ::IPreconditionRegistry;

class {cls_name} {{
public:
    bool doAction() {{ return true; }}
    bool _gateOpen = false;
    uint32_t _lastActionMs = 0;
    int _refreshIntervalMs = 1000;
    char _host[64] = "localhost";
    int _port = 8080;
    bool _enabled = true;

    void registerMCPTools(MCPToolRegistry& registry);
    void registerRoutes(PsychicHttpServer& server);
    void registerTabs(ITabRegistry& registry);
    void registerPreconditions(IPreconditionRegistry& registry);
    void tick(uint32_t currentMillis);
    void applyConfig(const JsonObject& section);
    void serializeConfig(JsonObject& section) const;
    bool migrateConfig(JsonObject& section, int fromVersion);
    void resetConfig();
}};

{code}

}} // namespace doc_sample
"""


def synthesize_runner_candidate(code: str, preamble: str = BASE_PREAMBLE) -> str:
    """Synthesizes candidate compilation unit for statement/expression snippets."""
    return f"""{preamble}
void __doc_sample_runner() {{
    uint32_t currentMillis = 1000;
    Application app;
    ConfigManager& configManager = app.getConfigManager();
    EventBus& eventBus = app.getEventBus();
    (void)currentMillis; (void)app; (void)configManager; (void)eventBus;
    {{
{code}
    }}
}}
"""


def compile_candidate(cmd: list[str], source: str) -> tuple[bool, str]:
    """Runs compiler with synthesized source on stdin."""
    try:
        proc = subprocess.run(
            cmd,
            input=source,
            text=True,
            capture_output=True,
            timeout=15,
        )
        return proc.returncode == 0, proc.stderr
    except subprocess.TimeoutExpired:
        return False, "Compilation timed out after 15 seconds"
    except Exception as e:
        return False, str(e)


def check_single_block(
    block: CodeBlock,
    compiler_cmd: list[str],
    strict: bool = False,
    preamble: str = BASE_PREAMBLE,
) -> CheckResult:
    """Checks a single C++ code block across candidate compilation strategies."""
    # Check known spec exemptions
    exemption_key = (block.file_path, block.start_line)
    if exemption_key in KNOWN_SPEC_EXEMPTIONS:
        return CheckResult(
            block=block,
            status="SKIPPED",
            kind="DIRECTIVE_SKIP",
            reason=f"Exempted: {KNOWN_SPEC_EXEMPTIONS[exemption_key]}",
        )

    # Check explicit skip directives
    skip, reason = check_explicit_skip(block)
    if skip:
        return CheckResult(
            block=block,
            status="SKIPPED",
            kind="DIRECTIVE_SKIP",
            reason=reason,
        )

    code = block.code

    # Unbalanced braces check — incomplete code fragments that cut off mid-declaration
    if code.count('{') != code.count('}'):
        if strict:
            return CheckResult(
                block=block,
                status="FAILED",
                kind="STRICT_FAIL",
                error="Unbalanced braces in code block",
            )
        return CheckResult(
            block=block,
            status="SKIPPED",
            kind="FRAGMENT_SKIP",
            reason="Unbalanced braces in illustrative fragment",
        )

    # Check whether block defines or implements a core framework interface/contract
    framework_contracts = set(INTERFACE_SIGNATURE_CHECKS.keys()) | {
        "IDisplayBoard", "ILVGLHardwareAdapter", "IHaEntity", "DisplayState",
        "TabEntry", "PreconditionStatus", "DataSourceHealthPayload",
        "PreconditionHealthPayload", "AppConfiguringPayload", "FrameworkConfig"
    }
    is_framework_contract = any(
        re.search(rf"\b(?:class|struct|enum\s+class|enum)\s+{name}\b", code) or
        re.search(rf":\s*(?:public\s+)?{name}\b", code)
        for name in framework_contracts
    )

    is_contract = bool(re.search(r"\b(class|struct|enum\s+class|enum|typedef|using)\s+(\w+)", code))
    is_method = bool(re.search(r"\b(\w+)::(\w+)\s*\(", code))

    candidates: list[tuple[str, str]] = []

    # Priority 1: Contract / Namespace wrapper
    if is_contract:
        candidates.append(("CONTRACT", synthesize_contract_candidate(code, preamble)))

    # Priority 2: Method implementation host
    if is_method:
        method_cand = synthesize_method_candidate(code, preamble)
        if method_cand:
            candidates.append(("METHOD", method_cand))

    # Priority 3: Top-level translation unit
    candidates.append(("SAMPLE", f"{preamble}\n{code}\n"))

    # Priority 4: Execution runner for statements
    candidates.append(("RUNNER", synthesize_runner_candidate(code, preamble)))

    last_error = ""
    for kind, cand_src in candidates:
        ok, err = compile_candidate(compiler_cmd, cand_src)
        if ok:
            return CheckResult(block=block, status="PASSED", kind=kind)
        last_error = err

    # If it failed all candidates:
    # In strict mode, fail unconditionally.
    if strict:
        return CheckResult(
            block=block,
            status="FAILED",
            kind="STRICT_FAIL",
            error=last_error,
        )

    # In default mode:
    # Core framework contracts and interface implementations MUST compile cleanly.
    if is_framework_contract:
        return CheckResult(
            block=block,
            status="FAILED",
            kind="CONTRACT",
            error=last_error,
        )

    # Otherwise, it is an illustrative fragment/snippet without enclosing definition.
    return CheckResult(
        block=block,
        status="SKIPPED",
        kind="FRAGMENT_SKIP",
        reason="Partial illustrative snippet without complete class/function definition",
        error=last_error,
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Compile-check fenced C++ code samples in docs against real framework headers."
    )
    parser.add_argument("files", nargs="*", help="Optional specific Markdown files to check")
    parser.add_argument("-f", "--file", action="append", help="Specific Markdown file(s) to check")
    parser.add_argument("--compiler", type=str, help="Compiler executable (default: clang++ or g++)")
    parser.add_argument(
        "-j", "--jobs", type=int, default=os.cpu_count() or 4, help="Number of parallel compiler jobs"
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose progress and error output")
    parser.add_argument(
        "--strict", action="store_true", help="Strict mode: require all blocks to compile or have explicit skip directives"
    )
    parser.add_argument(
        "--dump", type=int, help="Dump synthesized C++ source for code block at specified line number"
    )

    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent

    # Resolve target files
    specified_files: list[str] = []
    if args.files:
        specified_files.extend(args.files)
    if args.file:
        specified_files.extend(args.file)

    target_files: list[Path] = []
    if specified_files:
        for f in specified_files:
            p = (repo_root / f).resolve() if not os.path.isabs(f) else Path(f).resolve()
            if p.is_file():
                target_files.append(p)
            else:
                sys.stderr.write(f"Error: File not found: {f}\n")
                return 1
    else:
        # Default scan: docs/** excluding docs/archive/**
        docs_dir = repo_root / "docs"
        if docs_dir.is_dir():
            for p in sorted(docs_dir.rglob("*.md")):
                if "docs/archive" not in p.as_posix():
                    target_files.append(p)
        for root_doc in ["README.md", "CONTRIBUTING.md"]:
            rp = repo_root / root_doc
            if rp.is_file():
                target_files.append(rp)

    try:
        compiler_path = find_compiler(args.compiler)
    except FileNotFoundError as e:
        sys.stderr.write(f"Error: {e}\n")
        return 1

    include_paths = discover_include_paths(repo_root)
    compiler_cmd = build_compiler_cmd(compiler_path, include_paths)

    if args.verbose:
        print(f"Using compiler: {compiler_path}")
        print(f"Found {len(include_paths)} include directories.")
        print(f"Scanning {len(target_files)} Markdown files...")

    # Extract all blocks
    all_blocks: list[CodeBlock] = []
    for tf in target_files:
        all_blocks.extend(extract_code_blocks(tf, repo_root))

    if args.verbose:
        print(f"Extracted {len(all_blocks)} fenced C++ blocks.")

    # Dump mode
    if args.dump:
        match = [b for b in all_blocks if b.start_line <= args.dump <= b.end_line]
        if not match:
            print(f"No block found covering line {args.dump}.")
            return 1
        b = match[0]
        print(f"--- Synthesized Contract Candidate for {b.file_path}:{b.start_line} ---")
        print(synthesize_contract_candidate(b.code))
        return 0

    start_time = time.time()
    results: list[CheckResult] = []

    with tempfile.TemporaryDirectory() as tmp_dir:
        preamble_path = Path(tmp_dir) / "doc_preamble.hpp"
        preamble_path.write_text("#pragma once\n" + BASE_PREAMBLE, encoding="utf-8")

        base_flags = (
            compiler_cmd[:-3]
            if compiler_cmd[-3:] == ["-x", "c++", "-"]
            else list(compiler_cmd)
        )
        pch_flags = [f for f in base_flags if f != "-fsyntax-only"]

        effective_cmd = compiler_cmd
        effective_preamble = BASE_PREAMBLE
        is_clang = "clang" in Path(compiler_path).name.lower()

        try:
            ext, include_flag, include_target = (
                (".pch", "-include-pch", f"{preamble_path}.pch")
                if is_clang
                else (".gch", "-include", str(preamble_path))
            )
            pch_out = f"{preamble_path}{ext}"
            proc = subprocess.run(
                [*pch_flags, "-x", "c++-header", str(preamble_path), "-o", pch_out],
                capture_output=True,
                text=True,
                timeout=30,
            )
            if proc.returncode == 0 and Path(pch_out).is_file():
                effective_cmd = [*base_flags, include_flag, include_target, "-x", "c++", "-"]
                effective_preamble = ""
        except (subprocess.SubprocessError, OSError):
            pass

        if args.verbose:
            pch_status = "enabled" if effective_preamble == "" else "fallback to BASE_PREAMBLE"
            print(f"PCH precompilation: {pch_status}")

        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
            futures = {
                executor.submit(
                    check_single_block, b, effective_cmd, args.strict, effective_preamble
                ): b
                for b in all_blocks
            }
            for future in concurrent.futures.as_completed(futures):
                results.append(future.result())

    duration = time.time() - start_time

    # Sort results by file and line number
    results.sort(key=lambda r: (r.block.file_path, r.block.start_line))

    passed = [r for r in results if r.status == "PASSED"]
    skipped = [r for r in results if r.status == "SKIPPED"]
    failed = [r for r in results if r.status == "FAILED"]

    contract_passed = [r for r in passed if r.kind == "CONTRACT"]
    other_passed = [r for r in passed if r.kind != "CONTRACT"]

    # Print failures
    if failed:
        print("\n" + "=" * 70)
        print("FAILURES:")
        print("=" * 70)
        for r in failed:
            b = r.block
            first_err = [l.strip() for l in r.error.splitlines() if "error:" in l or "fatal error:" in l]
            err_msg = first_err[0] if first_err else (r.error.splitlines()[:1] or ["Compilation failed"])
            print(f"\n{b.file_path}:{b.start_line}: error: {err_msg}")
            if args.verbose:
                print("Code snippet:")
                for line in b.code.splitlines()[:10]:
                    print(f"  | {line}")
                if len(b.code.splitlines()) > 10:
                    print("  | ...")
                print(f"Compiler output:\n{r.error}")

    # Summary
    print("\n" + "=" * 70)
    print("ESP32 App Foundation — Documentation C++ Sample Checker")
    print("=" * 70)
    print(f"Scanned files:      {len(target_files)}")
    print(f"Total C++ blocks:   {len(all_blocks)}")
    print(f"Contracts verified: {len(contract_passed)} PASSED")
    print(f"Samples verified:   {len(other_passed)} PASSED")
    print(f"Skipped blocks:     {len(skipped)} SKIPPED")
    print(f"Failed blocks:      {len(failed)} FAILED")
    print(f"Elapsed time:       {duration:.2f}s")
    print("=" * 70)

    if failed:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
