## What and why

<!-- What this changes and the problem it solves. Link the issue: "Resolves #123". -->

## How it was tested

<!-- Builds, native tests, web tests, and hardware (board + env) where it matters. -->

## Checklist

- [ ] Builds for the affected environments (at least `esp32dev` or `esp32s3nano`)
- [ ] `pio test -e native` passes
- [ ] Portal changes: `cd test/web && npm test` passes
- [ ] Docs updated, and `python3 scripts/doc_lint.py` passes
- [ ] Breaking changes are listed in `CHANGELOG.md` under `[Unreleased]` with a migration note
- [ ] No secrets, private keys, SSIDs or personal paths in the diff
