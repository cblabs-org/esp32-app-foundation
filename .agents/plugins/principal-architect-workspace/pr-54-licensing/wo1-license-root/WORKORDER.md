---
name: pr-54-licensing-wo1
description: Handoff instructions for Work order 1 - Root License
complexity: mechanical
---
# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Add the Apache-2.0 LICENSE file to the repository root and update README.md to reflect the new license.

# Strict Boundary Box
- DO NOT touch any .cpp or .hpp files.
- DO NOT modify the sample-app or template-app directories.
- DO NOT touch platformio.ini.

# Tooling Constraints
- Use standard file write tools.

# Checklist
- [x] Create `LICENSE` file at the root of the workspace containing the standard Apache License, Version 2.0 text.
- [x] Update `README.md` to replace references to "Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International" with "Apache License 2.0".

# Status
- Completed 2026-08-04.

# Verification Requirement
- Ask the user if they are happy with the changes before distilling or committing. Only after approval, invoke the `@distillery` skill.
