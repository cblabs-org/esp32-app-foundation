#!/usr/bin/env python3
#
# ESP32 App Foundation (c) 2026 CB Labs
#
# https://github.com/matthewmcneill/esp32-app-foundation
#
# SPDX-License-Identifier: Apache-2.0
#
# Module: test/render/count_lines.py
# Description: Counts total, blank, comment-only and code lines in C/C++ and XML
#              source, so that the legacy C++ and its replacement are measured
#              the same way (cloc is not installed here). A line that holds both
#              code and a trailing comment counts as code. A blank line inside a
#              block comment counts as blank.
#
#              C/C++ (.c .cc .cpp .cxx .h .hh .hpp .hxx .ino): '//' and '/* */'
#              comments, string-aware: comment markers inside "..." strings,
#              '...' character literals and R"d(...)d" raw strings do not start
#              a comment, and a C++14 digit separator (1'000) is not read as the
#              start of a character literal.
#              XML (.xml .svg): '<!-- -->' comments.
#
# Exported Functions:
# - classify_lines(): Label every line of a source text blank / comment / code.
# - count_file():     Count one file, optionally restricted to selected lines.
# - main():           Command line: count_lines.py [--json] FILE[:LINES] ...
#
# Usage:
#   count_lines.py modules/U8G2Display/boards/DiagnosticsBoard.hpp
#   count_lines.py src/main.cpp:91,93,211,213        # only those 1-based lines
#   count_lines.py src/main.cpp:200-214              # a range
#   count_lines.py --self-test

import argparse
import json
import os
import sys

BLANK = "blank"      # Whitespace only (including inside a block comment)
COMMENT = "comment"  # Comment text and whitespace, no code
CODE = "code"        # At least one code character

C_EXTENSIONS = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".ino"}
XML_EXTENSIONS = {".xml", ".svg"}


def classify_c_family(text):
    """Label each line of C/C++ text as BLANK, COMMENT or CODE.

    Walks the text one character at a time, tracking whether it is inside a
    block comment, line comment, string, character literal or raw string, so
    comment markers in literals are not mistaken for comments.
    """
    lines = text.split("\n")
    labels = []

    in_block = False       # Inside /* ... */
    raw_end = None         # Inside a raw string: the closing )delim" to look for

    for line in lines:
        has_code = False
        has_comment = False
        i = 0
        n = len(line)
        in_string = False      # "..." (an ordinary string never spans lines; a backslash-continued one is not tracked)
        in_number = False      # Inside a numeric token, where ' is a digit separator

        while i < n:
            ch = line[i]
            nxt = line[i + 1] if i + 1 < n else ""

            # --- Inside a block comment ---
            if in_block:
                has_comment = True
                if ch == "*" and nxt == "/":
                    in_block = False
                    i += 2
                else:
                    i += 1
                continue

            # --- Inside a raw string: code until the terminator ---
            if raw_end is not None:
                has_code = True
                if line.startswith(raw_end, i):
                    i += len(raw_end)
                    raw_end = None
                else:
                    i += 1
                continue

            # --- Inside an ordinary string literal ---
            if in_string:
                has_code = True
                if ch == "\\":
                    i += 2
                elif ch == '"':
                    in_string = False
                    i += 1
                else:
                    i += 1
                continue

            # --- Normal code ---
            if ch == "/" and nxt == "/":
                has_comment = True
                break  # The rest of the line is a comment
            if ch == "/" and nxt == "*":
                has_comment = True
                in_block = True
                i += 2
                in_number = False
                continue
            if ch == "R" and nxt == '"' and (i == 0 or not (line[i - 1].isalnum() or line[i - 1] == "_")
                                              or line[max(0, i - 2):i] in ("u8", "L", "u", "U")):
                # Raw string: R"delim( ... )delim"
                open_paren = line.find("(", i + 2)
                if open_paren != -1:
                    delim = line[i + 2:open_paren]
                    raw_end = ")" + delim + '"'
                    has_code = True
                    i = open_paren + 1
                    continue
            if ch == '"':
                has_code = True
                in_string = True
                in_number = False
                i += 1
                continue
            if ch == "'" and not in_number:
                # Character literal: consume to the closing quote
                has_code = True
                i += 1
                while i < n:
                    if line[i] == "\\":
                        i += 2
                        continue
                    if line[i] == "'":
                        i += 1
                        break
                    i += 1
                continue
            if ch == "'" and in_number:
                # C++14 digit separator, e.g. 1'000'000
                has_code = True
                i += 1
                continue

            if not ch.isspace():
                has_code = True
                # Track numeric tokens so a digit separator is not a char literal
                if ch.isdigit() and not in_number and (i == 0 or not (line[i - 1].isalnum() or line[i - 1] == "_")):
                    in_number = True
                elif in_number and not (ch.isalnum() or ch in "._"):
                    in_number = False
            else:
                in_number = False
            i += 1

        # Block comments and raw strings carry onto the next line via in_block / raw_end.
        if has_code:
            labels.append(CODE)
        elif has_comment:
            labels.append(COMMENT)
        elif in_block:
            labels.append(BLANK if line.strip() == "" else COMMENT)
        else:
            labels.append(BLANK)

    return labels


