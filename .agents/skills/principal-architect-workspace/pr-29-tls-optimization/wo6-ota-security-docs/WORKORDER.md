---
name: pr-29-wo6
description: Handoff instructions for Work Order 6 — OTA security & strategy documentation
complexity: standard
---

# Work Order 6 — OTA Security & Strategy Documentation

## Role Definition

You are an expert senior software engineer and technical writer. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Document the design decisions and tradeoffs from the OTA strategy evaluation
session. Update the framework-expert skill's security references and the
developer-facing OTA guide with CA pinning rationale, PAT management guidance,
and the repo split strategy.

## Versioning

Read `.agents/rules/03-versioning.md`. This is a documentation-only change —
no version tag increment is needed.

## Strict Boundary Box

- You **MAY** modify files in `.agents/skills/framework-expert/security/`
- You **MAY** modify `.agents/skills/framework-expert/SKILL.md`
- You **MAY** modify `docs/OTADeveloperGuide.md`
- You **MAY** create new files in `.agents/skills/framework-expert/security/`
- Do **NOT** modify any firmware source files
- Do **NOT** create any branches or make git commits

## Prerequisites

> **CRITICAL**: WO4 and WO5 must be completed before this work order.
> You must read the **actual source code** to document the implemented patterns —
> do NOT work from design docs or implementation plans alone.

Before starting, read these source files to understand the implemented patterns:
- `modules/OTAUpdateManager/OTAUpdateManager.cpp` — the CA-pinned fallback implementation
- `lib/GithubClient/GithubClient.cpp` — the API connection fallback implementation
- `lib/GithubClient/ota_root_cas.h` — the dual-CA bundle
- `scripts/deploy_release.py` — the `--repo` flag implementation

## Line-Item Instructions

### Step 1: Update ota-security.md

- [ ] **1a.** Read `.agents/skills/framework-expert/security/ota-security.md` (current content).

- [ ] **1b.** Replace the "Transport Security for OTA Downloads" section
  (currently lines 55-64) with the new CA-pinned-with-fallback model. Cover:
  - Dual-CA bundle (`ota_root_cas.h`): Sectigo E46 (API) + ISRG Root X1 (CDN)
  - Try CA-validated first → fallback to `setInsecure()` if handshake fails
  - Self-healing property: degraded OTA still works, next firmware ships corrected certs
  - Firmware signature remains the mandatory trust anchor; transport cert is defense-in-depth
  - The dirty TLS context problem: why retry requires a fresh `WiFiClientSecure`

- [ ] **1c.** Add a new section **"NVS Encryption Analysis"** after the transport
  security section. Cover the three tiers:
  - Tier 1 (key partition): no eFuses, keys on flash — doesn't protect against physical extraction
  - Tier 1.5 (HMAC, ESP32-S3 only): 1 eFuse, real protection, but board-specific
  - Tier 2 (flash encryption): full protection, requires eFuses, changes entire dev workflow
  - Decision: none warranted for current threat model (only DeviceCrypto master key in NVS)

### Step 2: Update tls-tradeoffs.md

- [ ] **2a.** Read `.agents/skills/framework-expert/security/tls-tradeoffs.md` (current content).

- [ ] **2b.** Update the "`setInsecure()` — When It's Acceptable" section
  (currently lines 43-47) to reflect the new model:
  - OTA now uses CA-pinned-with-fallback, not blanket `setInsecure()`
  - `setInsecure()` is the fallback path, not the default
  - Reference the specific fallback trigger: TLS handshake failure (CA rotation)

- [ ] **2c.** Add a new subsection **"Fresh `WiFiClientSecure` on TLS Retry"**
  in the TLS Tradeoffs doc. This is a general coding pattern rule:
  - After a failed TLS handshake, the mbedTLS context is in a dirty state
  - Cannot call `setInsecure()` on the same client and retry
  - Must `delete` the old client and allocate fresh via `new (std::nothrow)`
  - Applies anywhere TLS retry logic is used, not just OTA

### Step 3: Create pat-management.md

