# Graveyard: Branch Naming Conflict

## What Failed

Attempting to create WO branches using slash-separated names like `pr-40-docs-excellence/wo1-template-fixes` when a branch named `pr-40-docs-excellence` already existed.

## Error

```
fatal: cannot lock ref 'refs/heads/pr-40-docs-excellence/wo1-template-fixes':
'refs/heads/pr-40-docs-excellence' exists; cannot create
'refs/heads/pr-40-docs-excellence/wo1-template-fixes'
```

Git stores branch refs as filesystem paths. A branch `A` and a branch `A/B` cannot coexist because `A` would need to be both a file (for the branch ref) and a directory (for the child ref).

## Also Failed

Renaming the feature branch to `pr-40` and using `pr-40/wo1-...` — same conflict.

## Resolution

Used dashes instead of slashes for WO branches: `pr-40-docs-excellence-wo1-template-fixes`. This avoids all git ref path conflicts while maintaining readability.

## Lesson for Future PAs

When the STACKING_REFERENCE.md says to use `[project-name]/wo[Y]-[topic]`, this only works if the feature branch name does NOT match the `[project-name]` prefix exactly. Either:
1. Use dashes (`project-woN-topic`) instead of slashes
2. Name the feature branch differently from the WO prefix (e.g., `pr-40-base` for feature, `pr-40/wo1-...` for WOs)
