# Versioning Policy — SemVer Tag Governance

> [!CAUTION]
> **FLASH GATE — read this before every firmware upload.**
>
> Building for compilation testing is fine without tagging.
> But **before flashing to hardware**, if the firmware includes **any** new
> functionality since the last tag:
> 1. Check the current highest tag: `git tag --list 'v*' --sort=-version:refname | head -1`
> 2. **Parse the channel** of that tag (`dev`, `beta`, `rc`, or stable/none):
>    - **If `dev`:** Increment the dev PRE_NUM within the same PATCH:
>      `git tag v0.X.Y-dev.(N+1) && git push origin v0.X.Y-dev.(N+1)`
>      This is **autonomous** — `dev` tags do not require user approval.
>    - **If `beta`, `rc`, or stable (no suffix):** The current PATCH is
>      **closed**. A dev tag within this PATCH would be a version regression.
>      **STOP and ask the user** what the next version should be (e.g.
>      `v0.X.(Y+1)-dev.1`). Incrementing PATCH requires user approval.
> 3. Rebuild (so `inject_version.py` picks up the new tag at compile time)
> 4. **Then** flash.
>
> Channel precedence (low → high): `dev < beta < rc < stable`.
> Once a PATCH reaches a higher channel, all dev tags for that PATCH are
> superseded. New dev work MUST target the next PATCH series.
>
> Skip this step **only** if the firmware contains NO new functionality.
>
> **The sequence is: Tag → Build → Flash. Never flash an untagged build.**

### Pre-flash checklist

Before calling `upload_firmware`, `agent_flash_monitor_verify`, or any
esptool/OTA upload command, stop and verify:

- [ ] Does this firmware contain new functionality since the last `dev` tag?
- [ ] If yes: is the highest tag a `dev` tag, or has the PATCH graduated to `beta`/`stable`?
- [ ] If graduated: have I asked the user for the next version number?
- [ ] Have I tagged, pushed, **and rebuilt** so the version is baked in?
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

> [!NOTE]
> **Deploy is a separate decision from build+flash.** The user builds, flashes,
> and validates on hardware. If satisfied, the user then runs `deploy_release.py`
> to sign and publish. Agents should not assume that a successful build+flash
> means the user wants to deploy — wait for the user to explicitly request
> deployment.

## Mechanical Enforcement

This rule is **enforced by a `PreToolUse` lifecycle hook** (`.agents/hooks.json`
→ `flash-tag-gate`). The hook runs before every `upload_firmware` and
`agent_flash_monitor_verify` MCP call and will **hard-deny** the flash if HEAD
has untagged commits. The denial message includes the exact tag command to run.

The agent does not need to remember this checklist — the runtime will block it.
The rule text above remains as educational context explaining *why* tagging
matters and the conventions for tag formats.

## Full Policy Reference

For tag anatomy, three-tier release model, decision tree, and tag hygiene
rules see: [`docs/developer/versioning-policy.md`](../../docs/developer/versioning-policy.md)