- [ ] **3a.** Create `.agents/skills/framework-expert/security/pat-management.md`
  as a new agent reference document. Include:

  **PAT Scope Requirements:**
  - Fine-grained token, single repository
  - Permission: `Contents: Read` only
  - No user permissions
  - No expiration (for binary distribution model)

  **Expiration Policy Rationale:**
  - Binary distribution means firmware ships with compiled-in PAT
  - Devices may be offline for months — expired PAT = bricked OTA
  - Fine-grained scope is the security boundary, not expiry

  **Blast Radius Analysis Table:**
  | Action | Possible with extracted PAT? |
  |---|---|
  | Read repository source code | ✅ Yes |
  | Read repository releases/assets | ✅ Yes |
  | Write/push to repository | ❌ No |
  | Access other repositories | ❌ No |
  | Access user account settings | ❌ No |
  | Create issues/PRs | ❌ No |

  **Source-Code Exposure Vulnerability:**
  - GitHub fine-grained PATs with `Contents: Read` grant access to source code, not just releases
  - GitHub has no "releases-only" read permission
  - An attacker who extracts the PAT from the firmware binary can clone the private repo

  **Repo Split Strategy:**
  - **Option A — Single private repo with PAT**: simpler setup, acceptable when source exposure is tolerable
  - **Option B — Separate public releases repo**: eliminates PAT from firmware entirely, maximum security
  - **Option C — Separate private releases repo with PAT**: source code in different repo, PAT only exposes release binaries
  - How to configure each:
    - `FrameworkConfig.otaGithubRepo` in `main.cpp`
    - `deploy_release.py --repo owner/releases-repo`
    - `otaSecrets.hpp` — include PAT or omit for public repos

  **Design Decisions:**
  - D1: Why no third-party OTA library was adopted
  - D2: Why the PAT stays compiled in the binary (not NVS)
  - D7: Why the OTA repo is configurable

- [ ] **3b.** Add a "See Also" section linking to `ota-security.md`,
  `tls-tradeoffs.md`, and `credential-storage.md`.

### Step 4: Update SKILL.md

- [ ] **4a.** Read `.agents/skills/framework-expert/SKILL.md` (current content).

- [ ] **4b.** Add trigger terms to the description frontmatter. Append after the
  existing trigger list (before the closing period):
  ```
  Also trigger on: "PAT", "personal access token", "OTA token",
  "releases repo", "split repo", "CA fallback", "dirty TLS context",
  "CA rotation", "NVS encryption".
  ```

- [ ] **4c.** Add a new row to the progressive disclosure table (after the
  Security row):
  ```
  | **PAT & Repository Config** (Token creation, repo split, blast radius) | `security/pat-management.md` |
  ```

### Step 5: Update OTADeveloperGuide.md

- [ ] **5a.** Read `docs/OTADeveloperGuide.md` (current content — 669 lines).

- [ ] **5b.** Add a new section (suggested: after the existing key management
  section) titled **"Private Repository & PAT Configuration"**. Cover:

  **PAT Creation Walkthrough:**
  1. Go to GitHub → Settings → Developer settings → Personal access tokens → Fine-grained tokens
  2. Click "Generate new token"
  3. Set token name (e.g., `ota_update_token`)
  4. Set expiration to "No expiration"
  5. Under "Repository access", select "Only select repositories" → choose your OTA repo
  6. Under "Repository permissions", set "Contents" to "Read-only"
  7. Leave all other permissions at "No access"
  8. Generate and copy the token

  **Repository Configuration:**
  - Single repo setup: set `config.otaGithubRepo = "owner/repo"` and provide PAT in `otaSecrets.hpp`
  - Split repo setup: set `config.otaGithubRepo = "owner/releases-repo"` (separate from source)
  - Public repo setup: set `config.otaGithubRepo = "owner/public-repo"` and omit the token (`config.otaGithubToken = nullptr`)

  **Deploy Script `--repo` Usage:**
  ```bash
  # Publish to a different repo than the source code repo
  python scripts/deploy_release.py --tag v1.0.0 --private-key private_key.pem \
      --local --repo owner/releases-repo
  ```

  **`otaSecrets.hpp` Setup:**
  - For private repos: create `include/otaSecrets.hpp` with the PAT
  - For public repos: omit the file entirely and set `config.otaGithubToken = nullptr`

## Verification Requirement

After completing all checklist items:
1. Confirm `ota-security.md` has the updated transport security section and NVS analysis
2. Confirm `tls-tradeoffs.md` has the updated `setInsecure()` section and fresh-client rule
3. Confirm `pat-management.md` exists with full content
4. Confirm `SKILL.md` has new trigger terms and table row
5. Confirm `OTADeveloperGuide.md` has the new PAT/repo section
6. **STOP and send a message back to the Principal Architect** reporting completion.
   Do NOT commit, do NOT distill, and do NOT make any further changes until explicitly approved.
