---
name: pr-32-wo1
description: Migrate OTA Developer Guide into docs/developer/ tree and create skill OTA system domain
complexity: standard
---

# Work Order: OTA Guide Migration

You are an expert senior software engineer and technical writer. You are NOT the Principal Architect.

## Objective

Move the OTA Developer Guide from the `docs/` root into the `docs/developer/` tree as a first-class developer guide, create a matching `systems/ota/` domain in the framework-expert skill, deduplicate overlapping content, and fix all cross-links.

## Strict Boundary Box

- **DO** modify files in `docs/`, `docs/developer/`, and `.agents/skills/framework-expert/`
- **DO NOT** modify any source code files
- **DO NOT** create branches or make git commits
- **DO** verify all API signatures against actual header files before writing them

## Pre-Flight

1. Read `.agents/rules/` files in order (mandatory)
2. Read the full `docs/OTADeveloperGuide.md` (714 lines) to understand its structure
3. Read `docs/developer/security.md` to identify overlapping OTA content
4. Read `docs/developer/build-system.md` to find cross-links to the OTA guide
5. Read `.agents/skills/framework-expert/interfaces/ota-verifier.md` to understand existing skill coverage

## Checklist

### Task 1: Move OTA Guide to Developer Tree

```bash
git mv docs/OTADeveloperGuide.md docs/developer/ota-update-system.md
```

The file content stays as-is for now — Tasks 2-3 will update it.

### Task 2: Update the OTA Guide Content

Read the moved file and make these changes:

**2a. Update the header** to match the developer guide template:
```markdown
# OTA Update System

> ESP32 App Foundation (c) 2026 CB Labs
>
> The complete OTA update lifecycle — state machine, release channels, signature verification, config snapshots, error codes, and deployment procedures.
```

**2b. Deduplicate with `security.md`**

`security.md` already covers:
- IOTAVerifier interface (begin/update/verify)
- Streaming verification architecture
- Anti-rollback (SECURITY_VERSION)
- Upload security and transport security
- DRAM task constraint
- Key rotation (merged from build_and_release_process.md in PR-31)

In the OTA guide, REPLACE the detailed signature verification content with a concise summary + cross-link:
```markdown
> For detailed coverage of signature verification, anti-rollback, key rotation,
> and the DRAM task constraint, see [Security § OTA Firmware Security](security.md#8-ota-firmware-security).
```

Keep the OTA guide's unique content:
- State machine diagram and pipeline steps
- Release channels (stable/beta/dev)
- Config lifecycle (Active/Previous/Golden)
- Multi-board manifest format
- REST API reference
- EventBus OTA topics
- Error codes (15 codes)
- Troubleshooting guide

**2c. Fix any internal links** that reference the old location (e.g., `../OTADeveloperGuide.md` → links to sibling docs in `developer/`).

### Task 3: Update Cross-Links in Other Docs

**3a. `docs/developer/build-system.md`** — Find the cross-link to `../OTADeveloperGuide.md` and update to `ota-update-system.md` (now a sibling).

**3b. `docs/developer/security.md`** — Add a cross-link to the OTA guide where relevant:
```markdown
> For the full OTA lifecycle (state machine, release channels, config snapshots,
> error codes), see [OTA Update System](ota-update-system.md).
```

**3c. `docs/README.md`** — Remove the OTA Developer Guide from "Framework References" (it's now in the developer tree). The section should only contain the Design Specification.

**3d. `docs/developer/README.md`** — Add the OTA guide to the reading order. Place it as item 14 in the Advanced Topics section:
```markdown
14. [OTA Update System](ota-update-system.md) — The complete OTA lifecycle: state machine, release channels, config snapshots, multi-board manifests, error codes, and deployment procedures.
```

### Task 4: Create Skill OTA System Tree

Create `.agents/skills/framework-expert/systems/ota/` with 3 reference files mirroring the OTA guide's unique content:

**4a. `systems/ota/overview.md`** — OTA architecture overview, state machine, pipeline steps, release channels. (~80-100 lines)

**4b. `systems/ota/config-lifecycle.md`** — Active/Previous/Golden config snapshots, migration during OTA, rollback behaviour. (~40-60 lines)

**4c. `systems/ota/deployment.md`** — REST API reference, `deploy_release.py` workflow, multi-board manifest format, error codes table. (~60-80 lines)

Each file should:
- Follow the existing skill reference format (see any file in `systems/config/` or `systems/display/` for the template)
- Cross-reference the relevant human doc: `docs/developer/ota-update-system.md`
- Cross-reference the interface: `interfaces/ota-verifier.md`
- Cross-reference security concerns: `security/ota-security.md`

### Task 5: Update Skill Routing Table

Read `.agents/skills/framework-expert/SKILL.md` and add the OTA system to the routing table:

```markdown
| **OTA Update System** (State machine, channels, config lifecycle, deploy) | `systems/ota/` |
```

Place it after the Web Portal row.

## Verification

1. Confirm `docs/OTADeveloperGuide.md` no longer exists (moved)
2. Confirm `docs/developer/ota-update-system.md` exists and has the updated header
3. Confirm `docs/developer/README.md` lists the OTA guide
4. Confirm `docs/README.md` no longer links to the OTA guide
5. Confirm `docs/developer/build-system.md` cross-link updated
6. Confirm `docs/developer/security.md` has cross-link to OTA guide
7. Confirm `.agents/skills/framework-expert/systems/ota/` has 3 files
8. Confirm SKILL.md routing table has OTA entry
9. List all modified/created/deleted files

## Completion

When finished, STOP and send a message back to the Principal Architect.
