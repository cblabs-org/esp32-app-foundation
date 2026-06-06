---
title: WO2 — sync_sdkconfig.py Pre-Build Invalidation Guard
distilled_at: 2026-06-06T11:16:41Z
original_plan_id: pr-19-sdkconfig-layering
artifacts:
  - context_artifacts/adr_board_config_api.md
  - context_artifacts/graveyard_getprojectoption_board_build.md
  - context_artifacts/state_sync_sdkconfig.md
---

## Executive Summary

WO2 is **complete and committed** (`9e19d2c` on `pr-19/wo2-sync-script`). `scripts/sync_sdkconfig.py` was created and wired into `platformio.ini` as the first `pre:` extra_scripts entry. It correctly invalidates stale `sdkconfig.<env>` files by comparing mtimes against `.defaults` source files. One critical API bug (see graveyard) was discovered and fixed: `GetProjectOption` does not work for `board_build.*` options — `BoardConfig().get("build.*")` must be used instead.

## Next Steps

- **PR-19 next work order:** WO3 (if any) or raise the PR merging `pr-19/wo2-sync-script` into the main `pr-19/sdkconfig-layering` branch.
- The commit is clean — `git log --oneline -3` on the branch to confirm.
- If doing a PR, the commit message from WO2 is already formatted correctly.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_board_config_api.md` — Why `env.BoardConfig().get("build.*")` must be used instead of `GetProjectOption` for `board_build.*` INI options
- `context_artifacts/graveyard_getprojectoption_board_build.md` — The silent failure of `GetProjectOption("board_build.cmake_extra_args")` and how it was diagnosed
- `context_artifacts/state_sync_sdkconfig.md` — Final state of `scripts/sync_sdkconfig.py` and `platformio.ini` changes
