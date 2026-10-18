# State: Script CLI Interfaces (post-WO4)

## `generate_manifest.py`

```
generate_manifest.py --manifest-url URL [--output FILE] [--merge-into FILE]
    Single-channel: --version TAG --channel CH --security-version N --boards B --firmware-dir DIR --base-url URL
    Multi-channel:  --channels A,B --versions X,Y --security-versions N,M --firmware-dirs D1,D2 --base-urls U1,U2
```

Key args:
- `--manifest-url` (required) — self-referencing URL
- `--base-url` (required in single mode) — prefix for asset URLs
- `--merge-into` (optional) — path to existing manifest.json to merge into
- `--output` (optional) — file path, default stdout

## `deploy_release.py`

```
deploy_release.py --tag TAG --private-key KEY (--run-id ID | --local)
    [--build] [--repo OWNER/REPO] [--sign-only] [--legacy-floating-tags] [--dry-run]
```

Pipeline steps:
1. Step 0: Ensure git tag on HEAD
2. Step 1: Collect/download binaries
3. Step 2: Sign firmware binaries
4. Step 3a: Generate SHA-256 hashes
5. Step 3b: Generate manifest.json (per-release)
6. Step 3c: Sign manifest.json
7. Step 4: Create GitHub Release (versioned)
8. Step 5: Upload versioned release assets
9. Step 6: Update OTA manifest release (download → merge → risk-floor → sign → upload)
10. Step 7 (LEGACY, opt-in): Update floating tags (behind `--legacy-floating-tags`)

New flags:
- `--legacy-floating-tags` — opt-in, deprecated, fires `DeprecationWarning`
- `--sign-only` — stops after Step 3c
