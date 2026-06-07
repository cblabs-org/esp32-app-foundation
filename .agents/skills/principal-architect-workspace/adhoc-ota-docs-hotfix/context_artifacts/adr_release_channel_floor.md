# ADR: Release Channel Model — Risk-Floor Semantics

## Context

The original channel implementation used exact-match filtering: a device on
`beta` only saw `beta` tags, never `stable`. This created a situation where
a `stable` release that was numerically higher than the latest beta would be
invisible to beta-channel devices.

## Decision — Risk-Floor Model

Channel subscription is now a **minimum stability floor**, not an exact track.
The acceptance rule for `getLatestReleaseForChannel()` is:

```
stable subscriber  → stable only          (getLatestRelease() fast-path)
rc     subscriber  → rc + stable
beta   subscriber  → beta + rc + stable
dev    subscriber  → dev + beta + rc + stable
```

Implementation: a single pass through the `/releases` list. Each release is
parsed via `parseVersionTag()`. Its `Channel` enum value is compared against
the subscriber's floor. The highest-SemVer accepted release wins.

**Channel enum (contiguous 2-bit, matches SemVer.hpp):**
```
DEV    = 0
BETA   = 1
RC     = 2
STABLE = 3
```

Note: the older non-contiguous mapping (DEV=0, BETA=2, RC=3, STABLE=15) has
been retired. `inject_version.py` and all downstream comparisons now use the
contiguous 2-bit scheme.

## Why Not Separate Per-Track Manifests?

Considered: a `manifest-stable.json`, `manifest-beta.json` model. Rejected
because it requires deploying two manifests per release and creates an
administrative burden. The risk-floor model is self-consistent and requires
no extra release artifacts.

## Impact on `GithubClient`

`getLatestReleaseForChannel()` was rewritten from:
- Early-return on first matching prerelease tag containing the channel string

To:
- Full scan, best-candidate tracking, commit winner to member fields at end
- Old approach would miss a stable release that superseded a beta

## Stash Note

The user's GithubClient changes (risk-floor scan, `SemVer.hpp` include)
arrived via an IDE stash. They are now fully merged into `main` as of
`9f07b65` and `77318d0`.
