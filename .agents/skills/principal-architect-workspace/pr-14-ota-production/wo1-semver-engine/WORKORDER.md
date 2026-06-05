---
name: pr-14-wo1-semver-engine
description: Handoff instructions for Work Order 1 — SemVer 2.0 Engine
---

# WO1 — SemVer 2.0 Engine

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create a SemVer 2.0 engine for firmware version comparison on ESP32. The engine uses packed 32-bit integers for zero-allocation comparison on MCU, with a stack-based tag parser for git release tags. Integrate into the existing `OTAUpdateManager` to replace the current lexicographic `String::compareTo()` version comparison.

## Strict Boundary Box

- ✅ You MAY create new files in `lib/SemVer/`
- ✅ You MAY create new test files in `test/test_unit/`
- ✅ You MAY modify `platformio.ini` (build_flags only)
- ✅ You MAY modify `modules/OTAUpdateManager/OTAUpdateManager.cpp` (version comparison logic only)
- ✅ You MAY create `scripts/inject_version.py`
- ❌ Do NOT modify `modules/Application/` — that's WO2's domain
- ❌ Do NOT modify `lib/OTADownloadEngine/` — that's WO2's domain
- ❌ Do NOT modify `sdkconfig.defaults` — that's WO2's domain
- ❌ Do NOT modify `modules/ConfigManager/` — that's WO5's domain
- ❌ Do NOT modify `web/portal/` — that's WO6's domain
- ❌ Do NOT modify `lib/GithubClient/` — that's WO4's domain
- ❌ Do NOT create branches or manage git — the PA handles that
- ❌ Do NOT modify `.agents/` files except the WORKORDER checklist

## Project House Style

- All file names and library names MUST use PascalCase (e.g., `SemVer.hpp`, `SemVer.cpp`)
- All files MUST start with the standard license header (see any existing file for the template — "ESP32 App Foundation (c) 2026 CB Labs")
- All public functions MUST have Doxygen-style `@brief` / `@param` / `@return` comments
- Use `#pragma once` for header guards
- Use `#ifndef NATIVE_BUILD` guards for any ESP-IDF-specific code
- Prefer `char[]` / stack buffers over Arduino `String` for new code

## Implementation Checklist

### 1. Create `lib/SemVer/SemVer.hpp`
- [ ] Add standard license header
- [ ] Define `enum class Channel : uint8_t { DEV = 0, BETA = 2, RC = 3, STABLE = 15 }`
- [ ] Define `struct FirmwareVersion`:
  - `uint8_t major = 0`
  - `uint8_t minor = 0`
  - `uint8_t patch = 0`
  - `Channel channel = Channel::STABLE`
  - `uint8_t preNum = 0`
- [ ] Declare `uint32_t packVersion(const FirmwareVersion& v)` — encodes as `MAJOR(8)|MINOR(8)|PATCH(8)|CHANNEL(4)|PRE_NUM(4)`
- [ ] Declare `FirmwareVersion parseVersionTag(const char* tag)` — parses git tags like `v3.1.0`, `v3.1.0-beta.1`, `v3.1.0-dev.3`, `v3.1.0-rc.1`
- [ ] Declare comparison operators on `FirmwareVersion` (`<`, `>`, `==`, `!=`, `<=`, `>=`) — delegate to `packVersion()` for comparison
- [ ] Declare `const char* channelToString(Channel ch)` — returns "dev", "beta", "rc", "stable"
- [ ] Declare `Channel stringToChannel(const char* str)` — parses channel string

### 2. Create `lib/SemVer/SemVer.cpp`
- [ ] Implement `packVersion()`:
  ```cpp
  return (uint32_t(v.major) << 24) | (uint32_t(v.minor) << 16) | (uint32_t(v.patch) << 8) | (uint32_t(v.channel) << 4) | uint32_t(v.preNum);
  ```
