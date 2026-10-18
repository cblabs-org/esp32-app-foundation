# ADR: Recovering Orphaned Stacked Branches After GitHub Squash-Merge

## Problem

When GitHub's "Squash and merge" is used on a PR, it creates a **new commit SHA** on main that doesn't match any commit in the original branch. If other branches were stacked on top (branched from the merged branch), they become orphaned — their base commits no longer exist in `main`'s history, so `git rebase main` replays the already-merged commits, causing conflicts.

### Concrete Example

```
main ← wo5 ← wo6 ← wo7  (stacked branches)
```

PR #86 squash-merged `wo5` → `main`. Now `wo6` and `wo7` still point to the original `wo5` commits, but `main` has a single squash commit `10543be` with different SHA. Naive `git rebase main` on `wo6` replays all of `wo5`'s commits, creating add/add conflicts.

## Solution: `--onto` Rebase with Original Base

Use `git rebase --onto` to transplant only the *new* commits from each stacked branch:

```bash
# Step 1: Rebase wo6 onto main, excluding the (now-merged) wo5 commits
git rebase --onto origin/main origin/display-interface/wo5-portal-scripts-docs display-interface/wo6-service-helper

# Step 2: Merge wo6, then rebase wo7 the same way
git rebase --onto origin/main <original-wo6-tip> display-interface/wo7-portal-pills
```

**Critical:** The `<old-base>` argument must be the *pre-rebase* tip of the parent branch. After wo6 is rebased and force-pushed, its remote ref changes — so for wo7, use the original wo6 commit SHA (`7396eec`), not the rebased one.

### If `--onto` Fails (Remote Ref Already Updated)

Fall back to cherry-pick:

```bash
# Identify wo7-only commits (2 commits beyond original wo6 tip)
git log --oneline 7396eec..origin/display-interface/wo7-portal-pills

# Create fresh branch from main and cherry-pick
git checkout -B display-interface/wo7-portal-pills origin/main
git cherry-pick d8e5feb 5b766ca
```

## Key Lesson

When using squash-merge with stacked branches, always note the original tip SHAs before merging. After squash-merge, use `--onto` rebase or cherry-pick to transplant the remaining stack.
