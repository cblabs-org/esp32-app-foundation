# Deploy Safety Gates — Architecture Decision Record

## Context

During live OTA testing of PR-37, three distinct failure modes caused the device
to reject otherwise valid firmware. All three stem from the same root cause:
ESP-IDF's `esp_app_desc_t.version` field (embedded in the binary at compile time)
must exactly match the version string in the OTA manifest.

## Gate 1: Clean Working Tree

**Failure mode:** `git describe --dirty` appends `-dirty` to the version tag when
the working tree has uncommitted changes. ESP-IDF uses `git describe` to set
`PROJECT_VER`, which becomes `esp_app_desc_t.version`. A binary built from a dirty
tree has version `v0.0.12-dev.7-dirty` which never matches manifest version
`v0.0.12-dev.7`.

**Gate:** `git status --porcelain` must produce empty output.

## Gate 2: Tag Describes HEAD

**Failure mode:** When multiple tags point at HEAD, `git describe --tags` picks
the "highest" one by its own sorting rules, which may not match the `--tag`
argument passed to the deploy script. Result: binary has `v0.0.12-dev.6` but
manifest says `v0.0.12-dev.7`.

**Gate:** `git describe --tags --exact-match HEAD` must return the exact `--tag` value.

## Gate 3: Clean Build

**Failure mode:** ESP-IDF's CMake compiles `esp_app_desc.c` (which defines the
binary version struct) based on the cached `PROJECT_VER` variable. If only git
tags change (no source changes), the incremental build does not recompile this
file, so the binary retains the stale version string from the previous build.

**Gate:** When `--build` is specified, run `pio run -t clean` before `pio run`.
