---
name: pr-28-wo11
description: Arduino String → std::string migration for authored lib/ APIs (GithubClient, OTADownloadEngine, MD5Utils, Logger)
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

# Objective

Migrate all authored `lib/` libraries from Arduino `String` to `std::string`
in both public APIs and internal implementation. This completes the WO6
migration that covered `modules/` but left `lib/` untouched.

**Key constraint:** These are ALL project-authored libraries (copyright
"ESP32 App Foundation (c) 2026"). None are third-party imports.

# Strict Boundary Box

- **ONLY** modify files under `lib/GithubClient/`, `lib/OTADownloadEngine/`,
  `lib/MD5Utils/`, and `lib/Logger/`.
- You MAY modify `modules/OTAUpdateManager/` ONLY to update call sites that
  pass `String` to these lib APIs.
- Do **NOT** change third-party framework API interop (ArduinoJson, PsychicHttp,
  base64) in module files — those stay as Arduino `String`.
- Do **NOT** modify test files unless they directly test these library APIs.
- Do **NOT** create or push any beta or stable git tags. You MAY create dev
  tags per the versioning rules below.

# Versioning Rules

Read `.agents/rules/03-versioning.md` for full details. Summary:
- Check `git tag --sort=-v:refname | head -1` to find the current latest tag.
- If you do a flash test build with new functionality, increment PRE_NUM and
  tag **before** building: `git tag v0.0.10-dev.N && git push origin v0.0.10-dev.N`
- `inject_version.py` reads the tag at build time — tag MUST be on HEAD before build.
- You MAY create dev tags autonomously.
- You MUST NOT create beta or stable tags.

# Device Info

- IP: `192.168.20.186`
- Board: ESP32-S3 NanoS3
- PlatformIO env: `esp32dev`
- The device MCP client skill is at `.agents/skills/device-mcp-client/SKILL.md`

# Checklist

## 1. Logger — Migrate internals to std::string (keep String overload)

**Files:** `lib/Logger/Logger.hpp`, `lib/Logger/Logger.cpp`

Logger should:
- **Keep** `registerSecret(const String& secret)` as a **convenience overload**
  for callers that still use Arduino `String` (framework API boundaries).
- **Change** the internal `secrets` vector from `std::vector<String>` to
  `std::vector<std::string>`.
- **Change** the `String` overload to convert to `std::string` internally
  (delegate to the `std::string` overload, not the other way around).
- **Change** `redactInPlace()` device path (non-NATIVE_BUILD) from `String`
  locals to `std::string` (it already uses `std::string` in NATIVE_BUILD).
- **Remove** the `#ifdef NATIVE_BUILD` String stub class — after this change
  Logger should use `std::string` internally on both platforms. The stub may
  still be needed for the `const String&` overload — keep it if so, or include
  `<Arduino.h>` for the real `String` on device builds.

## 2. MD5Utils — Migrate to std::string

**Files:** `lib/MD5Utils/MD5Utils.hpp`, `lib/MD5Utils/MD5Utils.cpp`

- [ ] Change return types from `String` to `std::string`.
- [ ] Change parameter types from `const String&` to `const std::string&`.
- [ ] Update internal implementation to use `std::string` operations.
- [ ] Update all callers in `modules/` (grep for `MD5Utils::` and `md5` calls).

## 3. GithubClient — Migrate to std::string

**Files:** `lib/GithubClient/GithubClient.hpp`, `lib/GithubClient/GithubClient.cpp`

- [ ] Change constructor params from `const String&` to `const std::string&`.
- [ ] Change all member variables from `String` to `std::string`.
- [ ] Change method return types and params from `String` to `std::string`.
- [ ] Update internal `.indexOf()` → `.find()`, `.substring()` → `.substr()`,
  `.length()` → `.length()` (same), `.c_str()` → `.c_str()` (same).
- [ ] Replace Arduino `String` concatenation (`+`) with `std::string` equivalents.
- [ ] Update all callers in `modules/OTAUpdateManager/` that pass `String` to
  GithubClient methods.

## 4. OTADownloadEngine — Migrate to std::string

**Files:** `lib/OTADownloadEngine/OTADownloadEngine.hpp`,
`lib/OTADownloadEngine/OTADownloadEngine.cpp`

- [ ] Change `fetchSignature()` and `downloadAndFlash()` params from
  `const String&` to `const std::string&`.
- [ ] Update internal `String` locals to `std::string`.
- [ ] Update all callers in `modules/OTAUpdateManager/`.

## 5. Cross-cutting: Update OTAUpdateManager call sites

**Files:** `modules/OTAUpdateManager/OTAUpdateManager.cpp`

- [ ] Update all call sites that pass `String` to the migrated libraries.
- [ ] This includes constructing `GithubClient`, calling `downloadAndFlash()`,
  `fetchSignature()`, and any `MD5Utils` calls.
- [ ] Use `std::string` throughout — do NOT convert back to `String` just to
  call these APIs.

# Verification

1. **Native build:** Run PlatformIO native build — must compile cleanly.
2. **ESP32 build:** Run PlatformIO esp32dev build — must compile cleanly.
3. **Native tests:** Run PlatformIO native tests — all 525+ must pass.
4. **Flash test:** Flash to the device at 192.168.20.186 and verify via serial
   monitor that OTA check completes successfully (or fails gracefully with
   the JSON parse error — that's expected and unrelated).

**CRITICAL:** Before distilling or committing final work, send a message back
to the Principal Architect confirming implementation status. Do not wrap up
until explicitly approved.
