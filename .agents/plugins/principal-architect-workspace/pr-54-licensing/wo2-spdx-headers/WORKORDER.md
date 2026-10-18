---
name: pr-54-licensing-wo2
description: Handoff instructions for Work order 2 - SPDX Headers
complexity: mechanical
---
# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Replace all prose CC BY-NC-SA 4.0 license headers with machine-readable `SPDX-License-Identifier: Apache-2.0` headers in framework source files.

# Strict Boundary Box
- DO NOT modify the root LICENSE file.
- DO NOT modify the sample-app or template-app directories (that is WO4).
- Only replace the license header text; do NOT modify any actual code logic.

# Tooling Constraints
- Use `sed` via `run_command` or similar mechanical replacements.

# Checklist
- [ ] Find all `.cpp`, `.hpp`, `.h`, `.c` files in `include/`, `lib/`, `modules/`, `src/`, `hardware/`, and `adapters/` containing "Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International".
- [ ] Replace the text with `SPDX-License-Identifier: Apache-2.0`.
- [ ] If there are accompanying lines like "To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/", remove them.
- [ ] Leave the copyright line intact (e.g. `* ESP32 App Foundation (c) 2026 CB Labs`).

# Verification Requirement
- Compile the project (`platformio` MCP tool) to ensure no headers were accidentally corrupted.
- Ask the user if they are happy with the changes before distilling or committing. Only after approval, invoke the `@distillery` skill.
- [x] Find all files with license
- [x] Replace headers
- [x] Remove accompanying lines
- [x] Verify copyright lines
- [x] Compile project
- [x] User approval
- [x] Distillery distillation

Status: Complete
