# Reporting Framework Issues

> ESP32 App Foundation (c) 2026 CB Labs

**GitHub Issues on this repo are the system of record for framework defects.**
If you find a bug — whether building in-tree or consuming the framework
out-of-tree — raise it here.

---

## Severity labels

| Label | Use when |
|-------|----------|
| `P1-blocker` | Prevents a clean build for any consumer |
| `P2-correctness` | Breaks a supported workflow or produces wrong behaviour |
| `P3-docs-dx` | Confusing docs, misleading comments, cosmetic build warnings |

## Attribution labels

| Label | Use when |
|-------|----------|
| `upstream-issue` | Found by a downstream consumer (not internally) |
| `found-by:<program>` | Name of the consumer program that found it |
| `backfill` | Historical — filed after the fix was already merged |

---

## Filing an issue

### Check your identity

```bash
gh auth status   # must show: account matthewmcneill, Active account: true
```

File under the **repo owner's identity only**. Never under a third-party account.

### Title format

```
[Module/Area] One-line description
```

### Body template

```markdown
## Symptom
What goes wrong. What the user sees or doesn't see.

## Evidence
File:line references, compiler output, build logs.

## Found against commit
`<SHA>`

## Suggested fix
Optional, but concrete enough to act on.

## Consumer context
Found by: <program>, <PR or session>
```

### Quick-file example

```bash
gh issue create \
  --repo matthewmcneill/esp32-app-foundation \
  --title "[U8G2Display] Explicit SPI pins silently ignored when MISO undefined" \
  --body-file issue-body.md \
  --label "P2-correctness,upstream-issue,found-by:power-monitor"
```

---

## Closing issues via PRs

Add `Fixes #N` anywhere in the PR description. GitHub auto-closes the issue
when the PR merges to `main`.

---

## Backfilling historical issues

If a bug was fixed before a GitHub issue was filed:

1. File the issue normally (body includes `**Resolved in:** commit \`<SHA>\``).
2. Add the `backfill` label.
3. Immediately close with reason **completed**.

```bash
NUM=$(gh issue create --title "..." --body "..." \
  --label "P1-blocker,upstream-issue,found-by:power-monitor,backfill" \
  --json number --jq '.number')

gh issue close "$NUM" --reason completed \
  --comment "Resolved in commit \`<SHA>\`, merged 2026-07-08."
```

---

## Consumer-side pointer convention

Once a framework issue is filed, slim any consumer-side audit entry to:

```markdown
## [Area] Title [OPEN / CLOSED]
- **Framework issue:** [#N](https://github.com/matthewmcneill/esp32-app-foundation/issues/N)
- **Status:** Resolved in `<SHA>` — OR — Open, workaround: <text>
```

Keep full bug descriptions on the framework issue, not in consumer repos.
