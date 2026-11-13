# Versioning Policy — SemVer Tag Governance

This document is the full reference for the project's versioning strategy.
For the operational build-gate rule enforced on every agent, see
`.agents/rules/03-versioning.md`.

---

## Tag Anatomy

All version tags follow strict SemVer with an optional channel suffix:

```
v<MAJOR>.<MINOR>.<PATCH>[-<CHANNEL>.<PRE_NUM>]
```

| Segment     | Meaning                                                                 |
|-------------|-------------------------------------------------------------------------|
| `MAJOR`     | Incompatible API or architectural break — human decision only           |
| `MINOR`     | Meaningful new capability shipped (not a per-build counter)             |
| `PATCH`     | Each merged PR that lands new functionality increments this             |
| `CHANNEL`   | `dev`, `beta`, `rc`, or omitted (stable)                                |
| `PRE_NUM`   | Pre-release iteration within a given PATCH (e.g. `beta.1`, `beta.2`)   |

**Examples:**

| Tag                 | Meaning                                                          |
|---------------------|------------------------------------------------------------------|
| `v0.1.3-dev.4`      | 4th in-flight dev build toward patch 3                           |
| `v0.1.3-beta.1`     | First beta release of patch 3, cut after PR closure              |
| `v0.1.3-beta.2`     | Follow-up beta if a fix was required post-PR                     |
| `v0.1.3`            | Stable release of patch 3 — manually promoted                   |
| `v0.2.0`            | Minor bump — new capability boundary, human decision             |

**Channel precedence** (low → high): `dev < beta < rc < stable`. `v0.1.3-beta.1`
is *higher* than `v0.1.3-dev.9`. Plain `git tag --sort=-version:refname` sorts
`dev` above `beta` alphabetically, so never use it to find "the latest tag".

---

## Choosing the Next Tag — `scripts/next_version.py`

Always get the next tag from the script rather than working it out by hand:

```bash
python3 scripts/next_version.py --current                # highest tag per channel + each channel's next tag
python3 scripts/next_version.py --fetch --channel dev    # next dev tag
python3 scripts/next_version.py --channel beta           # next beta (print only)
python3 scripts/next_version.py --channel beta --bump major   # e.g. v1.0.0-beta.1 (print only)
python3 scripts/next_version.py --channel dev --tag --push    # create + push a dev tag
```

What it guarantees:

- A new tag always sorts **above every existing tag of the same or a higher
  channel** — it never produces a version regression.
- `dev` continues the newest dev series, or opens the next PATCH once that
  core already has a `beta`, `rc` or stable tag.
- `beta`, `rc` and stable promote the core of the newest beta-or-higher tag,
  moving to the next PATCH once that core has reached a higher channel.
- `--bump patch|minor|major` and `--target X.Y.Z` pick the core explicitly
  (human decisions).
- `--tag` only creates **dev** tags, and only within the current dev series;
  a dev tag that opens a new PATCH needs `--new-series`, which agents pass
  only after the user approves. Beta, rc, stable, MINOR and MAJOR are always
  tagged by the user.

`scripts/inject_version.py` runs inside the PlatformIO build and keeps its own
copy of the tag parser, so `scripts/tests/test_next_version.py` checks that the
two agree on the tag pattern and channel ranks. Run the tests with
`python3 -m pytest scripts/tests`.

---

## Single Source of Truth

`scripts/inject_version.py` is the **sole owner** of version injection into the
firmware build. This means:

- **No** version literals may appear in `platformio.ini` build flags (except the
  static `native` env fallback which cannot read git at test time).
- **No** hardcoded version strings in source files.
- Version is always derived from git tags at build time.

Agents MUST NOT add `-DBUILD_VERSION` or `-DVERSION_*` flags anywhere other than
`inject_version.py`.

---

## Three-Tier Release Model

### Tier 1 — Dev (`-dev.N`)

**Purpose:** In-flight builds during active development, before a PR is merged.

