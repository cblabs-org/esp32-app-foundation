<!--
  ESP32 App Foundation (c) 2026 CB Labs
  Framework Skill Reference — Issue Management Process
  Loaded by: framework-expert skill agents
-->

# Issue Management Process

GitHub Issues on **`matthewmcneill/esp32-app-foundation`** are the canonical
system of record for all framework defects. This document tells you everything
you need to raise, track, link, and close a framework issue.

---

## When to raise an issue

Raise a GitHub issue whenever you find a defect in the framework that:

- Prevents a build or produces wrong behaviour (P1/P2), or
- Produces confusing output, misleading docs, or DX friction (P3).

Raise it against the **framework repo** even if you discovered it while working
in a consumer app. The framework repo is the right place for framework bugs.

---

## Label taxonomy

Apply labels at filing time. Every issue gets one severity label, plus any
applicable attribution/meta labels.

### Severity (choose one)

| Label | When to use |
|-------|-------------|
| `P1-blocker` | Prevents a clean build for any consumer |
| `P2-correctness` | Breaks a supported workflow or produces silently wrong behaviour |
| `P3-docs-dx` | Confusing docs, misleading comments, cosmetic build warnings |

### Attribution / meta (apply all that fit)

| Label | When to use |
|-------|-------------|
| `upstream-issue` | Issue was surfaced by a downstream consumer (not found internally) |
| `found-by:<your-program-name>` | Replace `<your-program-name>` with the slug of the consumer program you are working in (e.g. `found-by:power-monitor`, `found-by:energy-dashboard`). Create the label on the repo if it does not exist yet. |
| `backfill` | Historical issue filed after the fix was already merged; close immediately |

> **Important for agents:** The `found-by:` label must match the consumer program
> you are currently working in. Do not copy a label from an example or from a
> previous issue without checking it matches your context.

---

## How to file an issue

### Identity rule

File under the **repo owner's GitHub identity**. Before filing, confirm your
`gh` session is authenticated as the repo owner:

```bash
gh auth status
# Confirm: Active account: true, and the account is the framework repo owner.
# Do NOT proceed if you are logged in as a different user.
```

Never file under a third-party identity.

### Issue title format

```
[Module/Area] One-line description of the defect
```

Examples:
- `[U8G2Display] Explicit SPI pins silently ignored when DISPLAY_SPI_MISO undefined`
- `[portalBuilder] Paths resolved from cwd, not framework root — fails out-of-tree`

### Issue body template

```markdown
## Symptom
<!-- What goes wrong. What the user sees or doesn't see. -->

## Evidence
<!-- File paths, line numbers, compiler output, build logs. -->
<!-- Format: `modules/Foo/Foo.cpp:87` -->

## Found against commit
<!-- The framework commit SHA where the bug was confirmed. -->
`<SHA>`

## Suggested fix
<!-- Optional but helpful. Concrete enough to act on. -->

## Consumer context
<!-- Which consumer program found this, if applicable. -->
Found by: <your-program-name>, <PR or session context>
```

### Filing command

```bash
gh issue create \
  --repo matthewmcneill/esp32-app-foundation \
  --title "[Module] Description" \
  --body-file issue-body.md \
  --label "P2-correctness,upstream-issue,found-by:<your-program-name>"
```

Replace `found-by:<your-program-name>` with the label for your consumer program.
If the label does not exist on the repo yet, create it first:

```bash
gh label create "found-by:<your-program-name>" \
  --repo matthewmcneill/esp32-app-foundation \
  --color "a855f7" \
  --description "Reported by the <Your Program> consumer"
```

---

## How fixes close issues

The fixing PR's description must include:

```
Fixes #N
```

GitHub auto-closes the issue when the PR merges to `main`. No manual closing
is needed for issues resolved this way.

---

## Consumer-pointer convention

Once a framework issue is filed, the consumer-side audit entry is **replaced**
with a thin pointer. Never maintain a full copy of the bug description on the
consumer side — the framework issue is the source of truth.

### Format (open issue)

```markdown
## [Area] One-line title [OPEN]
- **Framework issue:** [#N](https://github.com/matthewmcneill/esp32-app-foundation/issues/N)
- **Workaround:** <describe any local workaround in use, or "none">
```

### Format (resolved issue)

```markdown
## [Area] One-line title [CLOSED]
- **Framework issue:** [#N](https://github.com/matthewmcneill/esp32-app-foundation/issues/N)
- **Resolved:** framework commit `<SHA>`, merged in PR #M
```

---

## Backfill convention

When a defect was fixed before a GitHub issue was created (e.g. discovered
retrospectively from a PR or audit):

1. File the issue using the standard body template.
2. Add the `backfill` label alongside the severity and attribution labels.
3. In the body, include: `**Resolved in:** commit \`<SHA>\`` (or PR #M).
4. **Immediately close** the issue with reason "completed".

This preserves history and lets consumers link their audit pointers to a real
issue number without leaving a ghost open issue on the board.

```bash
# Create a backfill issue and immediately close it
NUM=$(gh issue create \
  --repo matthewmcneill/esp32-app-foundation \
  --title "[Module] Description" \
  --body-file body.md \
  --label "P1-blocker,upstream-issue,found-by:<your-program-name>,backfill" | grep -oE '[0-9]+$')

gh issue close "$NUM" \
  --repo matthewmcneill/esp32-app-foundation \
  --reason completed \
  --comment "Resolved in commit \`<SHA>\` (merged in PR #M, <date>)."
```
