---
name: pr-48-wo1
description: Handoff instructions for Work Order 1 — create contribution-protocol.md and update issue-management.md
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Create the **contribution protocol** reference document for the `framework-expert` skill, and update `issue-management.md` to add an enhancement severity tier and cross-link to the new document. This closes the core documentation gap identified in GitHub Issue #43.

# Strict Boundary Box

- **DO** create and edit files ONLY within `.agents/skills/framework-expert/references/`.
- **DO NOT** touch `SKILL.md`, `KEEPING-CURRENT.md`, `skill_manifest.json`, or root `CONTRIBUTING.md` — those are WO2's scope.
- **DO NOT** touch any C++ source files, build files, or partition tables.
- **DO NOT** create branches, make architectural decisions, or act as a principal architect.
- **DO NOT** create an ADR register or index — that is a separate future concern.

# Tooling Constraints

- This is a documentation-only work order. No build or test tools are required.
- Use `view_file` to read existing files before editing.
- Use `write_to_file` and `replace_file_content` / `multi_replace_file_content` for edits.

# Context Files to Read First

Before starting, read these files to understand the existing patterns:

1. `.agents/skills/framework-expert/references/issue-management.md` — the peer document you're complementing
2. `.agents/rules/03-versioning.md` — versioning governance (contributions must respect this)
3. `.agents/rules/04-mcp-tooling.md` — MCP tool usage policy (contributions must respect this)
4. `CONTRIBUTING.md` (repo root) — the existing module-authoring guide (for tone/style reference)
5. `docs/developer/versioning-policy.md` — full versioning reference

# Line-Item Instructions

## Task 1: Create `references/contribution-protocol.md`

Create `.agents/skills/framework-expert/references/contribution-protocol.md` with the following structure. Use the same style, tone, and formatting conventions as `issue-management.md` (copyright header, clean markdown tables, concise prose).

### Required Sections:

**1. Header & Purpose**
- Copyright notice matching `issue-management.md`
- Title: `# Contribution Protocol`
- Intro paragraph: This document covers how to contribute changes to the ESP32 App Foundation framework — the counterpart to `issue-management.md` (which covers reporting defects).

**2. Route Selection Table** (the decision table from Issue #43)

| Situation | Route |
|---|---|
| Defect you cannot or should not fix | File an issue → `issue-management.md` |
| Defect you can fix | Follow this protocol (contribution) |
| New capability, shape is obvious and small | Follow this protocol, proposal step optional |
| New capability, shape needs agreement | Proposal step first, then contribution |
| Change to an architectural invariant | ADR required before implementation |
| New module donated upstream | Proposal + contribution, with module-authoring criteria |

**3. Enhancement / Proposal Process**
- How to file a proposal: create a GitHub issue with the `enhancement` label
- Lightweight proposal format: Problem / Proposed shape / Alternatives considered / Affected surface
- When a proposal is required: any change to a public interface in `include/interfaces/`, any new module, any change to an ADR-governed invariant
- When a proposal is optional: obvious bug fixes, doc fixes, small isolated changes

**4. PR Mechanics**
- Branch naming: use Conventional Commits (`feat(...)`, `fix(...)`, `chore(...)`, `docs(...)`)
- PR body must include `Fixes #N` referencing the parent issue
- Pre-submission: build must pass for both `arduino` and `espidf` frameworks
- Build and test: use PlatformIO MCP tools per `.agents/rules/04-mcp-tooling.md`, not raw `pio` CLI
- Host-side unit tests under `test/test_unit/` recommended for new interface surface

**5. Versioning Interaction**
- Contributors must not create `beta` or `stable` tags
- `dev` tags are agent-autonomous per `.agents/rules/03-versioning.md`
- Contributors must never execute `deploy_release.py` directly (interactive passphrase)
- Reference `docs/developer/versioning-policy.md` for full policy

**6. Authority**
- Who may file a PR: PA-tier and above; Tier-3 workers surface findings to their PA
- Who may file a proposal issue: PA-tier and above
- Consumer-originated contributions: branch in-tree (not fork); use `found-by:<consumer>` attribution

**7. Docs & Skill Co-Update Duty**
- A PR that changes an interface must update the corresponding `interfaces/*.md` in the skill
- If a topic moved, update the `SKILL.md` routing table
- Update `skill_manifest.json` when adding or removing skill files
- Reference Issue #19 as the cautionary precedent (docs drifted from `IDisplayBoard` API)

**8. ADR Guidance**
- ADRs are Architectural Decision Records capturing design invariants
- Current location: PA workspace `context_artifacts/` directories (no centralized register yet)
- When an ADR is required: changes to architectural invariants, new framework-level patterns
- How to reference: by number (e.g., ADR-02, ADR-39) — a centralized register is a future concern

## Task 2: Update `issue-management.md` — Add Enhancement Tier

Edit `.agents/skills/framework-expert/references/issue-management.md`:

**2a.** In the severity taxonomy table (around lines 32-38), add a fourth row:

| `P4-enhancement` | New capability or improvement — not a defect | Proposal |

**2b.** At the end of the file (after the last section), add a cross-link section:

```markdown
## Related

- **Contributing a fix or proposing a new capability →** [`contribution-protocol.md`](./contribution-protocol.md)
- **File an issue when you cannot or should not act; follow the contribution protocol when you can.**
```

# Verification Requirement

After completing all edits:
1. Re-read both files to confirm they are well-formed markdown with no broken links or formatting issues.
2. Verify the cross-links between the two documents are consistent.
3. **STOP and ask the user if they are happy** before distilling or committing.

---

## Status Update

- **Status:** 🟢 COMPLETED (2026-07-25)
- **Outcome:** Successfully created `references/contribution-protocol.md` and updated `references/issue-management.md` with `P4-enhancement` severity tier and cross-links. Verified markdown syntax and relative links. Approved by parent coordinator.

