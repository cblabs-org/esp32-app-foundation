---
complexity: standard
parent_pr: pr-128-datamanager-resilience
branch: 128-dm-resilience/wo7-docs-migration
base: 128-dm-resilience/wo5-health-surfacing
---

# WO7 — Documentation & Migration Guide

## Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT
the Principal Architect. Do not make high-level architectural decisions, do not branch, and
do not plan. Your task is strictly limited to executing the checklist below.

## Objective
Document the `FetchContext` contract, write a downstream migration guide for the breaking
`fetchUpdate()` API change, and update the framework-expert skill so future agents understand
the new data source architecture.

## Background

This PR introduced a breaking API change to `IDataSource::fetchUpdate()`:
- **Before**: `virtual UpdateStatus fetchUpdate() = 0;`
- **After**: `virtual UpdateStatus fetchUpdate(const FetchContext& ctx) = 0;`

Plus new framework features:
- `FetchContext` with `DeadlineToken` for cooperative deadline/cancel
- Framework-owned TLS client with cert bundle (replaces consumer-side cert-bundle hack)
- Health state surfacing (EventBus, REST, portal, display, MCP)
- Deadline enforcement (exceeded deadline → quarantine, not abort)

All downstream consumers (`WeatherModule` in sample-app, `MyApp` in template-app, and 3 external
private repos) need a migration guide. The framework-expert skill needs updating so future
agent sessions understand the new patterns.

## Strict Boundary Box

- **Do NOT** modify any source code files. This is a documentation-only WO.
- **Do NOT** file GitHub issues. Surface findings in the Completion Report.
- **Do NOT** create or modify test files.

## Tooling Constraints

No build/test tools needed — this is documentation only. Use file-read tools to reference
the actual code when writing docs.

## Line-Item Instructions

### 1. Create `docs/developer/fetch-context-contract.md`

- [ ] Document the `FetchContext` contract:
  - What `FetchContext` provides (deadline, client, policy constants)
  - What `DeadlineToken` provides (`isExpired()`, `isCancelled()`, `remainingMs()`)
  - The ownership split: framework owns scheduling (when to call `fetchUpdate()`),
    consumer owns transport (what happens inside one fetch)
  - What the consumer SHOULD do: check `ctx.deadline.isExpired()` in long loops
  - What the consumer MUST NOT do: build its own scheduling backoff, construct its
    own TLS client, call `setInsecure()`
  - Policy constants and their meaning
  - Thread safety: `fetchUpdate()` is called from the DataManager worker task
  - Health state machine overview (states, transitions, quarantine exit)

### 2. Create `docs/developer/migration-guide-fetchupdate.md`

- [ ] Downstream migration guide for the API break:
  - Title: "Migrating to `fetchUpdate(const FetchContext&)`"
  - Minimum viable migration: add `const FetchContext& ctx` to signature, ignore it
  - Recommended migration: use `ctx.client` for HTTP, check `ctx.deadline.isExpired()`
  - Before/after code examples (use WeatherModule as reference)
  - What to delete: `setInsecure()` calls, custom cert-bundle hacks, self-constructed
    `WiFiClientSecure` instances, custom scheduling backoff
  - `queueDataJob` signature change: `std::function<void()>` → `std::function<void(const FetchContext&)>`
  - Testing: mocks need signature update (can ignore `ctx` parameter)

### 3. Update framework-expert skill

Read the skill file at `.agents/skills/framework-expert/SKILL.md` to understand the existing
structure, then update it to cover:

- [ ] Add `FetchContext` to the "Building and extending" trigger list in the YAML description
- [ ] Add a section documenting the `FetchContext` / `IDataSource` contract
- [ ] Add a section on the health state machine and surfacing
- [ ] Add `DeadlineToken`, `DataSourceHealthPayload`, `data.source.health` to relevant
  sections
- [ ] Update any existing `IDataSource` documentation to reflect the new signature
- [ ] Add the `get_data_sources` MCP tool to the MCP section if one exists
- [ ] Add `/api/data/sources` and `/api/data/sources/reset` to the REST API section if one exists

### 4. Commit

- [ ] Commit all changes with message:
  `docs: FetchContext contract, fetchUpdate migration guide, framework-expert skill update (#128)`

## Verification Requirement

Documentation only — no build required. Read over each document for accuracy, consistency,
and completeness before committing. **Before committing, ask the user if they are happy
with the changes.**

## Issue Surfacing

If you discover a defect, upstream bug, or concern *outside your boundary box*, surface it
in your Completion Report below — do not file issues directly. The Principal Architect will
triage and file them under the correct author identity.

---

## Completion Report
<!-- Written by the worker at task completion -->

### Status
(pending)

### Changes Made
(pending)

### Decisions Made
(pending)

### Dead Ends & Warnings
(pending)

### Out-of-Scope Findings
(pending)

### Blockers
(pending)
