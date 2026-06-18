# Versioning Policy — SemVer Tag Governance

This rule applies to all agents, all modes, and all sessions. It governs how
version tags are created, incremented, and applied in this repository.

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
- `PATCH` should be the *next* unreleased patch number (i.e. current stable patch + 1).
- `PRE_NUM` starts at 1 and increments with each functionally-distinct build.
- Dev tags ARE published as GitHub pre-releases via `deploy_release.py` so that
  devices on the `dev` OTA channel can receive in-flight builds.
- Agents MAY run `deploy_release.py` for `dev` tags autonomously (same autonomy
  as tag creation). Beta/stable deploys still require user approval.

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
- Beta builds ARE published as GitHub pre-releases via `deploy_release.py`.
- `scripts/deploy_release.py` will mark these as `--prerelease` automatically.

---

### Tier 3 — Stable (no suffix)

**Purpose:** Production-ready releases, manually promoted from beta after
sufficient validation.

**When to tag:** Human decision only. There is no automatic promotion from beta
to stable under any circumstances.

**Who can tag:** **HUMAN ONLY.** Agents MUST NOT create stable tags, ever.

**How to tag:**
```bash
git tag v0.1.4 && git push origin v0.1.4
python3 scripts/deploy_release.py --local --build --private-key private_key.pem
```

**Rules:**
- Stable tags must point at HEAD at build time (`deploy_release.py` verifies this).
- Stable GitHub Releases are NOT marked as pre-release.
- A new stable tag supersedes all beta and dev tags for that patch.

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
  Does a dev tag already exist for the current target patch?
  ├─ YES: Increment PRE_NUM → CREATE v0.X.Y-dev.N+1 (autonomous ✅)
  └─ NO:  Start at PRE_NUM=1 → CREATE v0.X.Y-dev.1 (autonomous ✅)
      where X.Y = next unreleased patch (last stable/beta patch + 1)

Building firmware with NO new functionality →
  → No new tag. inject_version.py will use the nearest ancestor tag.

PR merged →
  → STOP. Inform the user. Do NOT create a beta tag.
  → User will decide whether to cut a beta and at what patch number.

User says "cut a beta" →
  → Confirm PATCH number (increment from last beta/stable patch).
  → Confirm PRE_NUM (1 unless a beta already exists for this patch).
  → Tag and push, then run deploy_release.py (it auto-discovers the tag).

User says "promote to stable" →
  → Confirm the tag matches the last beta for that patch (drop the suffix).
  → Run deploy_release.py (it auto-discovers the stable tag from HEAD).
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
| Increment `PATCH`             | ❌ No          | Yes                    |
| Increment `MINOR` or `MAJOR`  | ❌ No          | Yes                    |
| Run `deploy_release.py` (dev) | ✅ Yes         | No                     |
| Run `deploy_release.py` (β/stable) | ❌ No    | Yes                    |
| Move/force-push a tag         | ❌ No          | Yes (state in plan)    |
