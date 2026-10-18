---
name: pr-43-wo9
description: Full build verification across all environments and all 3 apps
complexity: standard
---

# WO9 — Full Build Verification

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Run a complete build and test verification across all environments for all 3 apps (framework, template-app, sample-app) to confirm that WO1-WO8 have not introduced any regressions.

## Strict Boundary Box

- **DO NOT** modify any source files. This WO is verification-only.
- **DO NOT** flash to hardware — builds only.
- If a build or test fails, **STOP and report the failure** with full error output. Do not attempt to fix it.

## Tooling Constraints

- Use the PlatformIO MCP `build_project` tool for all builds.
- Use the PlatformIO MCP `run_tests` tool for native test execution.
- Acquire the PlatformIO lock before starting builds and release it when done.

## Checklist

### Framework (root project)

- [ ] **Build `esp32dev`**: `build_project` with environment `esp32dev` from project root
- [ ] **Build `esp32s3nano`**: `build_project` with environment `esp32s3nano` from project root
- [ ] **Build `native`**: `build_project` with environment `native` from project root
- [ ] **Run native tests**: `run_tests` with environment `native` from project root — record pass/fail count

### Template App

- [ ] **Build `esp32dev`**: `build_project` with environment `esp32dev` from `template-app/`
- [ ] **Build `esp32s3nano`**: `build_project` with environment `esp32s3nano` from `template-app/`
- [ ] **Build `native`**: `build_project` with environment `native` from `template-app/`
- [ ] **Run native tests**: `run_tests` with environment `native` from `template-app/` — record pass/fail count

### Sample App

- [ ] **Build `esp32dev`**: `build_project` with environment `esp32dev` from `sample-app/`
- [ ] **Build `esp32s3nano`**: `build_project` with environment `esp32s3nano` from `sample-app/`
- [ ] **Build `native`**: `build_project` with environment `native` from `sample-app/`
- [ ] **Run native tests**: `run_tests` with environment `native` from `sample-app/` — record pass/fail count

## Verification Report

Record the results in a summary table:

| App | Environment | Build | Tests | Notes |
|---|---|---|---|---|
| Framework | esp32dev | ⬜ | — | |
| Framework | esp32s3nano | ⬜ | — | |
| Framework | native | ⬜ | ⬜ _/_ | |
| Template | esp32dev | ⬜ | — | |
| Template | esp32s3nano | ⬜ | — | |
| Template | native | ⬜ | ⬜ _/_ | |
| Sample | esp32dev | ⬜ | — | |
| Sample | esp32s3nano | ⬜ | — | |
| Sample | native | ⬜ | ⬜ _/_ | |

**Pass criteria:** All 9 builds succeed. All native tests pass with zero failures.
