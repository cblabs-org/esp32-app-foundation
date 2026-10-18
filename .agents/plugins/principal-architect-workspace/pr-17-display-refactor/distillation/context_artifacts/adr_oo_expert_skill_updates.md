# ADR — OO Expert Skill Updates

**Session**: 0735b228-f5fb-4a7a-b7e7-4e2807bebae0
**Date**: 2026-06-03

---

## What Was Changed

### `oo-expert/SKILL.md`
- Updated `description` frontmatter to include triggers for hierarchical sub-object design
  and Orchestrator Anti-Pattern identification
- Added **Reference Guide** table (progressive disclosure pattern — mirrors `embedded-systems` skill)
- Two new reference files loaded on-demand, not always

### New: `oo-expert/references/hierarchical-object-model.md`
Contains the generic (project-agnostic) version of Section 5:
- Domain Meaning Test (with pass/fail examples)
- Concrete types vs interface at different boundaries
- Data/config vs presentation separation
- Two-Step Smell
- Widget depth heuristic
- Layered subsystem priority model

### New: `oo-expert/references/service-module-pattern.md`
Contains the generic (project-agnostic) version of Section 6:
- Four orchestrator symptoms
- Correct inversion (Application owns transitions)
- Service-intrinsic exception (acceptable autonomy)
- Coupling audit table

---

## Key Design Decisions for the Skill Structure

### Why progressive disclosure?
Original draft would have added ~200 lines to `SKILL.md`. Not every module review needs
both patterns. Agent loads only what the code smell warrants.

### Why no project-specific content in oo-expert?
The first draft of Sections 5 and 6 contained `AppState`, `WifiManager`, `OTAProgressBoard`,
`display.carousel` etc. A future agent using this skill on a web backend or game engine
would be biased toward embedded display thinking. The skill is generic; project examples
belong in the project.

### Where did the project-specific content go?
Into `docs/design_specification.md §7.8` as the canonical developer reference for
the display module architecture. Future agents/developers working on the U8G2 module
will find the target design there, not buried in a skill file.

### Why not a new skill?
The patterns are OO expertise — an extension/elaboration of DIP, Law of Demeter,
and encapsulation as applied to module API design. They belong in `oo-expert`, not
as a separate fragmented skill.

### Why not an ADR file in .agents/design/?
The design spec already exists and has Section 7 (Display Architecture). Any developer
reading the spec should see the current target design, not need to cross-reference a
separate ADR file. The spec is the right home.

---

## Reference Loading Triggers

| Reference | Load When |
|---|---|
| `hierarchical-object-model.md` | Flat getter methods exposing internals; Law of Demeter violations; designing new multi-sub-object API; Two-Step Smell |
| `service-module-pattern.md` | Module holds Application hub reference; `tick()` polls external state; phase logic in service module; `SystemState`/`AppState` in wrong layer |
