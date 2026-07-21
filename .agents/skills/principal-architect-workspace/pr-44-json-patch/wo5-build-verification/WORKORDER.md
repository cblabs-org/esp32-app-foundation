---
name: pr-44-wo5
description: Handoff instructions for Work Order 5 — full build verification across all apps and environments
complexity: mechanical
---

# WO5 — Full Build Verification (3×3 Matrix)

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Verify that the JSON Patch feature (WO1-WO4) compiles and tests pass across all 3 apps and all environments. This is the gate check before PR submission.

## Strict Boundary Box

- **DO NOT** modify any source files unless a build fails and a trivial fix is needed (e.g. missing include). If a non-trivial fix is needed, report it back to me.
- Surface any out-of-scope discoveries to me (the PA) in your completion report

## Tooling Constraints

- Use the **PlatformIO MCP server** tools for all builds and test runs
- Read the pio-manager skill first: `.agents/skills/pio-manager/SKILL.md`

## Line-Item Instructions

### 1. Framework builds

Build the main framework for all environments:
- `pio run -e esp32dev` — ESP32 firmware
- `pio run -e esp32s3nano` — ESP32-S3 firmware  
- `pio run -e native` — Native build (expect linker error for missing main — that's OK, check compilation)

### 2. Native test run

- `pio test -e native` — Run all native unit tests
- Report total test count (should be 401+)

### 3. Template app builds

```bash
cd template-app
pio run -e esp32dev
pio run -e esp32s3nano
```

### 4. Sample app builds

```bash
cd sample-app
pio run -e esp32dev
pio run -e esp32s3nano
```

### 5. Sample app native tests

```bash
cd sample-app
pio test -e native
```

## Verification Requirement

Report a build matrix table:

| App | Environment | Build | Tests |
|---|---|---|---|
| Framework | esp32dev | ? | N/A |
| Framework | esp32s3nano | ? | N/A |
| Framework | native | ? | ?/? |
| Template | esp32dev | ? | N/A |
| Template | esp32s3nano | ? | N/A |
| Sample | esp32dev | ? | N/A |
| Sample | esp32s3nano | ? | N/A |
| Sample | native | ? | ?/? |

All cells must show ✅ or an explanation of pre-existing failures.

Once verified, **STOP and send a message back to me** with results. Do not wrap up or distill.