- [ ] Implement `parseVersionTag()`:
  - Skip leading `v` or `V` if present
  - Parse `MAJOR.MINOR.PATCH` using `strtoul()` — no heap allocation
  - If `-` follows, parse channel name (`dev`, `beta`, `rc`) — compare with `strncmp()`
  - If `.` follows channel, parse pre-release number
  - If no pre-release suffix, default to `Channel::STABLE`, `preNum = 0`
  - Return a `FirmwareVersion` struct (by value, no heap)
- [ ] Implement `channelToString()` and `stringToChannel()`
- [ ] Implement comparison operators — all delegate to `packVersion(a) <op> packVersion(b)`

### 3. Create `test/test_unit/test_semver.cpp`
- [ ] GoogleTest suite `SemVerTest`
- [ ] Test `packVersion` produces correct packed values
- [ ] Test `parseVersionTag("v3.1.0")` → major=3, minor=1, patch=0, channel=STABLE, preNum=0
- [ ] Test `parseVersionTag("v3.1.0-beta.1")` → channel=BETA, preNum=1
- [ ] Test `parseVersionTag("v3.1.0-dev.3")` → channel=DEV, preNum=3
- [ ] Test `parseVersionTag("v3.1.0-rc.1")` → channel=RC, preNum=1
- [ ] Test `parseVersionTag("3.1.0")` → works without leading `v`
- [ ] Test comparison: `v3.1.0-stable > v3.1.0-beta.1`
- [ ] Test comparison: `v3.1.0-beta.2 > v3.1.0-beta.1`
- [ ] Test comparison: `v3.2.0-dev.1 > v3.1.0`  (higher minor trumps channel)
- [ ] Test comparison: `v3.1.0 == v3.1.0`
- [ ] Test comparison: `v3.0.0-stable > v2.255.255-stable` (major boundary)
- [ ] Test `channelToString()` round-trips with `stringToChannel()`

### 4. Modify `platformio.ini`
- [ ] Add to `[env]` or common build_flags section:
  ```ini
  -DVERSION_MAJOR=0
  -DVERSION_MINOR=1
  -DVERSION_PATCH=0
  ```
  These are static defaults for local dev builds. CI/tag builds will override them via `inject_version.py`.

### 5. Create `scripts/inject_version.py`
- [ ] PlatformIO pre-build script (`Import("env")`)
- [ ] Run `git describe --tags --exact-match` to check if HEAD is a tag
- [ ] If tag found (e.g., `v3.1.0-beta.1`):
  - Parse the tag components
  - Override `build_flags` with `-DVERSION_MAJOR=3 -DVERSION_MINOR=1 -DVERSION_PATCH=0 -DVERSION_CHANNEL=2 -DVERSION_PRE_NUM=1`
- [ ] If no tag (local dev build): no-op — static defaults from `platformio.ini` apply
- [ ] Log the version being built: `print(f"[inject_version] Building version: ...")` 

### 6. Modify `modules/OTAUpdateManager/OTAUpdateManager.cpp`
- [ ] Add `#include "../../lib/SemVer/SemVer.hpp"` (or appropriate relative path)
- [ ] In `checkForUpdate()` (or `performUpdateTask()`), locate the existing version comparison logic
- [ ] Replace the `String::compareTo()` lexicographic check with:
  ```cpp
  FirmwareVersion local = { VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, Channel::STABLE, 0 };
  FirmwareVersion remote = parseVersionTag(releaseTagName);
  if (remote > local) { /* update available */ }
  ```
- [ ] Preserve the existing flow — just swap the comparison, don't restructure the method

## Verification

- [ ] `pio test -e native -f test_semver` — all unit tests pass
- [ ] `pio run -e esp32dev` — hardware build succeeds
- [ ] `pio run -e esp32s3nano` — hardware build succeeds

## Completion

Once all checklist items are verified, **STOP and send a message back to the Principal Architect** confirming:
1. All tests pass
2. Both hardware builds succeed  
3. Any issues or decisions that need escalation

Do NOT wrap up, distill, or commit until the PA explicitly approves.