**When to increment:** Increment `PRE_NUM` each time a build is done that
introduces new functionality or a significant change **within** the current
working branch / PR cycle.

**Who can tag:** Agents **MAY** create and push `dev` tags autonomously when
executing a build that includes new functionality, without requiring explicit
user approval. This is the only tier where autonomous tagging is permitted.

**How to tag:**
```bash
# First dev build toward patch 4
git tag v0.1.4-dev.1 && git push origin v0.1.4-dev.1

# Subsequent in-flight build
git tag v0.1.4-dev.2 && git push origin v0.1.4-dev.2
```

**Rules:**
- `PATCH` is the next patch above the highest tag of **any** channel —
  `next_version.py --channel dev` works it out. Opening a new PATCH series
  needs user approval; continuing the current dev series does not.
- `PRE_NUM` starts at 1 and increments with each functionally-distinct build.
- Dev tags can be published as GitHub pre-releases via `deploy_release.py` so
  that devices on the `dev` OTA channel receive in-flight builds. The **user**
  runs `deploy_release.py` for every channel: the signing key needs an
  interactive passphrase, so agents output the command instead of running it.

---

### Tier 2 — Beta (`-beta.N`)

**Purpose:** Candidate builds cut after a significant PR is closed and merged
into the main branch.

**When to tag:** After every major PR closure. Increment `PATCH` for each new
PR. Increment `PRE_NUM` within the same `PATCH` if a fix is required
post-merge but before stable promotion.

**Who can tag:** **HUMAN ONLY.** Agents MUST NOT create beta tags without
explicit user instruction. The user alone decides when a merged PR warrants a
beta cut.

**How to tag:**
```bash
# First beta after PR-N merges (new patch)
git tag v0.1.4-beta.1 && git push origin v0.1.4-beta.1

# Fix required — same patch, next beta
git tag v0.1.4-beta.2 && git push origin v0.1.4-beta.2
```

**Rules:**
- Beta tags replace all `dev` tags for that patch — once a beta is cut,
  subsequent in-flight work targets the **next** patch's `dev` series.
- Beta builds are published as GitHub pre-releases via `deploy_release.py`
  **after hardware validation**. Tagging a beta does not automatically deploy
  it — build, flash, and validate on hardware first, then deploy when satisfied.
- `scripts/deploy_release.py` will mark these as `--prerelease` automatically.

---

### Tier 3 — Stable (no suffix)

**Purpose:** Production-ready releases, manually promoted from beta after
sufficient validation.

**When to tag:** Human decision only. There is no automatic promotion from beta
to stable under any circumstances.

**Who can tag:** **HUMAN ONLY.** Agents MUST NOT create stable tags, ever.

**How to tag and release:**
```bash
# Phase 1: Tag + Build + Validate
git tag v0.1.4 && git push origin v0.1.4
pio run -e esp32dev -t clean && pio run -e esp32dev
pio run -e esp32dev --target upload
# → Test on hardware: verify /api/system version, portal, basic operation
# → Repeat for all board targets

# Phase 2: Deploy (when validated)
python3 scripts/deploy_release.py --local --private-key private_key.pem
```

**Rules:**
- Stable tags must point at HEAD at build time (`deploy_release.py` verifies this).
- Build, flash, and validate on hardware **before** deploying.
- Stable GitHub Releases are NOT marked as pre-release.
- A new stable tag supersedes all beta and dev tags for that patch.

---

## Long-Running Public Beta (1.0.0)

The first public release opens a beta series that stays open until outside
feedback says 1.0.0 is mature — potentially a long time. During that period:

- **Betas stay on `v1.0.0-beta.N`.** Each beta-worthy merge gets the next
  `beta.N` (`next_version.py --channel beta`), not a new PATCH.
- **Dev builds run ahead on `v1.0.1-dev.N`**, because a dev tag must sort
  above the open beta. Bench devices on the `dev` channel therefore never
  "upgrade" back down to a beta.
