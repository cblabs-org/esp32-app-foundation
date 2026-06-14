# Graveyard: Workspace "branch" Mode Causes Permission Errors in PA Harness

## Symptom

Worker agent spawned with `Workspace: "branch"` immediately failed on every file read/write with permission denied errors, even for files clearly within the project root.

## Root Cause

`Workspace: "branch"` creates an isolated git worktree at a new path outside the parent workspace. The Antigravity harness does not automatically grant the worker file permissions to the new worktree path. Every `read_file` and `write_file` attempt therefore hits a permission boundary.

## Failed Attempts

- Worker tried to read `.agents/skills/principal-architect-workspace/pr-23-display-tab/wo1-display-tab/WORKORDER.md` → permission denied
- Worker tried to read project source files → permission denied
- Worker could not even read rules files to understand what was happening

The worker was killed after completing blocks A1–A3 (C++ changes in the parent workspace before the worktree was created).

## Fix

Use `Workspace: "inherit"` for all serialised worker agents in the Antigravity harness. The worker inherits the parent's workspace permissions and can access all project files directly.

`Workspace: "branch"` should ONLY be used for genuinely parallel workloads where each worker needs a completely isolated copy of the repository, AND where the user has explicitly pre-granted read/write permissions to the new worktree paths.

## Harness Adapter Update

Fix was applied to `.agents/skills/principal-architect/references/harnesses/antigravity.md` (commit `899e59f`) and pushed to the skill library at `https://github.com/matthewmcneill/agent-skills.git` on branch `feat/ag2-compat`.
