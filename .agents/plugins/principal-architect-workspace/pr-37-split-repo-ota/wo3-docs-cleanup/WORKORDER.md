---
name: pr37-wo3
description: Handoff instructions for Work Order 3 — Documentation cleanup (de-GitHub-ify OTADownloadEngine, fix manifest field names, update OTA docs + skill docs)
complexity: mechanical
---

# WO3: Documentation Cleanup — De-GitHub-ify + Fix Drift

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Update all OTA-related documentation and code comments to reflect the split-repo, manifest-first architecture. Remove GitHub-specific language from the OTADownloadEngine, fix known documentation drift (wrong field names, wrong task spec), and update the PAT management guide.

## Strict Boundary Box

- **DO NOT** modify any functional C++ code — only comments and documentation strings
- **DO NOT** modify `scripts/deploy_release.py` or `scripts/generate_manifest.py` — that was WO2
- **DO NOT** modify `src/main.cpp` or `lib/OTACerts/` — that was WO1
- **DO NOT** create branches or make architectural decisions

## Tooling Constraints

- Use `platformio` MCP tools for build verification if needed
- This WO is documentation-only — build verification is optional but recommended

## Checklist

### 1. De-GitHub-ify `lib/OTADownloadEngine/OTADownloadEngine.hpp`

- [ ] Line 10 description: Change `"GitHub-aware OTA firmware download engine"` → `"OTA firmware download engine"`
- [ ] Line 24: Change `"Download engine class for GitHub firmware OTA"` → `"Download engine class for OTA firmware updates"`
- [ ] Line 28: Change `"Download .sig file following GitHub redirects"` → `"Download .sig file following HTTP redirects"`
- [ ] Line 79: Change `"GitHub-aware OTA firmware download engine"` → `"OTA firmware download engine"`
- [ ] Line 81: Change `"Handles redirect-following HTTP fetches from GitHub release assets"` → `"Handles redirect-following HTTP fetches from release asset URLs"`
- [ ] Line 132: Change `"the detached .sig file from a GitHub release asset URL"` → `"the detached .sig file from a release asset URL"`
- [ ] Line 135: Change `"URL of the .sig file (GitHub API asset URL)"` → `"URL of the .sig file (HTTPS asset URL)"`
- [ ] Line 136: Change `"Optional GitHub access token for private repos"` → `"Optional access token for private release repos"`
- [ ] Line 153: Change `"URL of the .bin file (GitHub API asset URL)"` → `"URL of the .bin file (HTTPS asset URL)"`
- [ ] Line 154: Change `"Optional GitHub access token for private repos"` → `"Optional access token for private release repos"`
- [ ] Line 195: Change `"Optional GitHub access token"` → `"Optional access token"`

### 2. De-GitHub-ify `lib/OTADownloadEngine/OTADownloadEngine.cpp`

- [ ] Line 10-11 description: Change `"GitHub binary downloader"` references → `"OTA binary downloader"` or similar
- [ ] Line 137: Change `"GitHub-appropriate headers (User-Agent, Cache-Control, Accept, Authorization, X-GitHub-Api-Version)"` → `"download headers (User-Agent, Cache-Control, Accept, Authorization)"`
- [ ] **Remove** line 170: `http.addHeader("X-GitHub-Api-Version", "2022-11-28");` — this is the only functional change in this WO, but it's removing a no-op header for a non-GitHub endpoint

### 3. Fix `modules/OTAUpdateManager/OTAUpdateManager.hpp` comment

- [ ] Find the exported functions comment that says `getSchemaVersion(): Return 4` and change to `getSchemaVersion(): Return 5` (the .cpp returns 5 at line 109)

### 4. Fix manifest field names in `docs/developer/ota-update-system.md`

- [ ] Lines 423-425, 437-438: Change `"firmware"` → `"firmwareUrl"` and `"signature"` → `"signatureUrl"` in the manifest schema JSON example
- [ ] Line 112: Change `"8KB stack, priority 1"` → `"16KB stack (OTA_TASK_STACK_BYTES = 16384), priority 5"` to match actual code

### 5. Fix manifest field names in `.agents/skills/framework-expert/systems/ota/deployment.md`

- [ ] Lines 131-132: Change `"firmware"` → `"firmwareUrl"` and `"signature"` → `"signatureUrl"` in the manifest schema JSON example

### 6. Update `.agents/skills/framework-expert/security/pat-management.md`

This file is substantially stale after the manifest-first + split-repo changes. Update it:

- [ ] **Configuration examples (lines 60-68, 84-88, 108-117):** Replace `config.otaGithubRepo` → `config.otaManifestUrl` and `config.otaGithubToken` → `config.otaToken` in all three Option sections. Update the URL format to `raw.githubusercontent.com` for the manifest.

- [ ] **Option A (Single Private Repo, lines 52-68):** Note that this option now requires the repo to be **public** for the `/releases/download/` URLs to work without API auth. If the repo must stay private, the user needs the API URL approach (or a separate releases repo).

- [ ] **Option B (Separate Public Releases Repo, lines 76-98):** This is now the **recommended** default. Update the code example:
  ```cpp
  config.otaManifestUrl = "https://raw.githubusercontent.com/owner/my-firmware-releases/main/manifest.json";
  config.otaToken = nullptr;  // no token needed for public repos
  ```

- [ ] **Option C (Separate Private Releases Repo, lines 100-123):** Update the code example:
  ```cpp
  config.otaManifestUrl = "https://raw.githubusercontent.com/owner/my-firmware-releases/main/manifest.json";
  config.otaToken = OTA_GITHUB_TOKEN;  // PAT scoped to releases repo only
  ```
  Note: For private repos, `raw.githubusercontent.com` does accept token auth (unlike `/releases/download/`).

- [ ] **Design Decision D7 (line 151-157):** Update reference from `FrameworkConfig.otaGithubRepo` → `FrameworkConfig.otaManifestUrl`. The rationale still applies — the manifest URL is configurable so downstream apps can each point to their own releases repo.

### 7. Verify (optional for documentation WO)

- [ ] Run native tests via MCP: `run_tests` (environment: `native`) — confirm no regressions from the one functional change (X-GitHub-Api-Version header removal)
- [ ] Run ESP32 build via MCP: `build_project` (environment: `esp32dev`) — confirm clean compile

## Verification Requirement

After completing all checklist items, **STOP and send a message back to the Principal Architect** reporting:
- Summary of all files modified with line counts
- Whether builds/tests pass (if run)
- Any issues or ambiguities encountered

Do NOT distill or wrap up until the Principal Architect explicitly approves.
