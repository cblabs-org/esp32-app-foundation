# ADR: Two-Stream Release Topology

## Decision

Separate binary releases and manifest releases into two independent streams:

| Stream | Tag Pattern | Contents |
|--------|-------------|----------|
| Binary releases | `v0.0.12-dev.2` (SemVer) | `firmware-*.bin`, `.sig`, `.sha256` |
| Manifest release | `manifest-latest` (floating) | `manifest.json`, `manifest.json.sig` |

## Why

1. The manifest is a **living index**, not a versioned artifact — overwriting it on each deploy is correct behaviour
2. Binary releases are immutable snapshots — mixing the manifest into them conflates two different lifecycles
3. GitHub auto-attaches source archives to every release — irrelevant noise for the manifest
4. Floating tag = fixed URL = plain HTTPS GET — no GitHub API, no search, no enumeration
5. The device's compiled-in URL (`manifest-latest`) works with any hosting backend behind a redirect

## Key Design Constraint

The device does **one plain HTTPS GET** to the fixed URL. It does NOT use the GitHub API. This means:
- The URL is hosting-agnostic (could be GitHub, S3, CDN)
- Migration is a DNS change or URL redirect — not a firmware update
- The `manifestUrl` field inside the manifest enables learned-URL migration

## Changes Made

- `OTA_MANIFEST_TAG` renamed from `"ota-manifest"` to `"manifest-latest"` in `deploy_release.py`
- Manifest assets removed from binary release upload (Step 5 asset patterns)
- Compiled-in URL in `main.cpp` updated to use `manifest-latest`
- Per-release manifest signing removed (was wasted — 4th passphrase prompt)
- All docs and skill references updated

## Rejected Alternatives

- **Monotonic manifest tags** (`manifest-1`, `manifest-2`): Requires a stable pointer mechanism anyway; adds complexity without clear benefit
- **Custom domain** (`ota.yourdomain.com`): Correct long-term but unnecessary right now — the architecture already supports it via absolute URLs and learned URL
- **Manifest in binary release**: Conflates two lifecycles; was the original design and explicitly rejected
