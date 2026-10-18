---
name: pr-43-wo1
description: Fix captureU8g2.py decoder dispatch and device-side format field
complexity: standard
---

# WO1 — Fix captureU8g2.py Decoder Dispatch

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix the `capture_display` tool's format field and the Python client's decoder dispatch so that U8G2 tile-format framebuffers are decoded correctly. Closes GitHub Issue #18.

## Strict Boundary Box

- **DO NOT** modify any files outside the checklist below.
- **DO NOT** change the decoder algorithms themselves — they are correct.
- **DO NOT** alter the base64 encoding logic or framebuffer capture logic.
- **DO NOT** add new dependencies.

## Tooling Constraints

- Use PlatformIO MCP `build_project` to verify C++ compilation.
- Use `python3 -c "import py_compile; py_compile.compile('...', doraise=True)"` for Python syntax check.

## Checklist

### Device-side (C++)

- [ ] **`modules/MCPServer/MCPBuiltinTools.cpp:373`** — Change `result["format"] = "xbm"` to `result["format"] = "u8g2-tile"`
- [ ] **`modules/MCPServer/MCPBuiltinTools.cpp:347`** — Change tool description from `"Capture the current display buffer as a base64-encoded image"` to `"Capture the current display buffer as a base64-encoded U8G2 tile-format monochrome framebuffer"`

### Client-side (Python)

- [ ] **`.agents/skills/device-mcp-client/scripts/captureU8g2.py:3-8`** — Fix module docstring: remove "XBM-format buffer" claim. Replace with:
  ```
  The response contains a U8G2 tile-format monochrome framebuffer (base64-encoded)
  along with width, height, and format metadata.
  ```
- [ ] **`.agents/skills/device-mcp-client/scripts/captureU8g2.py:115`** — Change default format from `'xbm'` to `'u8g2-tile'`
- [ ] **`.agents/skills/device-mcp-client/scripts/captureU8g2.py:128-132`** — Invert the dispatch logic:
  ```python
  if fmt == "xbm-rowmajor":
      img = decode_xbm_monochrome(raw_buffer, width, height)
  else:
      # U8G2 tile format — default for all U8G2 monochrome displays.
      # Handles both the new "u8g2-tile" discriminator and the legacy
      # "xbm" value from pre-fix firmware.
      img = decode_u8g2_monochrome(raw_buffer, width, height)
  ```
- [ ] **`.agents/skills/device-mcp-client/scripts/captureU8g2.py:131`** — Remove the "Fallback to legacy tile-based U8G2 decoding" comment (it's now the primary path).

## Verification

- [ ] PlatformIO build passes (MCPBuiltinTools.cpp compiles).
- [ ] Python syntax check passes.
- [ ] Manual review: confirm the dispatch logic handles all 3 format strings (`"u8g2-tile"`, `"xbm"` legacy, `"xbm-rowmajor"` future).
