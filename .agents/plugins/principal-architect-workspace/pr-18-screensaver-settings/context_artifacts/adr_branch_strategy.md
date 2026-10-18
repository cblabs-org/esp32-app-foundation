# ADR: Branch Strategy & Rogue Branch Cleanup

## Context

A prior session (during rule violation) created `pr-18-screensaver-settings` at commit `ad517a1` — a duplicate of the PR-17 WO4 refactoring commit. Meanwhile, the real work continued on `pr-17-wo4-module-root` which advanced to `0e11e30` (6 commits ahead).

## Decision

1. **Delete** the rogue `pr-18-screensaver-settings` branch
2. **Recreate** from `pr-17-wo4-module-root` HEAD (`0e11e30`) to include all post-WO4 follow-on work
3. Create WO branches sequentially off the feature branch

## Rationale

The rogue branch was created before the PA approval gate rule was enforced. It diverged from the canonical PR-17 work and would have caused merge conflicts or lost commits if used as-is.

## Result

Clean linear stack: `pr-17-wo4-module-root` → `pr-18-screensaver-settings` (9 commits, fast-forward merges throughout).
