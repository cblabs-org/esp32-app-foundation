---
name: pr41-wo17
description: Handoff instructions for Work Order 17 — Documentation updates
complexity: mechanical
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Update all developer documentation to reflect the new stub-header testing architecture. Remove references to `NATIVE_BUILD` guards as a pattern, and document the stub header library as the canonical approach for native-build compatibility.

## Strict Boundary Box

- **DO** modify documentation files in `docs/`, `README.md`, and skill files
- **DO** update the `framework-expert` skill if it references `NATIVE_BUILD`
- **DO NOT** modify source code files

## Tooling Constraints

- No special tooling needed — this is documentation only.

## Line-Item Instructions

### Developer Documentation

- [ ] **`docs/developer/testing.md`** (or equivalent) — Update to describe the stub-header architecture:
  - How native builds work (stub headers in `test/stubs/platform/` selected by include-path priority)
  - How to add a new stub when adding ESP-IDF API usage to a module
  - How to run native tests
  - The `HAS_DISPLAY` guard pattern for display-only code

- [ ] **`docs/developer/architecture.md`** (or equivalent) — Add a section on the platform abstraction strategy:
  - No runtime cost (stubs are compile-time only)
  - No `#ifdef` guards in production code
  - Stub headers mirror ESP-IDF/Arduino API surface

- [ ] **`README.md`** — Update any references to native builds or testing that mention `NATIVE_BUILD`

### Framework-Expert Skill

- [ ] **Grep the framework-expert skill** (`.agents/skills/framework-expert/`) for references to `NATIVE_BUILD`, `#ifdef`, or "native build guards". Update to reflect the new architecture.

### House Style Documentation

- [ ] **Update house style** (`.agents/skills/house-style-docs/`) if it references `NATIVE_BUILD` as a coding pattern. Replace with guidance on using stub headers.

### Embedded Systems Skill

- [ ] **Check `.agents/skills/embedded-systems/`** for references to the old guard pattern. Update if found.

### ADR (if appropriate)

- [ ] Consider writing a brief Architecture Decision Record documenting:
  - **Decision**: Replace inline `NATIVE_BUILD` guards with stub-header library
  - **Context**: 283 guards across 45+ files made production code unreadable
  - **Consequences**: Production code is clean; testing infrastructure lives in `test/stubs/`

## Verification Requirement

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy.
