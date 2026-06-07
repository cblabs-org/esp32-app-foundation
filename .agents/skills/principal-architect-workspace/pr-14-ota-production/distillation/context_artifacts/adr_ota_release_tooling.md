# ADR: Release Tooling — Local Mode and Key Encryption

## Context

`scripts/deploy_release.py` was originally written assuming a CI workflow:
1. GitHub Actions builds unsigned binaries
2. Developer downloads artifacts via `gh run download <run-id>`
3. Developer signs locally (key never touches CI)
4. Developer publishes the release

In practice the user always builds locally, making step 1-2 redundant.

## Decision: Add `--local` mode to deploy_release.py

`--run-id` and `--local` are mutually exclusive (enforced via `add_mutually_exclusive_group`).

`--local` mode:
- Copies `.pio/build/<board>/firmware.bin` into `release/` for each board
- Skips the `gh run download` step entirely
- Continues with sign → hash → manifest → publish

`--build` flag (only valid with `--local`):
- Runs `pio run -e <board>` for each board before staging
- Allows the entire release flow in a single command:
  `python3 scripts/deploy_release.py --tag vX.X.X --local --build --private-key private_key.pem`

`generate_hashes()` was also added as an explicit step — previously SHA256 hashes were done manually and not in the script.

## Decision: AES-256 key encryption in generate_keys.sh

`openssl genrsa` updated to `openssl genrsa -aes256`. Passphrase is prompted interactively — never stored.

`openssl dgst -sign` automatically prompts for the passphrase when it encounters an encrypted key. No script changes needed for signing.

`chmod 600` applied to the private key on generation.

**Existing key encryption (user action, not scripted):**
```bash
openssl pkey -in private_key.pem -aes256 -out private_key_enc.pem \
  && mv private_key_enc.pem private_key.pem && chmod 600 private_key.pem
```
User confirmed this has been done.

## The GITHUB_TOKEN Blocker

The Antigravity agent environment injects `GITHUB_TOKEN=github_pat_antigravitydummytoken` into every subprocess. This is intentional — it prevents agents from making authenticated GitHub API calls autonomously. Any `gh release create` or `gh release upload` command run by the agent will always receive a 401. These steps must always be run by the user in their own terminal. This is not a bug.
