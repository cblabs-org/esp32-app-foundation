---
name: pr-36-wo2
description: Handoff instructions for Work Order 2 (Manifest Fetcher + Signature Verification)
complexity: architect
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Implement a manifest fetcher in `OTAUpdateManager` that downloads `manifest.json` + `manifest.json.sig`, verifies the manifest signature using the existing `IOTAVerifier` infrastructure, and implements the URL fallback mechanism (learned URL → compiled-in URL).

# Strict Boundary Box
- DO NOT touch `GithubClient`, `deploy_release.py`, or `generate_manifest.py`.
- DO NOT rewire the OTA pipeline yet (that is WO3) — build the fetcher as a standalone private method.
- DO NOT modify `OTADownloadEngine`.
- Reuse the existing `IOTAVerifier` / `MbedTLSSignatureVerifier` for signature verification.

# Tooling Constraints
Use the `platformio` MCP tools (`build_project`) to compile and verify your code instead of running raw `pio` CLI commands.

# Context
Read the project plan for the full URL resilience design and verification rules:
`.agents/plugins/principal-architect-workspace/pr-36-manifest-first-ota/PROJECT.md`

# Line-Item Instructions

## OTAUpdateManager.hpp — New Private Method + Config

- [ ] Add a private method: `bool fetchAndVerifyManifest_(OTAManifest& manifest);` — returns true on success, false on failure (sets error state internally).
- [ ] Add a private member: `char learnedManifestUrl_[256]` — persisted via `IConfigurable`, initially empty.
- [ ] Add the `learnedManifestUrl_` field to the `IConfigurable` `applyConfig()` / `getConfig()` methods (persisted in `/mod/ota.json`).

## OTAUpdateManager.cpp — Manifest Fetcher Implementation

- [ ] Implement `fetchAndVerifyManifest_()` with the following logic:
  1. Determine URL order: if `learnedManifestUrl_[0] != '\0'`, try it first; otherwise use `_config->otaManifestUrl`.
  2. For each URL attempt:
     a. Download `manifest.json` into the static 4096-byte buffer (CA-pinned → insecure fallback, same pattern as existing code).
     b. Download `manifest.json.sig` into a 256-byte signature buffer. Construct the sig URL by appending `.sig` to the manifest URL.
     c. Verify the signature using the existing `IOTAVerifier` (call `begin()` with the public key, `update()` with the manifest bytes, `verify()` with the sig bytes).
     d. If verification passes → parse the manifest using `parseManifest()` → return true.
     e. If verification fails → log a warning and fall back to the next URL.
  3. If the learned URL fails and the compiled-in URL succeeds, log that fallback was used.
  4. If both fail → set `_state = OTAState::Error`, set `_lastOtaError`, publish progress, return false.
- [ ] After successful verification and parse: if `manifest.manifestUrl[0] != '\0'` and differs from `learnedManifestUrl_`, update `learnedManifestUrl_` and persist via config save. Log the URL migration.
- [ ] Bump the static `manifestBuf` from 2048 to 4096 bytes.
- [ ] Remove (or guard with `#if 0`) the old `downloadAndParseManifestVersion_()` method — it is superseded.

# Verification Requirement
Compile the project using the PlatformIO MCP `build_project` tool. The method won't be called yet (WO3 wires it), but it must compile cleanly. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.

# Status Update

**Status:** ✅ COMPLETE — 2026-06-20T11:30:00Z
**Commit:** `928466e` on branch `pr-36-manifest-first-ota-wo2-manifest-fetcher`
**Build:** esp32dev ✅ | esp32s3nano ✅
**Session:** `6db97e3f-4513-413e-a1df-e6194eb1b11f`

All checklist items implemented. `fetchAndVerifyManifest_()` compiles cleanly but is not yet wired into the pipeline (WO3). Legacy pipeline shims applied to resolve pre-existing v1→v2 compile errors. Distillery output saved to `context_bridge.md` + `context_artifacts/`.