- **`v1.0.0` is promoted from a chosen beta** when the maintainer decides it
  is ready (`next_version.py --channel stable` → `v1.0.0`).
- After promotion the normal per-PR PATCH rule resumes: the next beta is
  `v1.0.1-beta.1`, and dev moves on to `v1.0.2-dev.N` once that beta exists.

---

## Minor and Major Bumps

| Bump    | Trigger                                                                | Who decides |
|---------|------------------------------------------------------------------------|-------------|
| `PATCH` | Each merged PR with new functionality                                  | User        |
| `MINOR` | A meaningful new capability boundary or significant module addition    | User        |
| `MAJOR` | Incompatible architectural break, protocol change, or API overhaul     | User        |

Agents MUST NOT increment `MINOR` or `MAJOR` without explicit user instruction,
even when the scope of work seems to warrant it. Propose a bump in the
implementation plan and let the user confirm.

---

## Tag Hygiene Rules

1. **Tag before build** — when deploying a release, `deploy_release.py`
   auto-discovers the SemVer tag on HEAD via `discover_tag_on_head()`. The
   tag must exist before the build step so `inject_version.py` picks up the
   correct version.

2. **One canonical version per build** — if multiple SemVer tags exist on
   HEAD, `inject_version.py` selects the highest. Agents should avoid creating
   ambiguous double-tags on the same commit.

3. **Retag after housekeeping commits** — if non-firmware commits (agent
   workspace files, distillery artefacts, doc fixes) land after a release tag,
   `inject_version.py` will emit a WARNING. Before the next release, the tag
   must be moved to HEAD:
   ```bash
   git tag <tag> -f && git push origin --force <tag>
   ```

4. **Don't use non-SemVer branch tags for version tracking** — tags like
   `pr-18/wo3-display-tab` are branch markers, not version tags, and are
   ignored by `inject_version.py`. Do not create these with a `v` prefix.

---

## Decision Tree for Agents

```
Building firmware with new functionality →
  Run: python3 scripts/next_version.py --fetch --channel dev
  Does the suggested tag continue the current dev series (same PATCH)?
  ├─ YES: next_version.py --channel dev --tag --push (autonomous ✅)
  └─ NO (opens a new PATCH): STOP, ask the user to approve the tag,
         then re-run with --new-series

Building firmware with NO new functionality →
  → No new tag. inject_version.py will use the nearest ancestor tag.

PR merged →
  → STOP. Inform the user. Do NOT create a beta tag.
  → User will decide whether to cut a beta and at what patch number.

User says "cut a beta" →
  → Show the user next_version.py --channel beta (plus --bump/--target if
    they asked for a new PATCH/MINOR/MAJOR) and confirm the tag.
  → The user tags and pushes (beta tags are human-only).
  → Build, flash to hardware, and validate (version, portal, basic operation).
  → When satisfied, give the user the deploy_release.py command to run.

User says "promote to stable" →
  → Show the user next_version.py --channel stable and confirm the tag.
  → The user tags and pushes.
  → Build, flash to hardware, and validate.
  → When satisfied, give the user the deploy_release.py command to run.
  → This is a human-gated action — do not initiate unless explicitly instructed.
```

---

## Summary Table

| Action                        | Agent autonomy | Requires user approval |
|-------------------------------|---------------|------------------------|
| Create `dev` tag              | ✅ Yes         | No                     |
| Create `beta` tag             | ❌ No          | Yes                    |
| Create `rc` tag               | ❌ No          | Yes                    |
| Create `stable` tag           | ❌ No          | Yes                    |
| Continue a `dev` series        | ✅ Yes         | No                     |
| Open a new `PATCH` (incl. dev) | ❌ No          | Yes                    |
| Increment `MINOR` or `MAJOR`  | ❌ No          | Yes                    |
| Run `deploy_release.py` (any) | ❌ No          | Yes (user runs the CLI) |
| Move/force-push a tag         | ❌ No          | Yes (state in plan)    |
