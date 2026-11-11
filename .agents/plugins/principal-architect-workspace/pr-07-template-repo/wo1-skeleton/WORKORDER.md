---
name: pr-07-wo1
description: Handoff instructions for Work order 1 - Template Skeleton
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
Create the `template/` folder and basic application skeleton (`main.cpp`, `myApp.hpp`, `myApp.cpp`, and `platformio.ini`) to serve as a starting point for new applications using the ESP32 App Foundation framework.

## Strict Boundary Box
Do NOT modify any existing framework files outside the new `template/` directory.

## Checklist
- [ ] Create `template/platformio.ini` with environments for `esp32dev` and `esp32s3nano` and `native`. Use `lib_deps = symlink://../` to link the framework locally. Also include `olikraus/U8g2@2.36.18`, `bblanchon/ArduinoJson@7.4.3`, and `ESP32Async/ESPAsyncWebServer`. Ensure build flags mimic the framework's needs where applicable.
- [ ] Create `template/src/myApp.hpp` and `template/src/myApp.cpp`. Define a `MyApp` class that implements `ITickable` and potentially `IWebContributor`. Follow House Style documentation strictly.
- [ ] Create `template/src/main.cpp`. It should instantiate the framework's `Application`, instantiate and register `MyApp`, and contain the 3-line entry point (`begin()`, `tick()`).
- [ ] Ensure all C++ files follow the House Style Documentation.

## Closure Gate
- **Distillation Requirement**: When you have completed and verified all code changes, you MUST invoke the `@distillery` skill to package your WO output. Pass `.agents/plugins/principal-architect-workspace/pr-07-template-repo/wo1-skeleton/` as the target directory. Wait for it to finish before dropping control back to the PA.
