#!/usr/bin/env python3
"""
ESP32 App Foundation (c) 2026 CB Labs

https://github.com/cblabs-org/esp32-app-foundation

SPDX-License-Identifier: Apache-2.0

Module: scripts/check_newlib_printf.py
Description: Static checker preventing C99/64-bit printf format modifiers
             (%z*, %j*, %t*, %ll*, PRI*64/LEAST64/FAST64/MAX) that are
             unsupported under CONFIG_NEWLIB_NANO_FORMAT=y.

             Usage:
                 python3 scripts/check_newlib_printf.py
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence

SOURCE_ROOTS: tuple[str, ...] = (
    "src",
    "include",
    "lib",
    "modules",
    "adapters",
    "hardware",
    "sample-app/src",
    "sample-app/include",
    "sample-app/lib",
    "template-app/src",
    "template-app/include",
    "template-app/lib",
)

SOURCE_EXTENSIONS: tuple[str, ...] = (".c", ".cpp", ".h", ".hpp", ".ipp")
SKIP_DIRS: frozenset[str] = frozenset({"test", ".pio"})

FORMAT_RE = re.compile(
    r"%%|(%(?:[-+#0 ]*(?:\*|\d+)?(?:\.(?:\*|\d+))?(?:ll|[zjt])[diouxX]))"
)
PRI_RE = re.compile(r"\bPRI[diouxX](?:64|LEAST64|FAST64|MAX)\b")

_RAW_STRING_PREFIXES: frozenset[str] = frozenset({"", "u8", "u", "U", "L"})


@dataclass(frozen=True)
class Violation:
    """Represents a single Newlib-nano unsupported format specifier finding."""

    path: str
    line: int
    token: str
    detail: str

    def format(self) -> str:
        return f"{self.path}:{self.line}: {self.detail}"

    def __str__(self) -> str:
        return self.format()


def _is_ident_char(ch: str) -> bool:
    return ch.isalnum() or ch == "_"


def _is_raw_string_start(text: str, idx: int) -> bool:
    """Return True if text[idx:idx+2] == 'R\"' starts a C++ raw string literal."""
    if idx + 1 >= len(text) or text[idx] != "R" or text[idx + 1] != '"':
        return False
    j = idx - 1
    while j >= 0 and _is_ident_char(text[j]):
        j -= 1
    prefix = text[j + 1 : idx]
    return prefix in _RAW_STRING_PREFIXES


def _is_digit_separator(text: str, idx: int) -> bool:
    """Return True if text[idx] == \"'\" is a C++14 numeric digit separator."""
    if idx == 0 or idx + 1 >= len(text):
        return False
    prev_ch = text[idx - 1]
    next_ch = text[idx + 1]
    if not (prev_ch in "0123456789abcdefABCDEF" and next_ch in "0123456789abcdefABCDEF"):
        return False
    # Walk backward across the numeric literal token to ensure it began with 0-9
    # (so u8'a' is not confused with a digit separator).
    j = idx - 1
    while j >= 0 and (text[j].isalnum() or text[j] == "'"):
        j -= 1
    token_start = j + 1
    return text[token_start].isdigit()


def _scan_string_interior(
    interior: str, start_line: int, relpath: str, violations: list[Violation]
) -> None:
    for match in FORMAT_RE.finditer(interior):
        spec = match.group(1)
        if spec is None:
            continue
        line_no = start_line + interior[: match.start()].count("\n")
        violations.append(
            Violation(
                path=relpath,
                line=line_no,
                token=spec,
                detail=(
                    f"unsupported Newlib-nano printf specifier '{spec}' "
                    "(use %u with static_cast<unsigned>(...) or 32-bit format)"
                ),
            )
        )


