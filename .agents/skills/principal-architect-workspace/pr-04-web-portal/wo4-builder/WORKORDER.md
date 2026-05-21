---
id: pr-04-wo4-builder
title: portalBuilder.py
status: ready
---

## MANDATORY SKILL BOOT SEQUENCE — Execute FIRST

Before reading any code or writing any file, run these exact view_file calls in order:
1. view_file `.agents/skills/pio-manager/SKILL.md`
2. view_file `.agents/skills/embedded-systems/SKILL.md`
3. view_file `.agents/skills/house-style-docs/SKILL.md`
4. view_file `.agents/skills/oo-expert/SKILL.md`
5. view_file `.agents/skills/pio-manager/references/mcp-agent-reference.md`

Do NOT proceed past this block until all five are confirmed loaded.

---

## Objective
Create the `portalBuilder.py` build script to compile the SPA frontend into PROGMEM gzip arrays.

## Requirements
1. **Branch**: `pr-04-wo4-builder` (from `pr-04-web-portal`). Create this branch before making changes.
2. **Files**:
   - Create `scripts/portalBuilder.py`.
3. **Core Logic**:
   - The script must read files from `web/portal/`.
   - Compress the files using gzip.
   - Generate C++ header files containing the gzipped data as PROGMEM byte arrays.
   - Output headers to `include/portal_assets/`.
   - Update `platformio.ini` to run this script as an extra script before the main build, so the headers are always fresh.
4. **Testing**:
   - Verify the script correctly generates valid C++ headers.
   - Verify a full project build correctly incorporates these headers and compiles successfully.

## Guidance
- This is a standard PlatformIO extra script pattern.
- Ensure the generated headers have proper include guards.

## Closure Gate
- **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/skills/principal-architect-workspace/pr-04-web-portal/wo4-builder/` as the target directory. Wait for it to finish before dropping control back to the PA.
