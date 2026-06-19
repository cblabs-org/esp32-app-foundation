---
name: pr-32-wo2
description: Add deep links from design_specification.md into developer documentation
complexity: standard
---

# Work Order: Design Specification Deep Links

You are an expert senior software engineer and technical writer. You are NOT the Principal Architect.

## Objective

Add liberal deep links from `docs/design_specification.md` into the developer documentation in `docs/developer/`. The design spec should serve as a navigable architectural index — every section that has a corresponding developer guide should link to it, so readers can jump from "why" (design rationale) to "how" (developer tutorial).

## Strict Boundary Box

- **DO** modify ONLY `docs/design_specification.md`
- **DO NOT** modify any other files (developer docs, source code, skill files)
- **DO NOT** create branches or make git commits

## Pre-Flight

1. Read `.agents/rules/` files in order (mandatory)
2. Read `docs/developer/README.md` to get the full list of developer guides and their topics
3. Skim the design specification's table of contents to understand its structure (you read it already in WO1 pre-flight, or read lines 1-32 now)

## Mapping Table

Use this section-to-guide mapping to determine where to add links. For each design spec section, add a callout box at the TOP of the section (immediately after the section heading) linking to the relevant developer guide(s).

| Design Spec Section | Developer Guide(s) to Link |
|---|---|
| §2 Design Principles (2.1–2.9) | [coding-patterns.md](../developer/coding-patterns.md) — covers Hollywood Principle, Composition over Inheritance, SOLID, Service Module Pattern |
| §3 System Architecture Overview | [architecture-overview.md](../developer/architecture-overview.md) |
| §4 Framework Kernel | [architecture-overview.md](../developer/architecture-overview.md) |
| §5 Module System | [creating-a-module.md](../developer/creating-a-module.md) |
| §6 Extension Points & Interfaces | [creating-a-module.md](../developer/creating-a-module.md) — interface catalog and implementation examples |
| §7 Display Architecture | [using-optional-modules.md](../developer/using-optional-modules.md) |
| §8 Configuration Architecture | [config-system.md](../developer/config-system.md) |
| §9 Application Lifecycle | [architecture-overview.md](../developer/architecture-overview.md) — boot sequence and tick loop |
| §10 Web Portal Architecture | [extending-the-web-portal.md](../developer/extending-the-web-portal.md) |
| §11 Inter-Module Communication | [event-bus.md](../developer/event-bus.md) |
| §11A Logging & Observability | *(no developer guide yet — add a note: "Developer guide planned")* |
| §12 Repository & Build Architecture | [build-system.md](../developer/build-system.md) |
| §13 Security Architecture | [security.md](../developer/security.md) |
| §13.3 OTA Production System | [ota-update-system.md](../developer/ota-update-system.md) — NOTE: this file was moved here by WO1 |
| §14 Resource Budget & Constraints | [performance-guide.md](../developer/performance-guide.md), [embedded-safety.md](../developer/embedded-safety.md) |
| §15 Module Authoring Guide | [creating-a-module.md](../developer/creating-a-module.md), [coding-patterns.md](../developer/coding-patterns.md) |
| §16 Testing Architecture | [build-system.md](../developer/build-system.md) — testing section |
| §1 Purpose & Vision | *(no link needed — this is context, not tutorial)* |
| §17 Open Questions & Decision Log | *(no link needed — this is internal tracking)* |

## Link Format

Use a consistent callout format. Place it immediately after each section heading (before the section's prose):

```markdown
## 3. System Architecture Overview

> 📖 **Developer Guide:** [Architecture Overview](developer/architecture-overview.md) — tutorials on the 3-tier model, boot sequence, and interface contracts.
```

For sections with multiple relevant guides:

```markdown
## 14. Resource Budget & Constraints

> 📖 **Developer Guides:**
> - [Performance & Optimization](developer/performance-guide.md) — heap budgeting, PSRAM, flash size, TLS memory costs
> - [Embedded Safety](developer/embedded-safety.md) — memory rules, thread safety, DRAM constraints
```

## Rules

1. **Use relative paths** — the design spec is at `docs/design_specification.md`, so developer guides are at `developer/guide-name.md` (one level down).
2. **Add links to `##` (major section) headings only** — not every `###` subsection. Exception: §13.3 gets its own link because it maps to a dedicated guide.
3. **Preserve all existing content** — you are ONLY adding callout boxes, never removing or modifying existing prose.
4. **Keep callouts concise** — one line per guide, 2-3 words describing what it covers.
5. **Don't add links to §1 or §17** — these are context/tracking sections with no tutorial equivalent.
6. **For §11A (Logging)** — add the callout box but note "Developer guide planned" since no guide exists yet.

## Verification

1. Count the number of `📖 **Developer Guide` callouts added — should be 13-15
2. Confirm all relative paths are correct (`developer/guide-name.md`)
3. Confirm no existing content was modified or deleted
4. Confirm §13.3 links to `developer/ota-update-system.md` (not the old path)
5. Show a summary of sections linked and which guide(s) each points to

## Completion

When finished, STOP and send a message back to the Principal Architect.
