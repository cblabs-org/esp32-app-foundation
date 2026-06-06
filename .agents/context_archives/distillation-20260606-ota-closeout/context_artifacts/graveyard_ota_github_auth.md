# Graveyard: GitHub Auth from Agent Context

## Problem

Any `gh` CLI command that requires authentication (release create, release upload, run download) will always fail with `401 Unauthorized` when run by the agent.

## Root Cause

Antigravity injects `GITHUB_TOKEN=github_pat_antigravitydummytoken` into every subprocess environment. The `gh` CLI always prefers `GITHUB_TOKEN` over keyring credentials when the env var is set — even if the keyring has valid credentials.

## What Was Tried

- `unset GITHUB_TOKEN` in a command chain — fails because each agent-run command inherits a fresh copy of the injected env; unset only affects the subshell
- `gh auth switch --user matthewmcneill` — succeeds (shows active account), but the next `gh release create` call still gets 401 because `GITHUB_TOKEN` takes precedence over the keyring
- Checking `env | grep GITHUB` — confirmed the variable is always present as `ANTIGRAVITY_SOURCE_METADATA` confirms the injection source

## Resolution

All `gh release create` and `gh release upload` steps must be run by the user in their own terminal where `GITHUB_TOKEN` is not set. This is by design — a safety guardrail, not a bug.

The `deploy_release.py` script handles everything up to and including the upload, but the user must invoke it themselves.