def classify_xml(text):
    """Label each line of XML text as BLANK, COMMENT or CODE ('<!-- -->' comments)."""
    labels = []
    in_comment = False
    for line in text.split("\n"):
        has_code = False
        has_comment = False
        i = 0
        n = len(line)
        while i < n:
            if in_comment:
                has_comment = True
                end = line.find("-->", i)
                if end == -1:
                    i = n
                else:
                    in_comment = False
                    i = end + 3
                continue
            if line.startswith("<!--", i):
                has_comment = True
                in_comment = True
                i += 4
                continue
            if not line[i].isspace():
                has_code = True
            i += 1
        if has_code:
            labels.append(CODE)
        elif has_comment:
            labels.append(COMMENT)
        elif in_comment and line.strip() != "":
            labels.append(COMMENT)
        else:
            labels.append(BLANK)
    return labels


def classify_lines(text, extension):
    """Label every line of `text`; the comment syntax is chosen by `extension`."""
    if extension in XML_EXTENSIONS:
        return classify_xml(text)
    if extension in C_EXTENSIONS:
        return classify_c_family(text)
    raise ValueError("unsupported extension: %r" % extension)


def parse_line_spec(spec):
    """Parse '91,93,211-213' into a set of 1-based line numbers."""
    selected = set()
    for part in spec.split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part:
            first, last = part.split("-", 1)
            selected.update(range(int(first), int(last) + 1))
        else:
            selected.add(int(part))
    return selected


def count_file(path, selected=None):
    """Count one file. `selected` optionally restricts to a set of 1-based line numbers."""
    extension = os.path.splitext(path)[1].lower()
    with open(path, encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    # A trailing newline does not start another line.
    if text.endswith("\n"):
        text = text[:-1]
    labels = classify_lines(text, extension)
    counts = {"total": 0, BLANK: 0, COMMENT: 0, CODE: 0}
    for number, label in enumerate(labels, start=1):
        if selected is not None and number not in selected:
            continue
        counts["total"] += 1
        counts[label] += 1
    return counts


def self_test():
    """Check the classifier on the cases that fool naive counters."""
    cases = [
        ("code\n\n// c\n/* b */\nint x; // t\n", ".cpp", [CODE, BLANK, COMMENT, COMMENT, CODE]),
        ('const char* s = "//not a comment";\n', ".cpp", [CODE]),
        ('const char* s = "/* not */"; int y;\n', ".cpp", [CODE]),
        ("char q = '\"'; // quote char\n", ".cpp", [CODE]),
        ("/* a\n\n b */ int z;\n", ".cpp", [COMMENT, BLANK, CODE]),
        ("int big = 1'000'000; // sep\n", ".cpp", [CODE]),
        ('auto r = R"x(// in raw\n/* still raw */)x";\n// after\n', ".cpp", [CODE, CODE, COMMENT]),
        ('s = "a\\"//b";\n', ".cpp", [CODE]),
        ("<a>\n<!-- c -->\n<!-- multi\n\n line -->\n<b/> <!-- t -->\n\n", ".xml",
         [CODE, COMMENT, COMMENT, BLANK, COMMENT, CODE, BLANK]),
    ]
    failures = 0
    for text, extension, expected in cases:
        got = classify_lines(text[:-1] if text.endswith("\n") else text, extension)
        if got != expected:
            failures += 1
            print("FAIL %r\n  expected %s\n  got      %s" % (text, expected, got))
    print("self-test: %d cases, %d failures" % (len(cases), failures))
    return 1 if failures else 0


def main():
    parser = argparse.ArgumentParser(description="Count blank, comment and code lines in C/C++ and XML.")
    parser.add_argument("files", nargs="*", help="FILE or FILE:LINES (LINES like 91,93,211-213)")
    parser.add_argument("--json", action="store_true", help="Emit JSON instead of a table")
    parser.add_argument("--self-test", action="store_true", help="Run the built-in classifier checks")
    args = parser.parse_args()

    if args.self_test:
        return self_test()
    if not args.files:
        parser.error("no files given")

    rows = []
    for argument in args.files:
        path, selected = argument, None
        if ":" in argument:
            path, spec = argument.rsplit(":", 1)
            selected = parse_line_spec(spec)
        counts = count_file(path, selected)
        counts["file"] = argument
        rows.append(counts)

    totals = {key: sum(row[key] for row in rows) for key in ("total", BLANK, COMMENT, CODE)}

    if args.json:
        print(json.dumps({"files": rows, "totals": totals}, indent=2))
        return 0

    width = max(len(row["file"]) for row in rows + [{"file": "TOTAL"}])
    print("%-*s %7s %7s %8s %7s" % (width, "file", "total", "blank", "comment", "code"))
    for row in rows:
        print("%-*s %7d %7d %8d %7d" % (width, row["file"], row["total"], row[BLANK], row[COMMENT], row[CODE]))
    if len(rows) > 1:
        print("%-*s %7d %7d %8d %7d" % (width, "TOTAL", totals["total"], totals[BLANK], totals[COMMENT], totals[CODE]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
