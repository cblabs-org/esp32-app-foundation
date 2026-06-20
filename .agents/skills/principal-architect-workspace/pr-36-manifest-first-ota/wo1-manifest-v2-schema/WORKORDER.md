---
name: pr-36-wo1
description: Handoff instructions for Work Order 1 (Manifest v2 Schema + Parser)
complexity: standard
---

# Role Definition
You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective
Redesign the `OTAManifest` struct and parser to support the v2 multi-channel manifest schema with absolute URLs and manifest signing support. Update `generate_manifest.py` to produce the new format.

# Strict Boundary Box
- DO NOT touch `OTAUpdateManager`, `OTADownloadEngine`, or `GithubClient`.
- DO NOT modify `deploy_release.py` (that is WO4).
- DO NOT change the download or verification pipeline — only the data structures and parser.

# Tooling Constraints
Use the `platformio` MCP tools (`build_project`) to compile and verify your code instead of running raw `pio` CLI commands.

# Context
Read the project plan for full schema specification:
`.agents/skills/principal-architect-workspace/pr-36-manifest-first-ota/PROJECT.md`

# Line-Item Instructions

## OTAManifest.hpp — Struct Redesign

- [x] Add `static constexpr size_t kMaxChannels = 4;` (dev, beta, rc, stable).
- [x] Add `static constexpr size_t kMaxUrlLen = 256;` for absolute URL fields.
- [x] Modify `OTAAsset` — replace `char firmware[kMaxFilenameLen]` and `char signature[kMaxFilenameLen]` with `char firmwareUrl[kMaxUrlLen]` and `char signatureUrl[kMaxUrlLen]`. Keep `board`, `size`, `sha256` unchanged.
- [x] Create a new `OTAChannel` struct containing: `char version[kMaxVersionTagLen]`, `char name[16]`, `uint32_t securityVersion`, `OTAAsset assets[kMaxBoards]`, `uint8_t assetCount`.
- [x] Redesign `OTAManifest` to contain: `uint8_t manifestVersion`, `char manifestUrl[kMaxUrlLen]`, `OTAChannel channels[kMaxChannels]`, `uint8_t channelCount`. Remove the old flat `version`, `channel`, `securityVersion`, `assets`, `assetCount` fields.
- [x] Add a new free function declaration: `const OTAChannel* findChannel(const OTAManifest& manifest, const char* channelName);`
- [x] Update `findAssetForBoard()` signature to take `const OTAChannel&` instead of `const OTAManifest&`.
- [x] Preserve all existing docstrings and update them for the new schema.

## OTAManifest.cpp — Parser Redesign

- [x] Update `parseManifest()` to handle the new JSON structure: read `manifestVersion` (integer, e.g. `1`), `manifestUrl`, then iterate the `channels` object (keyed by channel name).
- [x] For each channel key, populate an `OTAChannel` entry — copy version, securityVersion, channel name (from the JSON key), and iterate its `assets` array.
- [x] For assets, read `firmwareUrl` and `signatureUrl` (absolute URLs) instead of `firmware` and `signature` (filenames).
- [x] Implement `findChannel()` — linear scan by channel name, returns `const OTAChannel*` or `nullptr`.
- [x] Update `findAssetForBoard()` to accept `const OTAChannel&`.
- [x] Bump the static manifest buffer comment to note 4096-byte requirement.

## generate_manifest.py — v2 Output

- [x] Update `build_manifest()` to accept a `base_url` parameter (e.g. `https://github.com/owner/repo/releases/download/v1.0.0`).
- [x] Add `--manifest-url` CLI argument (the self-referencing URL for the manifest).
- [x] Add `--base-url` CLI argument (prefix for firmware/signature asset URLs).
- [x] Output the v2 JSON structure with `manifestVersion: 1`, `manifestUrl`, and `channels` object (single channel per invocation — the deploy script calls it once per channel and merges, OR it takes `--channels` with comma-separated tags).
- [x] **Hard cut to v2** — no v1 backward compatibility. Remove the old v1 format output entirely.

# Verification Requirement
Compile the project using the PlatformIO MCP `build_project` tool to verify the struct changes compile cleanly (even though nothing uses them yet). **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.

---

# Status Update

**Status:** ✅ **COMPLETE**
**Completed:** 2026-06-20T11:18:00Z
**Commit:** `b1757e9` — `feat: OTAManifest v2 schema — multi-channel, absolute URLs, findChannel()`
**Branch:** `pr-36-manifest-first-ota-wo1-manifest-v2-schema`

**Files changed (4):**
- `lib/OTAManifest/OTAManifest.hpp` — v2 struct redesign
- `lib/OTAManifest/OTAManifest.cpp` — v2 parser implementation
- `scripts/generate_manifest.py` — v2 output with absolute URLs
- `test/test_unit/test_ota_manifest/test_ota_manifest.cpp` — 14 v2 test cases (all passing)

**Verification:** `test_unit/test_ota_manifest` — 14/14 PASSED on native target. Expected downstream breakage in `OTAUpdateManager.cpp` (WO3 scope).

**Distilled to:** `wo1-manifest-v2-schema/context_bridge.md`
