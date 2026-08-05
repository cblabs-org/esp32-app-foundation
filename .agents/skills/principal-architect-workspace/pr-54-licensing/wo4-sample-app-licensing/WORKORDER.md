---
name: pr-54-licensing-wo4
description: Handoff instructions for Work order 4 - Sample App Licensing
complexity: mechanical
---
# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Ensure the `sample-app/` and `template-app/` source files use the same Apache-2.0 SPDX headers and license structure as the framework.

# Strict Boundary Box
- ONLY modify files inside `sample-app/` and `template-app/`.
- DO NOT touch framework files in `include/`, `lib/`, `modules/`, etc.
- DO NOT modify application logic.

# Tooling Constraints
- Use mechanical find/replace tools where possible.

# Checklist
- [x] Replace prose CC BY-NC-SA 4.0 headers in `sample-app/` source files with `SPDX-License-Identifier: Apache-2.0`.
- [x] Replace prose CC BY-NC-SA 4.0 headers in `template-app/` source files with `SPDX-License-Identifier: Apache-2.0`.
- [x] Remove any extraneous CC links in those headers.

# Status Update (2026-08-04)
- Licensing headers updated to Apache-2.0.
- Changes verified and committed (commit: 2179b55).
- Distillation completed.

# Verification Requirement
- Compile the sample-app and template-app if possible, or visually inspect changes.
- Ask the user if they are happy with the changes before distilling or committing. Only after approval, invoke the `@distillery` skill.
