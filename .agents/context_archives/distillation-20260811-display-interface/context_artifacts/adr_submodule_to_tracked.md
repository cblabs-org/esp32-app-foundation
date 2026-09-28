# ADR: framework-expert Submodule → Tracked Directory

## Context

The `framework-expert` skill was managed as a git submodule pointing to `mm-skills/framework-expert`. During the display interface work, WO5/WO6 agents updated documentation inside the submodule. However, the upstream repo was subsequently force-pushed (during a skill-manager operation), orphaning the commit (`3cc4a69`) that the parent repo referenced.

Result: `git submodule update --init` failed with "not our ref", and the entire `.agents/skills/framework-expert/` directory was empty — all skill docs were inaccessible.

## Decision

Convert `framework-expert` from a submodule to a regular tracked directory. This repo (`esp32-app-foundation`) is the **canonical source** for all `framework-*` skills. The skill-manager should treat this repo as upstream and clone/sync FROM here TO `mm-skills/framework-expert`, not the reverse.

## Implementation

1. `git submodule deinit -f .agents/skills/framework-expert`
2. `git rm -f .agents/skills/framework-expert`
3. `rm -rf .git/modules/.agents/skills/framework-expert`
4. Copy content back (without `.git/`), `git add` as regular files
5. Commit: `cbd035c`

## Impact

- Skill content is now version-controlled with the framework — no more orphaned refs
- Skill-manager needs config update to push FROM this repo rather than pull TO it
- `framework-portal-design` was already tracked as regular files (no action needed)
