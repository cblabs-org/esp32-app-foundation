# Architecture Decision Record: Manual Port of TimeManager over Git Merge

## Context
The `feature/timemanager-upgrade` branch introduced massive architectural improvements (IConfigurable standardizations, MCPServer plugins, portal Web UI). However, it was built using our legacy `camelCase` naming conventions. In parallel, `main` aggressively migrated to a strict `PascalCase` structure for all modules and headers to fix ESP-IDF build consistency.

## Problem
Attempting a standard `git merge` or `git rebase` on macOS (a case-insensitive filesystem) caused persistent tracking errors. Git would attempt to merge `timeManager.cpp` and `TimeManager.cpp`, resulting in duplicate untracked files, lost capitalization, and broken `#include` chains. The PIO Native tests failed completely due to these missing files.

## Decision
We aborted the automated merge process entirely. Instead, a manual surgical port (Option A) was executed directly onto the `main` branch. The feature branch files were selectively checked out and manually refactored inline to adhere to the `PascalCase` convention (e.g., `getMCPServer()`, `AppState::Boot`, `OTAUpdateManager`). 

## Consequences
- **Positive:** We avoided polluting the filesystem with ghost files.
- **Positive:** We maintained the strict `PascalCase` project standard without regressions.
- **Positive:** All 401 PIO native tests successfully passed.
- **Negative:** We lost the granular commit history of the feature branch, compressing it into a single manual integration commit on `main`.
