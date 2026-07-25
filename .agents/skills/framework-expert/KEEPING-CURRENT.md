# Keeping This Skill Current

## When to Update

Update this skill's references when:
- A new interface is added to `include/interfaces/`
- An existing interface signature changes
- The registration API (`registerModule()`) changes
- New EventBus topics are added by framework modules
- The boot lifecycle phases change
- The web portal SPA shell contract changes
- ADRs are added that establish new architectural rules (see `references/contribution-protocol.md` § ADR Guidance for when ADRs are required)
- New sub-systems are added (e.g., config, display, event-bus, web-portal)

## How to Update

1. Read the changed source files.
2. Update the corresponding markdown file in the relevant sub-directory (`interfaces/`, `systems/`, `coding-patterns/`, `performance/`, `embedded-safety/`, `guides/`).
3. If a new topic area or tree was added, update the routing table in `SKILL.md`.
4. Update the `skill_manifest.json` timestamp, version, and file inventory.

## Source of Truth

- Interface contracts: `include/interfaces/*.hpp`
- Registration API: `modules/Application/Application.hpp`
- Boot sequence: `modules/Application/Application.cpp` → `begin()`
- Full developer docs: `docs/developer/`
- Architecture notes: `docs/developer/architecture-overview.md`
