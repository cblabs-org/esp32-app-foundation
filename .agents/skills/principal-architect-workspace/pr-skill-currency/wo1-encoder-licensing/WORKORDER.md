---
name: skill-currency-wo1
description: Handoff instructions for Work Order 1 — encoder license-header regression fix
complexity: mechanical
---

# Parent Project

**PR/Project:** `pr-skill-currency` — see
[`../PROJECT.md`](../PROJECT.md). Human approval of this decomposition is recorded
in that file's Status Log (2026-08-05, "PLAN GATE SATISFIED"). That committed
approval is your authorisation to execute this WORKORDER autonomously within the
Strict Boundary Box below.

**Active branch:** `skill-currency/wo1-encoder-licensing` (already created off `main`).

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You
are NOT the Principal Architect. Do not make high-level architectural decisions, do
not create branches, and do not plan. Your task is strictly limited to executing the
checklist below.

# Objective

The EncoderModule was recovered (PR #75) from a pre-licensing branch, so 5 files
still carry the old `Creative Commons Attribution-NonCommercial-ShareAlike 4.0`
license header. Replace it with the machine-readable `SPDX-License-Identifier:
Apache-2.0` header, matching the rest of the repository (this is the same swap the
`pr-54-licensing` WO2 sweep applied everywhere else). **Header comments only — no
code logic changes.**

# Strict Boundary Box

- **ONLY** these 5 files may be modified:
  - `include/EncoderConfig.h`
  - `modules/EncoderModule/EncoderModule.cpp`
  - `modules/EncoderModule/EncoderModule.hpp`
  - `test/stubs/platform/driver/pulse_cnt.h`
  - `test/test_unit/test_encoderModule/test_EncoderModule.cpp`
- Do NOT modify the root `LICENSE` file, any other source file, or any docs.
- Do NOT change any code, includes, or logic — only the license lines inside the
  top comment block.
- Do NOT run `git add -A` / `git add .`. Stage ONLY the 5 files above (an unrelated
  untracked `.agents/context_archives/...` directory exists in the working tree and
  must NOT be committed).
- Do NOT create branches, open PRs, or touch any other branch.

# Tooling Constraints (per `.agents/rules/04-mcp-tooling.md`)

- For build verification you MUST use the **`platformio` MCP server** tools (e.g.
  `build_project`), NOT raw `pio` CLI. If a queue/lock error occurs, consult the
  `pio-manager` skill.

# Line-Item Instructions

For **each** of the 5 files, inside the top `/* ... */` header block, replace the
two Creative Commons lines:

```
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
```

with the single line:

```
 * SPDX-License-Identifier: Apache-2.0
```

- [ ] Leave the `* ESP32 App Foundation (c) 2026 CB Labs` copyright line and the
      repo-URL line intact.
- [ ] Leave the `* Module:`, `* Description:`, and any exports block untouched.
- [ ] Preserve the exact comment framing (` * ` prefixes, blank comment lines, and
      the closing `*/`). Do not reflow or reindent the rest of the block.
- [ ] If a file's CC wording differs slightly from the two lines above, replace the
      equivalent CC-license lines with the same single SPDX line — the end state is:
      **zero** files under `git ls-files` contain the string
      `Creative Commons Attribution-NonCommercial`.

# Verification Requirement

- [ ] Confirm no CC headers remain:
      `git ls-files | grep -E '\.(h|hpp|cpp|c)$' | xargs grep -l "Creative Commons Attribution-NonCommercial"`
      must return **nothing**.
- [ ] Confirm all 5 files now contain `SPDX-License-Identifier: Apache-2.0`.
- [ ] Run a **native** build via the `platformio` MCP server to prove no comment
      block was corrupted (broken `*/`, etc.). Report the result.
- [ ] Show `git diff --stat` — it must list exactly the 5 files and be
      comment-line-only changes.

# Completion Protocol

When done, **STOP and send a message back to the Principal Architect** with: the
verification results, the `git diff --stat`, and the native build outcome. Do NOT
commit, wrap up, or distill until the PA explicitly approves. Once approved, the PA
will direct commit + next steps.

# Status

- Pending execution.
