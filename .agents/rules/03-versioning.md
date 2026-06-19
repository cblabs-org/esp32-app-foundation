# Versioning Policy — SemVer Tag Governance

> [!CAUTION]
> **BUILD GATE — read this before every build.**
>
> If you are about to build firmware that includes **any** new functionality:
> 1. Check the current highest tag: `git describe --tags --abbrev=0`
> 2. Increment the dev PRE_NUM: `git tag v0.X.Y-dev.(N+1) && git push origin v0.X.Y-dev.(N+1)`
> 3. **Then** build. `inject_version.py` reads the tag at compile time — if you
>    build first and tag after, the firmware ships with the wrong version.
>
> This is **autonomous** — `dev` tags do not require user approval.
> Skip this step **only** if the build contains NO new functionality.
> **Never build first and tag after.**

---

## Quick Reference

| Tag format              | Channel  | Agent can tag? | Requires user? |
|-------------------------|----------|---------------|----------------|
| `v0.X.Y-dev.N`         | Dev      | ✅ Yes         | No             |
| `v0.X.Y-beta.N`        | Beta     | ❌ No          | Yes            |
| `v0.X.Y`               | Stable   | ❌ No          | Yes            |

| Action                        | Agent autonomy | Requires user approval |
|-------------------------------|---------------|------------------------|
| Create `dev` tag              | ✅ Yes         | No                     |
| Create `beta` / `stable` tag  | ❌ No          | Yes                    |
| Increment `PATCH`/`MINOR`/`MAJOR` | ❌ No     | Yes                    |
| Run `deploy_release.py` (dev) | ✅ Yes         | No                     |
| Run `deploy_release.py` (β/stable) | ❌ No    | Yes                    |

## Full Policy Reference

For tag anatomy, three-tier release model, decision tree, and tag hygiene
rules see: [`docs/developer/versioning-policy.md`](../../docs/developer/versioning-policy.md)
