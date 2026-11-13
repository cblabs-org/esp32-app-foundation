<!--
  ESP32 App Foundation (c) 2026 CB Labs
  Framework Skill Reference — Contribution Protocol
  Loaded by: framework-expert skill agents
-->

# Contribution Protocol

This document covers how to contribute changes to the ESP32 App Foundation framework — the counterpart to [`issue-management.md`](./issue-management.md) (which covers reporting defects).

---

## Route Selection Table

Use this table to determine the appropriate workflow for your change:

| Situation | Route |
|---|---|
| Defect you cannot or should not fix | File an issue → [`issue-management.md`](./issue-management.md) |
| Defect you can fix | Follow this protocol (contribution) |
| New capability, shape is obvious and small | Follow this protocol, proposal step optional |
| New capability, shape needs agreement | Proposal step first, then contribution |
| Change to an architectural invariant | ADR required before implementation |
| New module donated upstream | Proposal + contribution, with module-authoring criteria |

---

## Enhancement / Proposal Process

### When a Proposal is Required vs. Optional

- **Required:** Any change to a public interface in `include/interfaces/`, any new framework module, or any change affecting an ADR-governed invariant.
- **Optional:** Obvious bug fixes, documentation fixes, or small isolated changes with no interface impact.

### How to File a Proposal

File a GitHub issue on `cblabs-org/esp32-app-foundation` with the `P4-enhancement` label.

### Lightweight Proposal Format

Use the following structure for proposal issues:

```markdown
## Problem
<!-- What limitation, missing feature, or friction does this proposal address? -->

## Proposed Shape
<!-- Design summary, proposed interfaces, or API contracts. -->

## Alternatives Considered
<!-- What other options were considered and why were they rejected? -->

## Affected Surface
<!-- List of files, interfaces, modules, or build configurations affected. -->
```

---

## PR Mechanics

- **Branch Naming:** Use Conventional Commits formatting for branch names and PR titles (`feat(...)`, `fix(...)`, `chore(...)`, `docs(...)`).
- **Issue Linking:** The PR description body must include `Fixes #N` referencing the parent issue so GitHub auto-closes it upon merge.
- **Pre-Submission Build:** Clean compilation must pass for both `arduino` and `espidf` frameworks before submitting a PR.
- **Build and Test Tooling:** Always use PlatformIO MCP tools (e.g. `build_project`, `run_tests`) per [`.agents/rules/04-mcp-tooling.md`](../../../rules/04-mcp-tooling.md), rather than running raw `pio` CLI commands.
- **Unit Tests:** Host-side unit tests under `test/test_unit/` are recommended for any new or modified interface surface.

---

## Versioning Interaction

Contributions must strictly respect the framework's versioning policy:

- **Tag Authority:** Contributors must not create `beta` or `stable` tags (these are human-gated).
- **Dev Tags:** `dev` tags (`v0.X.Y-dev.N`) are agent-autonomous during development per [`.agents/rules/03-versioning.md`](../../../rules/03-versioning.md).
- **Deployment Scripts:** Contributors must never execute `deploy_release.py` directly, as the private key prompt requires interactive user passphrase input.
- **Policy Reference:** For the complete versioning strategy, refer to [`docs/developer/versioning-policy.md`](../../../../docs/developer/versioning-policy.md).

---

## Authority & Contribution Origins

- **PR & Proposal Authorization:** Filing PRs and proposal issues is authorized for PA-tier agents and above. Tier-3 worker agents must surface findings and recommendations to their PA.
- **Consumer-Originated Contributions:** Framework contributions originating from consumer app projects must be branched in-tree within the framework repository (not forks), utilizing `found-by:<consumer>` attribution labels.

---

## Docs & Skill Co-Update Duty

To prevent documentation drift, any PR modifying framework capabilities must update corresponding skill documentation in lockstep:

- **Interface Changes:** Any PR modifying or adding a public interface must update the corresponding `interfaces/*.md` reference document in this skill.
- **Routing Table:** If a topic is added or moved, update the routing table in `SKILL.md`.
- **Manifest Refresh:** Update `skill_manifest.json` whenever adding or removing skill files.
- **Cautionary Precedent (Issue #19):** Issue #19 serves as the project's primary lesson on documentation drift, where skill docs drifted from the actual `IDisplayBoard` API implementation. Co-updating skill docs with code changes is mandatory.

---

## ADR Guidance

Architectural Decision Records (ADRs) capture design invariants and framework-level design choices.

- **Storage Location:** ADRs currently reside in PA workspace `context_artifacts/` directories (a centralized framework ADR register is a separate future concern).
- **When Required:** An ADR is required prior to implementation when introducing changes to architectural invariants or introducing new framework-level design patterns.
- **Referencing ADRs:** Reference ADRs by number (e.g., ADR-02, ADR-39) in proposals, PRs, and commit messages.
