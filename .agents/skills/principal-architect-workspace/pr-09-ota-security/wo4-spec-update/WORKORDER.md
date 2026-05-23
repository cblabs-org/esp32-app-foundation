---
name: pr-09-ota-security-wo4
description: Handoff instructions for Work Order 4 — Update design_specification.md §13.3 to document the complete OTA cryptographic verification pipeline
---

# WO4 — Design Specification §13.3 Update

You are an expert technical writer and execution-focused worker agent. **DO NOT trigger or act as the principal-architect.** Your task is strictly limited to updating the design specification document.

## Objective

Rewrite §13.3 ("OTA Security & Blessing Ceremony") in `docs/design_specification.md` to fully document the cryptographic verification pipeline implemented by WO1–WO3. The existing content is outdated — it mentions "streaming MD5" which has been replaced by RSA+SHA256 signature verification.

## Strict Boundary Box

- **ONE file to modify**: `docs/design_specification.md`
- **ONE section to rewrite**: §13.3 (lines 1428–1446, from "### 13.3" to just before "### 13.4")
- **DO NOT** modify any code files
- **DO NOT** modify any other sections of the spec
- **DO NOT** create git branches

## Context Files — Read These First

Before writing, you MUST read these files to understand the implemented architecture:

1. `include/interfaces/IOtaVerifier.hpp` — the verifier interface contract
2. `lib/otaDownloadEngine/MbedTLSSignatureVerifier.hpp` — the MbedTLS implementation
3. `lib/otaDownloadEngine/OtaDownloadEngine.hpp` — the download engine API
4. `lib/otaDownloadEngine/OtaError.hpp` — error codes
5. `modules/otaUpdateManager/otaUpdateManager.hpp` — the full orchestrator (OtaState, OtaProgressEvent, API)
6. `modules/otaUpdateManager/otaUpdateManager.cpp` — the implementation (8-step pipeline)
7. `include/FrameworkConfig.hpp` — the 3 new OTA config fields
8. `modules/u8g2Display/u8g2DisplayModule.hpp` lines 87–92 — OtaProgressPayload
9. Read `.agents/skills/house-style-docs/SKILL.md` for documentation standards

## Content Requirements

The rewritten §13.3 must cover ALL of the following subsections. Use the existing spec's style (markdown headers, tables, code blocks, blockquotes for prohibitions/notes).

### §13.3 OTA Security & Blessing Ceremony

#### §13.3.1 Overview
- The framework provides a complete OTA update pipeline: release discovery → binary download → cryptographic verification → flash → reboot
- Three-layer architecture:
  - **`otaUpdateManager`** (module) — orchestrator, state machine, FreeRTOS task
  - **`OtaDownloadEngine`** (lib) — redirect-aware HTTP download, streaming flash
  - **`IOtaVerifier` / `MbedTLSSignatureVerifier`** (interface + lib) — RSA+SHA256 signature verification via mbedtls

#### §13.3.2 Cryptographic Verification Pipeline
- `IOtaVerifier` interface contract: `begin(publicKey)` → `update(chunk, len)` per chunk → `verify(signature, sigLen)` post-download
- Default implementation: `MbedTLSSignatureVerifier` using `mbedtls_pk`, `mbedtls_sha256`, `mbedtls_md`
- RAII pattern — destructor calls `mbedtls_pk_free()` + `mbedtls_sha256_free()`
- Public key is PEM-encoded RSA, injected via `FrameworkConfig.otaPublicKey`
- If `otaPublicKey` is `nullptr`, signature verification is **skipped** (degraded security mode)

#### §13.3.3 Download Engine
- `OtaDownloadEngine` handles:
  - Redirect-following HTTP GET (up to 5 hops, HTTP/1.0 to avoid chunked-transfer)
  - Separate `.sig` file fetch with its own redirect chain
  - ESP32 binary magic byte validation (`0xE9`)
  - Partition space validation (binary size ≤ available OTA partition)
  - Chunked streaming write to `Update.h` with per-chunk `IOtaVerifier::update()`
  - Post-download `IOtaVerifier::verify()` against the fetched signature
  - Progress callbacks for UI integration
- `OtaError` enum: `None`, `HttpConnectionFailed`, `HttpError`, `TooManyRedirects`, `InvalidBinaryHeader`, `PartitionTooSmall`, `FlashWriteFailed`, `SignatureVerificationFailed`, `DownloadFailed`
- The engine does NOT own the `WiFiClient` — caller passes by reference

