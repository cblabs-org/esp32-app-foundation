# Keeping This Skill Current

## When to Update

Update this skill's references when:
- A new interface is added to `include/interfaces/`
- An existing interface signature changes
- The registration API (`registerModule()`) changes
- New EventBus topics are added by framework modules
- The boot lifecycle phases change
- The web portal SPA shell contract changes

## How to Update

1. Read the changed source files
2. Update the corresponding `references/*.md` file
3. If a new topic area was added, create a new reference and add it
   to the routing table in `SKILL.md`
4. Update the `skill_manifest.json` timestamp

## Source of Truth

- Interface contracts: `include/interfaces/*.hpp`
- Registration API: `modules/Application/Application.hpp`
- Boot sequence: `modules/Application/Application.cpp` → `begin()`
- Full developer docs: `docs/developer/`