def scan_text(text: str, relpath: str = "<string>") -> list[Violation]:
    """Scan C/C++ source text for Newlib-nano unsupported format specifiers."""
    violations: list[Violation] = []
    code_chars: list[str] = []
    n = len(text)
    i = 0
    line = 1

    while i < n:
        ch = text[i]

        # Line comment // ...
        if ch == "/" and i + 1 < n and text[i + 1] == "/":
            i += 2
            while i < n:
                if text[i] == "\n":
                    continued = i > 0 and text[i - 1] == "\\"
                    code_chars.append("\n")
                    line += 1
                    i += 1
                    if not continued:
                        break
                else:
                    i += 1
            code_chars.append(" ")
            continue

        # Block comment /* ... */
        if ch == "/" and i + 1 < n and text[i + 1] == "*":
            i += 2
            while i < n:
                if text[i] == "*" and i + 1 < n and text[i + 1] == "/":
                    i += 2
                    break
                if text[i] == "\n":
                    code_chars.append("\n")
                    line += 1
                i += 1
            code_chars.append(" ")
            continue

        # Character literal '...' (skipping C++14 numeric digit separators)
        if ch == "'" and not _is_digit_separator(text, i):
            code_chars.append(" ")
            i += 1
            while i < n:
                c = text[i]
                if c == "\\":
                    if i + 1 < n:
                        if text[i + 1] == "\n":
                            code_chars.append("\n")
                            line += 1
                        i += 2
                    else:
                        i += 1
                    continue
                if c == "'":
                    i += 1
                    break
                if c == "\n":
                    # Unterminated char literal ends at unescaped newline
                    code_chars.append("\n")
                    line += 1
                    i += 1
                    break
                i += 1
            continue

        # C++11 raw string literal R"delim(...)delim"
        if ch == "R" and _is_raw_string_start(text, i):
            paren_idx = text.find("(", i + 2)
            if paren_idx != -1:
                delim = text[i + 2 : paren_idx]
                if len(delim) <= 16 and not any(
                    c in delim for c in (" ", "\t", "\r", "\n", "\\", ")")
                ):
                    closing = f"){delim}\""
                    end_idx = text.find(closing, paren_idx + 1)
                    if end_idx != -1:
                        interior = text[paren_idx + 1 : end_idx]
                        _scan_string_interior(interior, line, relpath, violations)
                        nl_count = interior.count("\n")
                        if nl_count:
                            code_chars.extend("\n" * nl_count)
                            line += nl_count
                        code_chars.append(" ")
                        i = end_idx + len(closing)
                        continue

        # Regular string literal "..."
        if ch == '"':
            start_line = line
            interior_chars: list[str] = []
            code_chars.append(" ")
            i += 1
            while i < n:
                c = text[i]
                if c == "\\":
                    if i + 1 < n:
                        nxt = text[i + 1]
                        interior_chars.append(c)
                        interior_chars.append(nxt)
                        if nxt == "\n":
                            code_chars.append("\n")
                            line += 1
                        i += 2
                    else:
                        interior_chars.append(c)
                        i += 1
                    continue
                if c == '"':
                    i += 1
                    break
                if c == "\n":
                    code_chars.append("\n")
                    line += 1
                interior_chars.append(c)
                i += 1
            _scan_string_interior("".join(interior_chars), start_line, relpath, violations)
            continue

        # Normal code character
        if ch == "\n":
            line += 1
        code_chars.append(ch)
        i += 1

    code_text = "".join(code_chars)
    for match in PRI_RE.finditer(code_text):
        macro = match.group(0)
        line_no = 1 + code_text[: match.start()].count("\n")
        violations.append(
            Violation(
                path=relpath,
                line=line_no,
                token=macro,
                detail=(
                    f"unsupported Newlib-nano format macro '{macro}' "
                    "(use 32-bit PRI*32 or %u with static_cast<unsigned>(...))"
                ),
            )
        )

    violations.sort(key=lambda v: (v.line, v.token))
    return violations


def scan_repo(
    repo_root: Path,
    roots: Sequence[str] = SOURCE_ROOTS,
) -> list[Violation]:
    """Scan all existing production C/C++ files under repo_root."""
    root_path = Path(repo_root).resolve()
    violations: list[Violation] = []

    for rel_root in roots:
        target_dir = root_path / rel_root
        if not target_dir.is_dir():
            continue

        for dirpath, dirnames, filenames in os.walk(target_dir):
            dirnames[:] = sorted(d for d in dirnames if d not in SKIP_DIRS)
            for fname in sorted(filenames):
                if not fname.endswith(SOURCE_EXTENSIONS):
                    continue
                file_path = Path(dirpath) / fname
                rel = file_path.relative_to(root_path)
                if any(part in SKIP_DIRS for part in rel.parts):
                    continue
                text = file_path.read_text(encoding="utf-8", errors="replace")
                violations.extend(scan_text(text, rel.as_posix()))

    return violations


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Check production C/C++ sources for printf length modifiers "
            "unsupported under CONFIG_NEWLIB_NANO_FORMAT=y."
        )
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parent.parent,
        help="Repository root directory (defaults to repo root).",
    )
    args = parser.parse_args(argv)

    violations = scan_repo(args.root)
    if violations:
        for v in violations:
            print(v.format(), file=sys.stderr)
        print(
            f"check_newlib_printf: found {len(violations)} unsupported "
            "Newlib-nano printf specifier(s).",
            file=sys.stderr,
        )
        return 1

    print("check_newlib_printf: OK (0 violations)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
