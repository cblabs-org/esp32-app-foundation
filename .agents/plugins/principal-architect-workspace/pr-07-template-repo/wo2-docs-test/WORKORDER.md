---
name: pr-07-wo2
description: Handoff instructions for Work order 2 - Template README and Integration Test
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

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Write the `template/README.md` and verify that running PlatformIO build on the template directory successfully compiles the template app against the framework.

## Strict Boundary Box
Do NOT modify any existing framework files outside the new `template/` directory.

## Checklist
- [ ] Write `template/README.md` explaining how to use it (specifically how to add the framework as a git submodule).
- [ ] Add `test/test_unit/test_myapp/` with a stub unit test in the template.
- [ ] Verify that building the template succeeds. You must use `mcp_platformio_build_project` with `projectDir` set to the absolute path of the `template/` folder. Wait for the background task to complete and use `mcp_platformio_query_logs` to ensure it compiled successfully against `esp32dev` and `esp32s3nano`. Fix any compilation errors in the template if they arise.

## Closure Gate
- **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/skills/principal-architect-workspace/pr-07-template-repo/wo2-docs-test/` as the target directory. Wait for it to finish before dropping control back to the PA.
