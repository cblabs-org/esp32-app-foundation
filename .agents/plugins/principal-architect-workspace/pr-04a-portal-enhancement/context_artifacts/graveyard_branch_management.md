# Graveyard: Branch Management Failures

## Problem
When multiple architect sessions work on the same repository, uncommitted changes are silently lost during branch switches and merges. This happened twice in this session:

### Incident 1: Portal Fixes Lost After Merge
- Session A (this session) made portal fixes to `app.js`, `wifi.js`, `index.html`, `system.js`
- User manually merged other branches into `main`
- All uncommitted portal fixes were wiped — the merge brought the original committed versions forward
- **Root cause**: Fixes were in the working tree, never committed. Branch operations reset the working tree.

### Incident 2: Status Pill Lost
- After re-applying the first batch of fixes, the Status sub-pill injection was missed
- Three interdependent pieces were needed: `webServer.cpp` (synthetic tab), `index.html` (`tab-status` div), `system.js` (target `tab-status`)
- Missing any one piece breaks the feature silently

## Prevention Strategies (Not Yet Implemented)
1. **Mandatory WIP commit before spawning** — Bridge prompts should require `git commit -m "WIP: ..."` before any new session starts
2. **Git worktrees** — Each architect session gets an isolated checkout via `git worktree add`
3. **Stash guard in plan-spawn** — Automatically `git stash` before switching branches
4. **Pre-merge checklist** — User verifies no uncommitted changes exist before merging

## Key Lesson
Never assume uncommitted changes survive branch operations. Always commit before context-switching.
