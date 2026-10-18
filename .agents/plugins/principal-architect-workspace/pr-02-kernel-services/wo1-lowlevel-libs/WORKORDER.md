---
title: "WO1: Low-level libs"
project: "PR-02: Kernel Services"
branch: "pr-02-wo1-lowlevel-libs"
status: "QUEUED"
---
## MANDATORY SKILL BOOT SEQUENCE — Execute FIRST

Before reading any code or writing any file, run these exact view_file calls in order:
1. view_file `.agents/skills/pio-manager/SKILL.md`
2. view_file `.agents/skills/embedded-systems/SKILL.md`
3. view_file `.agents/skills/house-style-docs/SKILL.md`
4. view_file `.agents/skills/oo-expert/SKILL.md`
5. view_file `.agents/skills/pio-manager/references/mcp-agent-reference.md`

Do NOT proceed past this block until all five are confirmed loaded.
---

## Objective
Implement the low-level libraries for the Kernel Services phase: `deviceCrypto`, `logger`, `md5Utils`, and `githubClient`.

## Constraints & Requirements
- **Directory**: All libraries must go into `lib/` (e.g. `lib/deviceCrypto/`, `lib/logger/`, `lib/md5Utils/`, `lib/githubClient/`).
- **ESP-IDF Calls**: You must use native-build guards (`#ifndef PIO_UNIT_TESTING` or similar) on all ESP-IDF calls, so they compile cleanly under `native` test environments.
- **Reference Mining**: You may look at `departures-board/lib/*` for patterns, but all code must be written fresh. DO NOT blindly copy files.
- **House Style**: Apply the `house-style-docs` standards to every header and source file.
- **Branch**: Create and checkout branch `pr-02-wo1-lowlevel-libs` derived from `pr-02-kernel-services`.

## Deliverables
- [ ] `lib/deviceCrypto/` — hardware MAC key derivation, AES-256 pattern.
- [ ] `lib/logger/` — ESP-IDF wrapper pattern (add `LOG_SECURE()` macro).
- [ ] `lib/md5Utils/` — streaming MD5 for OTA.
- [ ] `lib/githubClient/` — HTTPS + cert validation pattern.

## Closure Gate
- **Build Verification**: Both `esp32dev` and `esp32s3nano` must build clean with the new libraries.
- Run `mcp_platformio_build_project` with `background: true` on both environments and confirm success before ending your session.
