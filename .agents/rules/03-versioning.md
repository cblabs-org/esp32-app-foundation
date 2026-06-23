# Versioning Policy — SemVer Tag Governance

> [!CAUTION]
> **FLASH GATE — read this before every firmware upload.**
>
> Building for compilation testing is fine without tagging.
> But **before flashing to hardware**, if the firmware includes **any** new
> functionality since the last tag:
> 1. Check the current highest tag: `git describe --tags --abbrev=0`
> 2. Increment the dev PRE_NUM: `git tag v0.X.Y-dev.(N+1) && git push origin v0.X.Y-dev.(N+1)`
> 3. Rebuild (so `inject_version.py` picks up the new tag at compile time)
> 4. **Then** flash.
>
> This is **autonomous** — `dev` tags do not require user approval.
> Skip this step **only** if the firmware contains NO new functionality.
>
> **The sequence is: Tag → Build → Flash. Never flash an untagged build.**

### Pre-flash checklist

Before calling `upload_firmware`, `agent_flash_monitor_verify`, or any
esptool/OTA upload command, stop and verify:

- [ ] Does this firmware contain new functionality since the last `dev` tag?
- [ ] If yes: have I tagged, pushed, **and rebuilt** so the version is baked in?
- [ ] `curl /api/system` after boot — does `firmware_version` match the new tag?

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
| Run `deploy_release.py` (any) | ❌ No*          | Yes (User runs CLI)    |

* **Agents MUST NEVER execute `deploy_release.py` directly.** The private key requires an interactive passphrase, which causes background agent tasks to hang indefinitely on `openssl`. Always output the exact CLI command and ask the user to run it in their terminal.

## Full Policy Reference

For tag anatomy, three-tier release model, decision tree, and tag hygiene
rules see: [`docs/developer/versioning-policy.md`](../../docs/developer/versioning-policy.md)
