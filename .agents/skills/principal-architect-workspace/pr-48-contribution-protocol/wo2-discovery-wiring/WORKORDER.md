---
name: pr-48-wo2
description: Handoff instructions for Work Order 2 — wire discovery into SKILL.md, repoint CONTRIBUTING.md, refresh manifest
complexity: mechanical
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Wire the contribution protocol (created in WO1) into the skill's discovery layer so agents can actually find it. Update the SKILL.md routing table and trigger vocabulary, repoint root `CONTRIBUTING.md`, update `KEEPING-CURRENT.md`, and refresh `skill_manifest.json`.

# Strict Boundary Box

- **DO** edit: `SKILL.md`, `KEEPING-CURRENT.md`, `skill_manifest.json` (all in `.agents/skills/framework-expert/`)
- **DO** edit: `CONTRIBUTING.md` at repo root
- **DO NOT** touch `references/contribution-protocol.md` or `references/issue-management.md` — those were completed in WO1.
- **DO NOT** touch any C++ source files, build files, or partition tables.
- **DO NOT** create branches, make architectural decisions, or act as a principal architect.

# Tooling Constraints

- Documentation-only work order. No build or test tools required.
- Use `view_file` to read existing files before editing.
- Use `replace_file_content` / `multi_replace_file_content` for edits.

# Context Files to Read First

1. `.agents/skills/framework-expert/SKILL.md` — the file you're editing (routing table + triggers)
2. `.agents/skills/framework-expert/KEEPING-CURRENT.md` — the file you're editing
3. `.agents/skills/framework-expert/skill_manifest.json` — the file you're refreshing
4. `.agents/skills/framework-expert/references/contribution-protocol.md` — the file WO1 created (so you know what to link to)
5. `CONTRIBUTING.md` at repo root — the file you're repointing

# Line-Item Instructions

## Task 1: Update SKILL.md Routing Table

In the "Detailed References" routing table in `SKILL.md`, add a new row:

| **Contributing changes or proposing new capabilities** | `references/contribution-protocol.md` |

Place it immediately after the "Filing and triaging framework issues" row (which points to `issue-management.md`), so the two peer documents are adjacent.

## Task 2: Update SKILL.md Trigger Vocabulary

In the "Trigger Reference" section of `SKILL.md`, find the issue management trigger block and add a **new block** immediately after it for contribution triggers:

```markdown
- **Contributing & proposals**: "raise a PR", "submit a fix", "contribute to the
  framework", "open a pull request", "upstream a change", "patch the framework",
  "propose a feature", "suggest an improvement", "new framework capability",
  "write an ADR", "architectural decision", "enhancement proposal",
  "contribution protocol".
  → Read `references/contribution-protocol.md` for the full process.
```

## Task 3: Update KEEPING-CURRENT.md

In `KEEPING-CURRENT.md`, find the line about ADRs (line ~12: `- ADRs are added that establish new architectural rules`) and expand it to:

```markdown
- ADRs are added that establish new architectural rules (see `references/contribution-protocol.md` § ADR Guidance for when ADRs are required)
```

## Task 4: Repoint Root CONTRIBUTING.md

The root `CONTRIBUTING.md` is currently a module-authoring guide. It should be restructured to serve as a **contribution overview** that links to the detailed protocol. Replace its content with:

```markdown
# Contributing to the ESP32 App Foundation

Thank you for your interest in contributing! This document provides an overview
of how to contribute to the framework.

## Quick Links

| What you want to do | Where to go |
|---|---|
| **Report a bug** or framework defect | [Issue Management Protocol](.agents/skills/framework-expert/references/issue-management.md) |
| **Contribute a fix** or upstream a change | [Contribution Protocol](.agents/skills/framework-expert/references/contribution-protocol.md) |
| **Propose a new capability** or enhancement | [Contribution Protocol](.agents/skills/framework-expert/references/contribution-protocol.md) § Enhancement / Proposal Process |
| **Write a new module** (authoring guide) | [Module Developer Guide](.agents/skills/framework-expert/guides/module-developer-guide.md) |
| **Understand the architecture** | [Architecture Overview](.agents/skills/framework-expert/references/architecture.md) |

## Module Authoring

For detailed guidance on building modules that integrate with the framework
(interfaces, configuration, web portal, display, RTOS considerations), see the
[Module Developer Guide](.agents/skills/framework-expert/guides/module-developer-guide.md).

## Agent Harness Configuration

This repository uses a shared agent configuration in `.agents/`:

- **`AGENTS.md`** — Canonical, harness-neutral bootstrap (read by all harnesses)
- **`CLAUDE.md`** — Claude Code stub (imports `AGENTS.md`)
- **`GEMINI.md`** — Antigravity overrides and rules-load backstop
- **`.agents/rules/`** — Unconditional operational constraints

See each file for details on agent configuration.
```

## Task 5: Refresh skill_manifest.json

Read the current `skill_manifest.json`. Then:

1. Update `sourceDocsVersion` from `"PR-33"` to `"PR-48"`.
2. Update `lastUpdated` to `"2026-07-25"`.
3. Add `"references/contribution-protocol.md"` to the `references` array (in alphabetical order among the other `references/` entries).
4. **Do a file inventory** of the entire `.agents/skills/framework-expert/` directory (list all files recursively). Update the `references` array to match the actual files on disk. Remove any entries for files that no longer exist. Add any files that exist but are missing from the manifest.

# Verification Requirement

After completing all edits:
1. Re-read all modified files to confirm well-formed markdown.
2. Verify the routing table row in SKILL.md correctly links to the new protocol file.
3. Verify the trigger vocabulary block is properly formatted and placed.
4. Verify `CONTRIBUTING.md` links are correct relative paths.
5. Verify `skill_manifest.json` file count matches actual files on disk.
6. **STOP and ask the user if they are happy** before distilling or committing.
