# ADR: Framework LittleFS Data Merge Strategy

## Status
Accepted

## Context
The framework owns `timezones.json` (used by TimeManager for the timezone picker). It lived only in the root project's `data/` directory. Sub-apps (template-app, sample-app) had empty `data/` directories, meaning `pio run -t uploadfs` produced an empty LittleFS filesystem — breaking the timezone picker until manually re-downloaded.

## Decision
Created `scripts/merge_framework_data.py` — a PlatformIO pre-build script that copies framework-owned data files from the root `data/` directory into the sub-app's `data/` at build time.

## Alternatives Considered

### Symlinks
Rejected — PlatformIO's `uploadfs` doesn't follow symlinks reliably across all platforms (especially Windows). Also creates fragile path dependencies.

### PIO `data_dir` override
PIO's `data_dir` setting can only point to one directory. We'd lose the ability for sub-apps to have their own app-specific data files alongside framework files.

### Git submodule / shared data folder
Over-engineered for a single file. Also forces sub-app developers to understand git submodules.

### Copy at build time (CHOSEN)
Simple, reliable, cross-platform. Follows the existing `portalBuilder.py` pattern. App-specific files take priority (framework copy is skipped if a file with the same name already exists). No-ops when run from the root project (source == destination).

## Key Design Decisions

1. **App-priority rule**: If a sub-app already has a file with the same name, the framework's copy is NOT overwritten. This lets apps customize framework defaults.
2. **Relative paths from CWD**: PlatformIO sets CWD to the project directory. Using `../data/` resolves correctly for sub-apps and avoids the `__file__` NameError in SCons context (SCons doesn't define `__file__`).
3. **Gitignored copies**: The copied `timezones.json` is added to each sub-app's `.gitignore` so it's never committed — only the root `data/timezones.json` is the source of truth.