#### §13.3.4 Orchestrator State Machine
Document the `OtaState` enum and transitions:
```
Idle → Checking → Idle              (no update found)
Idle → Downloading → Verifying → Complete → (reboot)
Any  → Error                        (on failure)
```
- `checkForUpdate()`: queries `GithubClient` for latest release metadata
- `performUpdate()`: launches a FreeRTOS task (8KB stack, priority 1) that runs the full 8-step pipeline
- The FreeRTOS task ensures `tick() ≤ 10ms` is never violated by download I/O

#### §13.3.5 FrameworkConfig OTA Fields
Document the 3 new fields in a table:

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `otaPublicKey` | `const char*` | `nullptr` | PEM-encoded RSA public key for signature verification |
| `otaGithubRepo` | `const char*` | `nullptr` | GitHub repository in "owner/repo" format |
| `otaGithubToken` | `const char*` | `nullptr` | Optional GitHub access token for private repos |

Plus the existing fields: `firmwareUpdatesEnabled`, `otaQuietHour`.

#### §13.3.6 EventBus Integration
- The orchestrator publishes `OtaProgressEvent` on topic `"ota.progress"`
- Struct fields: `float progress`, `const char* status`, `bool complete`, `bool success`
- Layout-compatible with `OtaProgressPayload` in `u8g2DisplayModule.hpp` — the display module subscribes and renders the `OtaProgressBoard`
- Events are published at each pipeline step and on progress callback from the download engine

#### §13.3.7 Firmware Blessing Ceremony
**Keep the existing blessing ceremony content** (lines 1435–1446) exactly as-is, but:
- Update the introductory sentence to note it applies AFTER the new pipeline completes
- Keep the prohibition blockquote about blessing at download time

#### §13.3.8 REST API
Document the new routes:
- `POST /api/ota { "action": "checkGithub" }` — triggers `checkForUpdate()` + `performUpdate()` 
- `POST /api/ota { "action": "status" }` — returns JSON: `{ "state": "idle|checking|downloading|verifying|complete|error", "lastError": "..." }`
- Plus the existing `rollback` and `markValid` actions

#### §13.3.9 Security Considerations
- **Public key management**: The consuming app provides the PEM key — never hardcoded in the framework
- **WiFiClientSecure**: Heap-allocated per §14.3 memory rules
- **GitHub CDN**: Uses `setInsecure()` for CDN redirect hops (different cert chains than `api.github.com`)
- **Degraded mode**: If no public key configured, downloads proceed without signature verification — log warning
- **Rollback safety**: The blessing ceremony ensures bad firmware is automatically reverted

## Formatting Requirements

1. Match the spec's existing style: markdown headers (`###`, `####`), tables, code blocks, blockquotes
2. Use the same notation patterns seen elsewhere in the spec (e.g., `§14.3` cross-references)
3. Keep the content factual and architectural — no implementation tutorial style
4. Target ~150–200 lines for the entire rewritten §13.3

## Verification

After completing the edit:
1. Visually confirm §13.3 reads coherently from §13.2 into §13.4
2. Verify no other sections were modified
3. **STOP and send a message back to me (the Principal Architect)** reporting completion
4. **DO NOT** wrap up, distill, or commit until I explicitly approve

---

## Execution Status

| Field | Value |
|-------|-------|
| **Status** | ✅ COMPLETE |
| **Commit** | `c6ebb51` |
| **Branch** | `pr-09-ota-security-wo4-spec-update` |
| **Session** | `83646401-387d-4ff9-8b44-04b9ac3936c3` |
| **Date** | 2026-05-22 |
| **Lines Added** | 221 insertions, 5 deletions |
| **Section Size** | 236 lines (§13.3.1–§13.3.9) |

### Deliverables
- [x] §13.3.1 Architecture Overview
- [x] §13.3.2 Cryptographic Verification Pipeline
- [x] §13.3.3 Download Engine (with full OtaError table)
- [x] §13.3.4 Orchestrator State Machine
- [x] §13.3.5 FrameworkConfig OTA Fields
- [x] §13.3.6 EventBus Integration
- [x] §13.3.7 Firmware Blessing Ceremony (preserved original)
- [x] §13.3.8 REST API
- [x] §13.3.9 Security Considerations
