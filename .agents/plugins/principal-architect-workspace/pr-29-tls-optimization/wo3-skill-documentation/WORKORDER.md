---
name: pr-27-wo3
description: Handoff instructions for Work Order 3 — Framework-expert skill TLS documentation
complexity: standard
---

# Work Order 3 — TLS Memory Optimization Documentation

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Write a TLS memory optimization reference document for the framework-expert skill,
incorporating the measured results from WO2. Update the framework-expert SKILL.md
to link the new reference.

## Versioning

Read and follow `.agents/rules/03-versioning.md`. Documentation-only changes do not
warrant a new dev tag.

## Strict Boundary Box

- **ONLY** create/modify files within `.agents/skills/framework-expert/`
- Do **NOT** modify `sdkconfig.defaults` or any source code
- Do **NOT** create any branches or make git commits
- You **MUST** read the WO2 results file at:
  `.agents/plugins/principal-architect-workspace/pr-27-tls-optimization/wo2-build-test-measure/RESULTS.md`
  to incorporate measured data.

## Prerequisites

WO2 must be complete with `RESULTS.md` containing measured heap and flash metrics.

## Line-Item Instructions

- [ ] **1. Read the WO2 results** from:
  `.agents/plugins/principal-architect-workspace/pr-27-tls-optimization/wo2-build-test-measure/RESULTS.md`

- [ ] **2. Read the TLS analysis** from:
  `.gemini/antigravity/brain/07cb3857-55da-4085-b60a-2fecec6a6dec/tls_optimization_analysis.md`
  (This contains the full investigation — wolfSSL evaluation, BearSSL verdict, etc.)

- [ ] **3. Create the TLS optimization reference document** at:
  `.agents/skills/framework-expert/performance/tls-memory-optimization.md`

  The document MUST cover:

  ### a) TLS Memory Budget Guidance
  - Per-connection heap cost breakdown (idle vs active transfer)
  - What the framework's sdkconfig.defaults provides out-of-the-box
  - How much headroom a downstream app developer has for additional TLS connections

  ### b) sdkconfig Settings Rationale
  - Explain each PR-27 setting and WHY it was chosen
  - `CONFIG_MBEDTLS_DYNAMIC_BUFFER=y` — idle savings, fragmentation trade-off
  - `CONFIG_MBEDTLS_SSL_KEEP_PEER_CERTIFICATE=n` — cert freed after handshake
  - `CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_NONE=y` — no built-in CAs
  - Disabled features (renegotiation, ALPN, session tickets)

  ### c) Measured Results
  - Include the actual before/after numbers from WO2 RESULTS.md
  - Flash size savings
  - Heap metrics (idle and during OTA if available)

  ### d) Certificate Bundle Guidance for Downstream Developers
  - Framework defaults to `NONE` — why
  - **Option A**: Pin specific CA certs via `setCACert()` (preferred — smallest footprint)
  - **Option B**: Re-enable `CMN` or `FULL` in board-specific `sdkconfig.defaults.*` delta:
    ```ini
    CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_NONE=n
    CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN=y
    ```
  - Note: this pattern is complementary to future S3 CA pinning for OTA security

  ### e) The 16 KB RX Buffer
  - Why `CONFIG_MBEDTLS_SSL_IN_CONTENT_LEN=16384` cannot be reduced
  - GitHub CDN sends full-size TLS records — smaller buffers cause connection failure
  - MFLN is a dead end (CDNs don't honour it)

  ### f) Fragmentation Monitoring
  - Watch `heap_max_alloc` vs `heap_free` via `/api/system` endpoint
  - A large gap indicates fragmentation
  - Dynamic buffers can worsen fragmentation under sustained TLS churn

  ### g) Alternative TLS Libraries (Summary)
  - **wolfSSL**: Viable but NOT a drop-in swap — WiFiClientSecure (Arduino) directly
    calls mbedTLS APIs. Would require rewriting to `esp_http_client`. GPLv3 licensing concern.
  - **BearSSL**: Not viable — no ESP-IDF integration, no hardware crypto, Arduino-only.

- [ ] **4. Update the framework-expert SKILL.md** to reference the new document.
  Look at the existing skill structure — if there's a reference table, add an entry.
  If not, add a link in the appropriate section. The trigger terms already include
  "TLS memory", "heap usage", "PSRAM", "out of memory" which will route queries here.

- [ ] **5. Verify consistency**: Read back the created file and confirm all sections
  are present and the measured data matches WO2 results.

## Verification Requirement

After completing all checklist items:
1. Confirm the new reference document exists and covers all required sections
2. Confirm SKILL.md is updated with a link to the new reference
3. **STOP and send a message back to the Principal Architect** reporting completion. Do NOT commit, do NOT distill, and do NOT make any further changes until explicitly approved.
