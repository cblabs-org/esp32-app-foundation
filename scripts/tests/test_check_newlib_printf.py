"""
ESP32 App Foundation (c) 2026 CB Labs

https://github.com/cblabs-org/esp32-app-foundation

SPDX-License-Identifier: Apache-2.0

Module: scripts/tests/test_check_newlib_printf.py
Description: pytest suite for scripts/check_newlib_printf.py.

    Run: python3 -m pytest scripts/tests
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

import pytest

SCRIPTS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO_ROOT = Path(SCRIPTS).parent
sys.path.insert(0, SCRIPTS)

import check_newlib_printf as cnp  # noqa: E402


# --- Unsupported format specifiers in string literals ---


@pytest.mark.parametrize(
    "spec",
    [
        "%zu",
        "%zd",
        "%zi",
        "%zo",
        "%zx",
        "%zX",
        "%llu",
        "%lld",
        "%lli",
        "%llo",
        "%llx",
        "%llX",
        "%jd",
        "%ju",
        "%td",
        "%tu",
        "%08zu",
        "%-10zu",
        "%#llx",
        "%+lld",
        "%.*llu",
        "%*.*zu",
        "%8.4llu",
    ],
)
def test_flags_unsupported_format_specifiers(spec):
    code = f'ESP_LOGI(TAG, "val={spec}", x);\n'
    violations = cnp.scan_text(code, "sample.cpp")
    assert len(violations) == 1
    assert violations[0].line == 1
    assert violations[0].token == spec


def test_allows_standard_and_escaped_percent_specifiers():
    code = (
        'ESP_LOGI(TAG, "ok=%u %d %lu %ld %lx %s %%zu %%llu %%%%", '
        'static_cast<unsigned>(a), b, c, d, e, s);\n'
    )
    assert cnp.scan_text(code, "ok.cpp") == []


def test_triple_percent_before_zu_is_flagged():
    # %% is a literal %, followed by %zu which is an active conversion specifier
    code = 'snprintf(buf, n, "%%%zu", x);\n'
    violations = cnp.scan_text(code, "triple.cpp")
    assert [v.token for v in violations] == ["%zu"]


# --- PRI*64 / LEAST64 / FAST64 / MAX macros in code ---


@pytest.mark.parametrize(
    "macro",
    [
        "PRIu64",
        "PRId64",
        "PRIi64",
        "PRIo64",
        "PRIx64",
        "PRIX64",
        "PRIuLEAST64",
        "PRIdLEAST64",
        "PRIuFAST64",
        "PRIxFAST64",
        "PRIuMAX",
        "PRIdMAX",
    ],
)
def test_flags_64bit_and_max_pri_macros_in_code(macro):
    code = f'snprintf(buf, n, "%" {macro}, val);\n'
    violations = cnp.scan_text(code, "pri.cpp")
    assert len(violations) == 1
    assert violations[0].line == 1
    assert violations[0].token == macro


@pytest.mark.parametrize("macro", ["PRIu32", "PRId32", "PRIx32", "PRIu16", "PRIu8"])
def test_allows_32bit_and_narrower_pri_macros(macro):
    code = f'snprintf(buf, n, "%" {macro}, val);\n'
    assert cnp.scan_text(code, "pri32.cpp") == []


def test_pri_macro_inside_string_literal_or_identifier_not_flagged():
    code = (
        'const char* doc = "Avoid PRIu64 in firmware";\n'
        'int my_PRIu64_helper = 1;\n'
    )
    assert cnp.scan_text(code, "non_macro.cpp") == []


# --- String, raw-string, char-literal, and comment edge cases ---


def test_string_containing_comment_markers_still_scanned():
    code = (
        'const char* a = "http://example.com %zu";\n'
        'const char* b = "/* not a comment */ %llu";\n'
    )
    violations = cnp.scan_text(code, "urls.cpp")
    assert [(v.line, v.token) for v in violations] == [(1, "%zu"), (2, "%llu")]


def test_raw_string_literals_are_scanned_with_exact_line_numbers():
    code = (
        'const char* r1 = R"(count=%zu)";\n'
        'const char* r2 = u8R"raw(\n'
        'line2\n'
        'val=%llu\n'
        ')raw";\n'
    )
    violations = cnp.scan_text(code, "raw.cpp")
    assert [(v.line, v.token) for v in violations] == [(1, "%zu"), (4, "%llu")]


def test_comments_ignored_and_multiline_comment_preserves_line_numbers():
    code = (
        "// %zu and PRIu64 in line comment\n"
        "/*\n"
        " * %llu and PRId64 in block comment\n"
        " * %jd and %td\n"
        " */\n"
        'ESP_LOGE(TAG, "real bug %zu", n);\n'
        'snprintf(buf, len, "%" PRIu64, v);\n'
    )
    violations = cnp.scan_text(code, "comments.cpp")
    assert [(v.line, v.token) for v in violations] == [
        (6, "%zu"),
        (7, "PRIu64"),
    ]


def test_char_literals_and_digit_separators_do_not_break_scanner():
    code = (
        "char q = '\"';\n"
        "char slash = '/';\n"
        "char esc = '\\'';\n"
        "uint32_t big = 1'000'000;\n"
        'ESP_LOGW(TAG, "after=%zu", n);\n'
    )
    violations = cnp.scan_text(code, "chars.cpp")
    assert [(v.line, v.token) for v in violations] == [(5, "%zu")]


# --- Repository scanning, exclusions, and CLI exit codes ---


def test_scan_repo_skips_missing_roots_and_excluded_subdirs(tmp_path):
    # Production file with a violation
    prod_dir = tmp_path / "modules" / "Demo"
    prod_dir.mkdir(parents=True)
    (prod_dir / "Demo.cpp").write_text('ESP_LOGI(TAG, "%zu", n);\n', encoding="utf-8")

    # Ignored test/ and .pio/ subtrees with violations
    test_subdir = prod_dir / "test"
    test_subdir.mkdir()
    (test_subdir / "DemoTest.cpp").write_text('printf("%zu", n);\n', encoding="utf-8")

    pio_subdir = tmp_path / "lib" / ".pio" / "deps"
    pio_subdir.mkdir(parents=True)
    (pio_subdir / "dep.c").write_text('printf("%llu", n);\n', encoding="utf-8")

    # Non-C/C++ extension should be ignored
    (prod_dir / "notes.md").write_text('"%zu" PRIu64\n', encoding="utf-8")

    violations = cnp.scan_repo(tmp_path)
    assert len(violations) == 1
    assert violations[0].path == "modules/Demo/Demo.cpp"
    assert violations[0].line == 1
    assert violations[0].token == "%zu"


def test_cli_returns_1_on_violation(tmp_path, capsys):
    mod_dir = tmp_path / "src"
    mod_dir.mkdir(parents=True)
    (mod_dir / "main.cpp").write_text('printf("%zu", x);\n', encoding="utf-8")

    rc = cnp.main(["--root", str(tmp_path)])
    captured = capsys.readouterr()
    assert rc == 1
    assert "src/main.cpp:1:" in captured.err


def test_real_repo_is_clean_and_cli_exits_0(capsys):
    assert cnp.scan_repo(REPO_ROOT) == []
    rc = cnp.main(["--root", str(REPO_ROOT)])
    captured = capsys.readouterr()
    assert rc == 0
    assert "0 violations" in captured.out
