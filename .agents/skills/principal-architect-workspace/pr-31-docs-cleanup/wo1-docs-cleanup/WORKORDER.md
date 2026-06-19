---
name: pr-31-wo1
description: Legacy documentation cleanup — archive, merge, fix stale APIs, update indexes
complexity: standard
---

# Work Order: Legacy Documentation Cleanup

You are an expert senior software engineer and technical writer. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Reorganise `docs/` by archiving superseded reports, merging unique content from the retiring build guide, fixing stale API references in developer guides, and updating the docs index.

## Strict Boundary Box

- **DO** modify files ONLY in `docs/` (root and `developer/` subdirectory)
- **DO NOT** modify any source code, skill files, or agent workspace files
- **DO NOT** create branches or make git commits
- **DO** verify all API signatures against actual header files before writing them

## Pre-Flight

1. Read `.agents/rules/` files in order (mandatory)
2. Read the current `docs/README.md` to understand the index structure
3. Read the current `docs/developer/README.md` to understand the developer guide index

## Checklist

### Task 1: Archive 3 Assessment Reports

Create `docs/archive/` directory and move these files:

```bash
mkdir -p docs/archive
git mv docs/security-evaluation-report.md docs/archive/
git mv docs/embedded-best-practices-assessment.md docs/archive/
git mv docs/comparative_analysis_report.md docs/archive/
```

### Task 2: Merge Unique Content From `build_and_release_process.md`

Read the full file `docs/build_and_release_process.md` (500 lines).

**2a. Merge §12 Key Rotation (L446–500) → `docs/developer/security.md`**

Read `docs/developer/security.md` and find the "OTA Firmware Security" section. Add a new subsection `### Key Rotation` at the end of that section (before "The Central Tension" section) containing the 5-step procedure from L446–500. Adapt the formatting to match the existing doc style (copyright header, blockquote intro, code blocks). Preserve the two WARNING/CAUTION callouts verbatim.

**2b. Merge §11 CI-Assisted Workflow (L395–442) → `docs/developer/build-system.md`**

Read `docs/developer/build-system.md` and add a new subsection `### CI-Assisted Workflow (Optional)` at the end of the release pipeline section. Include:
- When to reinstate CI builds (3 scenarios)
- Why it was removed (local signing is simpler)
- How to use `--run-id` mode
- The `gh run list` + `deploy_release.py --run-id` workflow

**2c. Fix ECDSA→RSA inconsistency**

Note: Line 391 of `build_and_release_process.md` says "ECDSA" but the actual signing is RSA-2048 PKCS#1 v1.5 SHA-256. When merging content, use "RSA-2048" consistently.

**2d. Delete `build_and_release_process.md`**

After completing 2a and 2b:
```bash
git rm docs/build_and_release_process.md
```

### Task 3: Fix Stale MCP Tool API in `docs/developer/mcp-server.md`

The file documents the old `std::function` handler signature. The actual API (since PR-28) uses static function pointers:

**Current actual API** (verify against `modules/MCPServer/MCPToolRegistry.hpp`):
```cpp
using MCPToolHandler = void(*)(void* context, JsonObject& params, JsonObject& result);

void addTool(const char* name,
             const char* description,
             MCPToolHandler handler,
             void* context = nullptr,
             bool mutatesState = false);

void addTool(const char* name,
             const char* description,
             const char* inputSchemaJson,
             MCPToolHandler handler,
             void* context = nullptr,
             bool mutatesState = false);
```

Update ALL occurrences of the old `std::function` API in `mcp-server.md`:
1. The handler type definition — replace `std::function<void(JsonObject&, JsonObject&)>` with `MCPToolHandler` and explain it's a static function pointer
2. The `addTool()` signatures — add the `void* context` parameter
3. Any usage examples — update to use the `static void handler(void* ctx, ...)` pattern with `static_cast`
4. Reference `template/src/myApp.cpp` as the canonical example of the new pattern

Also check for and update the template example code in `docs/developer/mcp-server.md` to match the pattern in the actual `template/src/myApp.cpp` — read that file (around line 140-160) to see the current correct pattern.

### Task 4: Update `docs/README.md`

Replace the entire content with:

```markdown
# ESP32 App Foundation — Documentation

> ESP32 App Foundation (c) 2026 CB Labs
>
> Documentation index for the ESP32 App Foundation framework.

---

## For App Developers (Building ON the Framework)

Start here: [Developer Guide](developer/README.md) — tutorials, interface
contracts, and module authoring guidelines.

## Framework References

- [Design Specification](design_specification.md) — architectural blueprint,
  38 ADRs, subsystem details, and design decisions
- [OTA Developer Guide](OTADeveloperGuide.md) — OTA state machine, release
  channels, manifest format, and deployment procedures

## Historical Archives

Point-in-time assessments from early development. Kept for provenance.

- [Security Evaluation Report](archive/security-evaluation-report.md) — v0.1.0 audit (2026-06-01)
- [Embedded Best Practices Assessment](archive/embedded-best-practices-assessment.md) — initial review (2026-05-31)
- [Comparative Analysis Report](archive/comparative_analysis_report.md) — T1/T2 agent comparison
```

### Task 5: Add PAT Management Subsection to `docs/developer/security.md`

Read the skill reference file `.agents/skills/framework-expert/security/pat-management.md`.

Add a new subsection `### Personal Access Tokens (PATs)` to `docs/developer/security.md` after the "Web Portal & MCP Authentication" section. Cover:
- Why PATs are needed (GitHub API rate limits, private repo access)
- How to create a fine-grained PAT (Contents: read-only, single repo)
- Blast radius: scope to the releases repo, not the source repo
- The split-repo model (source vs releases)
- How PATs are stored (KeyManager encrypted storage)
- CA fallback and certificate bundle configuration

Keep it concise — the skill reference has ~8KB of content, but the human doc should be ~40-60 lines covering the essentials with a reference link to the OTA Developer Guide for the full workflow.

### Task 6: Add TLS Memory Optimization Subsection to `docs/developer/performance-guide.md`

Read the skill reference file `.agents/skills/framework-expert/performance/tls-memory-optimization.md`.

Add a new subsection `### TLS Memory Optimization` to the performance guide, placed after the existing TLS memory cost discussion. Cover:
- The problem (TLS is the largest single RAM consumer)
- Key sdkconfig knobs (dynamic buffer sizing, cert bundle mode, feature pruning)
- Measured results table (before/after heap savings)
- Certificate bundle modes (CMN/FULL/NONE and the DEFAULT_NONE bug)

Keep it concise — ~40-60 lines summarising the essentials.

## Verification

1. Confirm `docs/archive/` exists with 3 files
2. Confirm `docs/build_and_release_process.md` is deleted
3. Confirm `docs/developer/security.md` has Key Rotation and PAT sections
4. Confirm `docs/developer/build-system.md` has CI Workflow section
5. Confirm `docs/developer/mcp-server.md` has NO `std::function` references
6. Confirm `docs/developer/performance-guide.md` has TLS optimization section
7. Confirm `docs/README.md` has 3 sections (Developers, References, Archives)
8. List all modified/created/deleted files

## Completion

When finished, STOP and send a message back to the Principal Architect. Do not wrap up or distill until explicitly approved.
