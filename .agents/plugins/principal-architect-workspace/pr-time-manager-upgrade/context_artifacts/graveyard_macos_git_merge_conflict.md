# Graveyard: macOS Git Case Insensitivity Merge Conflict

## The Failure
We attempted to merge the `feature/timemanager-upgrade` branch into `main`. `main` had transitioned all directory and file names to `PascalCase` (e.g., `modules/timeManager/` became `modules/TimeManager/`). The feature branch was rooted before this structural shift.

During the `git merge`, macOS treated `timeManager.hpp` and `TimeManager.hpp` as the identical path on disk, leading to silent overwrites. When `git status` or `git reset --hard` was executed, git reported the index as clean but the physical disk contained the lowercase `timeManager` files, causing `#include` compilation failures because C++ compilers are case-sensitive.

## Why it failed
1. macOS uses APFS which is case-preserving but case-insensitive.
2. Git's internal index tracks case accurately, causing a drift between the physical filesystem state and the index state.
3. Automated merges between branches with massive case-renames are inherently broken on macOS APFS volumes.

## Solution
Instead of fighting the filesystem with `git mv` workarounds or temporary renaming schemes, we executed Option A:
- Abort the merge entirely.
- Stay on `main`.
- Surgically checkout only the modified lines/files from the feature branch.
- Manually refactor those incoming lines to match the new `PascalCase` architecture.
