# State: Files Changed in WO6

## Commit `e83a122` — 3 files, +273 −124

### 1. `docs/developer/ota-update-system.md`

| Section | Change Type | Summary |
|---|---|---|
| §1 Quick-Start | Field rename | `otaGithubRepo` → `otaManifestUrl`, `otaGithubToken` → `otaToken` |
| §2 Component Table | Row removed + edit | Removed `GithubClient` row, updated `OTAManifest` description, fixed module path casing |
| §3.1 State Machine | Description update | "Checking" → "Fetching and verifying the signed manifest" |
| §3.2 Pipeline Steps | Full rewrite | 12-step manifest-first pipeline with 3 defence-in-depth gates, request count table |
| §4 heading | Renamed | "Release Channels" → "Release Channels & Manifest-First Discovery" |
| §4.1 | Full rewrite | New "Release Discovery (Manifest-First)" with flow diagram |
| §4.1.1 | New section | "Manifest Signing" — threat model, verification rule |
| §4.1.2 | New section | "Manifest URL Resilience" — compiled-in vs learned |
| §4.1.3 | New section | "Defence-in-Depth" — 3-gate description |
| §4.2 | Config update | JSON schema 2→5, added `updatesEnabled`, `checkHour`, `learnedManifestUrl` |
| §7.2 | Table rewrite | `otaGithubRepo` → `otaManifestUrl` with full URLs, PAT via `otaToken` |
| §8.1 | Full rewrite | v1 per-release → v2 multi-channel schema with absolute URLs, key properties |
| §8.3 | Removed | Legacy fallback section (no longer applicable) |
| §11.2 | Deploy steps | Added manifest signing + `ota-manifest` release steps |
| §15 | Field table | Updated FrameworkConfig fields + IConfigurable schema v5 + `learnedManifestUrl` |
| §17 | Security checklist | Added `otaManifestUrl`, `manifest.json.sig`, defence-in-depth checks |

### 2. `.agents/skills/framework-expert/systems/ota/deployment.md`

Complete rewrite (127 → ~170 lines). Key additions:
- `ota-manifest` fixed release approach (replaces floating tags)
- Manifest signing section with threat model
- Defence-in-depth gates
- `--legacy-floating-tags` flag documentation
- v2 manifest schema with absolute URLs
- Manifest URL resilience section

### 3. `.agents/skills/framework-expert/SKILL.md`

Trigger keyword changes:
- **Removed:** `"floating tag"`, `"latest-dev"`, `"latest-stable"`, `"O(1) polling"`
- **Added:** `"manifest signing"`, `"manifest-first"`, `"otaManifestUrl"`, `"ota-manifest release"`, `"manifest.json.sig"`, `"signed manifest"`, `"manifest URL resilience"`, `"learned URL"`, `"compiled-in URL"`, `"defence-in-depth"`, `"manifest schema v2"`, `"multi-channel manifest"`
