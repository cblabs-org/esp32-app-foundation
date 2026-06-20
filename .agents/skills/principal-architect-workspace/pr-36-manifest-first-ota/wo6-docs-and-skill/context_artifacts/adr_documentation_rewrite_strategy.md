# ADR: Documentation Rewrite Strategy for Manifest-First OTA

## Context

PR-36 (WO1–WO5) replaced the GithubClient-based OTA discovery with a manifest-first architecture. WO6 needed to update all documentation and the framework-expert agent skill to match.

## Decisions

### 1. Full section rewrite vs incremental patches

**Decision:** Full rewrite of §3.2 (Pipeline Steps), §4 (Release Channels), and §8.1 (Manifest Schema). Incremental patches for §1, §7, §11, §15, §17.

**Why:** The old §4.1/§4.1.1 sections were deeply intertwined with the floating-tag/GithubClient architecture. Patching individual sentences would leave inconsistent mental models. A clean rewrite ensures the narrative flows naturally for readers unfamiliar with the old architecture.

### 2. §7 Repository Configuration table updated to use full manifest URLs

**Decision:** Replaced `config.otaGithubRepo` = `"owner/repo"` with full `config.otaManifestUrl` URLs.

**Why:** The old field took a compact `owner/repo` string; the new field takes a full URL. This changes the table formatting significantly but accurately reflects the developer experience.

### 3. deployment.md: complete overwrite vs surgical edit

**Decision:** Complete overwrite of the file.

**Why:** The file was 127 lines and almost every section needed changes. The floating tags section, manifest schema, and deploy workflow descriptions were all structurally different. A clean rewrite was more reliable than surgical multi-replace across the entire file.

### 4. Error table left unchanged

**Decision:** No new error codes were added in WO1–WO5. The `OTAError` enum still ends at `BinaryVersionMismatch = -16`. Error tables in both docs and deployment.md were left as-is.

### 5. SKILL.md trigger keywords

**Decision:** Removed `"floating tag"`, `"latest-dev"`, `"latest-stable"`, `"O(1) polling"`. Added 12 new manifest-first triggers. Did not add `"floating tag"` as a trigger even for backward compat — the old terms should fade from use.
