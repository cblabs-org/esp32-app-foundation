# Build and Release Process

> ESP32 App Foundation (c) 2026 CB Labs

This document covers the complete lifecycle for a developer taking this repository from a fresh clone through to a signed firmware release published for OTA update.

---

## Table of Contents

1. [Prerequisites](#1-prerequisites)
2. [Repository Setup](#2-repository-setup)
3. [Generating the Signing Key Pair](#3-generating-the-signing-key-pair)
4. [Embedding the Public Key in Firmware](#4-embedding-the-public-key-in-firmware)
5. [Building Firmware for All Targets](#5-building-firmware-for-all-targets)
6. [Running Tests](#6-running-tests)
7. [Release Channels](#7-release-channels)
8. [Tagging a Release](#8-tagging-a-release)
9. [Signing and Publishing a Release](#9-signing-and-publishing-a-release)
10. [Verifying the Release](#10-verifying-the-release)
11. [CI-Assisted Workflow (Optional)](#11-ci-assisted-workflow-optional)
12. [Key Rotation](#12-key-rotation)

---

## 1. Prerequisites

### Required tools

| Tool | Purpose | Install |
|------|---------|---------|
| **Python 3.8+** | Build scripts, release tooling | [python.org](https://python.org) |
| **PlatformIO CLI** (`pio`) | Firmware compilation and flash | `pip install platformio` |
| **OpenSSL** | Key generation and firmware signing | Pre-installed on macOS/Linux; [slproweb.com](https://slproweb.com/products/Win32OpenSSL.html) on Windows |
| **GitHub CLI** (`gh`) | Creating and uploading GitHub Releases | [cli.github.com](https://cli.github.com) |
| **Git** | Source control and version tagging | [git-scm.com](https://git-scm.com) |

### Verify your tools

```bash
python3 --version    # 3.8+
pio --version        # any recent version
openssl version      # any version ≥ 1.1
gh --version         # any recent version
git --version        # any version
```

### Authenticate GitHub CLI

```bash
gh auth login
# Follow the prompts — choose GitHub.com → HTTPS → authenticate via browser
gh auth status       # confirm you are logged in
```

---

## 2. Repository Setup

```bash
git clone https://github.com/matthewmcneill/esp32-app-foundation.git
cd esp32-app-foundation
```

Install PlatformIO project dependencies (this happens automatically on first build, but can be triggered explicitly):

```bash
pio pkg install
```

---

## 3. Generating the Signing Key Pair

The firmware release pipeline uses **RSA-2048** asymmetric signing:
- The **private key** signs the `.bin` binary at release time. It is **never committed to git** and is **AES-256 passphrase-protected**.
- The **public key** is embedded in the firmware and used to verify OTA downloads before flashing.

> [!IMPORTANT]
> This step only needs to be done **once per project**. If `private_key.pem` already exists, do not regenerate — you would need to rebuild and re-flash all devices with the new public key.

### Generate the key pair

```bash
chmod +x scripts/generate_keys.sh
./scripts/generate_keys.sh
```

You will be prompted to set a passphrase — choose something strong and store it securely (a password manager is recommended). You will need this passphrase **every time you sign a release**.

The script generates:
- `private_key.pem` — AES-256 encrypted, `chmod 600`, **never commit to git**
- `public_key.pem` — safe to commit, used to generate the embedded header

### Verify the key is encrypted

```bash
head -1 private_key.pem
# Expected: -----BEGIN ENCRYPTED PRIVATE KEY-----
# (NOT "BEGIN PRIVATE KEY" or "BEGIN RSA PRIVATE KEY" — those are unencrypted)
```

### Verify `.gitignore` protection

The following sensitive files are excluded from git by `.gitignore` and must **never** be committed:

| File | Reason |
|------|--------|
| `private_key.pem` | Private signing key — compromise allows unsigned firmware to be accepted by devices |
| `public_key.pem` | Source form of the public key — the compiled header is what matters |
| `include/publicKey.hpp` | Generated C header — regenerated from `public_key.pem` at any time |
| `release/` | Staging directory containing signed binaries |
| `*.sig` | Detached signatures |
| `*.sha256` | Hash files |

Verify nothing sensitive is tracked:

```bash
git check-ignore private_key.pem include/publicKey.hpp release/ && echo "All protected ✓"
git status --short | grep -E "pem|sig|sha256|release/"  # Should produce no output
```

> [!CAUTION]
> If `private_key.pem` ever appears in `git status` as untracked or staged, **do not commit**. Run `git rm --cached private_key.pem` immediately and verify `.gitignore` is correct.

---

## 4. Embedding the Public Key in Firmware

The public key must be converted to a C header so the firmware can verify OTA signatures at runtime.

```bash
xxd -i public_key.pem > include/publicKey.hpp
```

Then in your application's `main.cpp`, pass it to the framework:

```cpp
#include "publicKey.hpp"

FrameworkConfig config;
config.otaPublicKey = reinterpret_cast<const char*>(public_key_pem);
config.otaGithubRepo = "matthewmcneill/esp32-app-foundation";
```

> [!NOTE]
> Whenever you rotate keys, devices in the field will still carry the **old** public key and will reject binaries signed with the new private key. You must use a two-step transitional process to avoid breaking OTA for deployed devices.

**Transitional signing is mandatory.** See [Key Rotation](#11-key-rotation) for the full procedure.

---

## 5. Building Firmware for All Targets

The project supports two hardware targets:

| Environment | Board |
|-------------|-------|
| `esp32dev` | Generic ESP32 Dev Module |
| `esp32s3nano` | ESP32-S3 Nano (USB-JTAG console) |

### Build all targets

```bash
pio run -e esp32dev
pio run -e esp32s3nano
```

Output binaries land in:
- `.pio/build/esp32dev/firmware.bin`
- `.pio/build/esp32s3nano/firmware.bin`

### Version injection

The version is automatically injected at build time by `scripts/inject_version.py`. It reads all git tags pointing at `HEAD`, parses them as SemVer, and selects the highest. Ensure your release commit is tagged **before** building:

```bash
git tag v1.2.3
pio run -e esp32dev   # will inject version 1.2.3
```

### Flashing a device for local testing

```bash
pio run -e esp32s3nano --target upload   # or esp32dev
```

---

## 6. Running Tests

Run the full native test suite before tagging a release:

```bash
pio test -e native
```

All tests must pass. The test suite covers:
- SemVer parsing and comparison
- OTA config schema migration
- OTA manifest parsing
- Config manager lifecycle
- ConfigManifest rollback logic
- MCP server tooling

---

## 7. Release Channels

The firmware supports three release channels that allow you to ship different stability tiers to different devices simultaneously.

### Channel definitions

| Channel | Tag suffix | Intended audience | Typical use |
|---------|-----------|------------------|-------------|
| **stable** | *(none)* e.g. `v1.2.3` | All production devices | Fully tested, recommended for all users |
| **beta** | `-beta.N` e.g. `v1.2.3-beta.1` | Opt-in testers | Feature-complete but undergoing final validation |
| **dev** | `-dev.N` e.g. `v1.2.3-dev.0` | Developers only | Work-in-progress builds, may be unstable |

### How channel assignment works

The channel is inferred automatically from the git tag suffix by `scripts/deploy_release.py` — no separate flag is needed. The tag pattern determines the channel written into `manifest.json`:

```
v1.2.3           → channel: stable
v1.2.3-beta.1    → channel: beta
v1.2.3-dev.0     → channel: dev
```

A device subscribed to `stable` will only ever see releases tagged without a suffix. A device subscribed to `beta` will see beta and stable releases (beta is a superset of stable). A device subscribed to `dev` sees all releases.

### How devices subscribe to a channel

Each device independently selects its channel via the web portal:

1. Open the device portal → navigate to **System → OTA**
2. In the **Settings** card, use the **Update Channel** dropdown
3. Select `Stable`, `Beta`, or `Dev`
4. The setting is persisted to NVS — it survives reboots and OTA updates

The device sends its subscribed channel to GitHub when checking for updates. The `GithubClient` filters releases by matching the channel label in the manifest before comparing versions.

### Typical promotion workflow

For a significant change, the recommended process is:

```
┌─────────────────────────────────────────────────────────────────────┐
│  1. Ship v1.2.3-dev.0  →  dev-channel devices receive and test      │
│  2. Ship v1.2.3-beta.1 →  beta-channel devices receive and test     │
│  3. Ship v1.2.3        →  all stable devices receive the release    │
└─────────────────────────────────────────────────────────────────────┘
```

Each stage uses the same signing and publishing command — only the tag changes:

```bash
# Stage 1 — dev release
git tag v1.2.3-dev.0 && git push origin v1.2.3-dev.0
python3 scripts/deploy_release.py --tag v1.2.3-dev.0 --local --build --private-key private_key.pem

# Stage 2 — beta release (same commit or a fixed one)
git tag v1.2.3-beta.1 && git push origin v1.2.3-beta.1
python3 scripts/deploy_release.py --tag v1.2.3-beta.1 --local --build --private-key private_key.pem

# Stage 3 — stable release
git tag v1.2.3 && git push origin v1.2.3
python3 scripts/deploy_release.py --tag v1.2.3 --local --build --private-key private_key.pem
```

> [!NOTE]
> There is no automated promotion between channels — each stage is an explicit tag and publish. This is intentional: a dev build and a stable build may be built from slightly different commits after final fixes are cherry-picked.

### Hotfix releases

For urgent fixes that bypass the full dev → beta → stable cycle, tag directly on the stable channel with an incremented patch:

```bash
git tag v1.2.4 && git push origin v1.2.4
python3 scripts/deploy_release.py --tag v1.2.4 --local --build --private-key private_key.pem
```

---

## 8. Tagging a Release

The version tag drives both the build version injection and the GitHub Release name.

### Tag format

| Channel | Tag pattern | Example |
|---------|-------------|---------|
| Stable | `vMAJOR.MINOR.PATCH` | `v1.2.3` |
| Beta | `vMAJOR.MINOR.PATCH-beta.N` | `v1.2.3-beta.1` |
| Dev | `vMAJOR.MINOR.PATCH-dev.N` | `v1.2.3-dev.0` |

### Create and push a tag

```bash
git tag v1.2.3
git push origin v1.2.3
```

> [!CAUTION]
> Do not tag until all commits for the release are on the branch. The version injector reads `git tag --points-at HEAD` — if you tag after building, the binary will embed the wrong version.

---

## 9. Signing and Publishing a Release

All signing happens **locally**. The private key never touches GitHub Actions or any remote server.

### Option A — Build, sign and publish in one command (recommended)

```bash
python3 scripts/deploy_release.py \
    --tag v1.2.3 \
    --local --build \
    --private-key private_key.pem
```

This single command:
1. Runs `pio run -e esp32dev` and `pio run -e esp32s3nano`
2. Copies the `.bin` files into `release/`
3. Signs each binary with your private key (**passphrase prompted twice**)
4. Generates SHA-256 hash files
5. Generates `release/manifest.json`
6. Creates the GitHub Release with `gh release create`
7. Uploads all assets: `.bin`, `.bin.sig`, `.bin.sha256`, `manifest.json`

### Option B — Sign already-built binaries

If you have already built and just want to sign and publish:

```bash
python3 scripts/deploy_release.py \
    --tag v1.2.3 \
    --local \
    --private-key private_key.pem
```

### Option C — Dry run (preview without executing)

```bash
python3 scripts/deploy_release.py \
    --tag v1.2.3 \
    --local --build \
    --private-key private_key.pem \
    --dry-run
```

### What gets uploaded to the GitHub Release

| File | Description |
|------|-------------|
| `firmware-esp32dev.bin` | Compiled firmware for ESP32 Dev Module |
| `firmware-esp32dev.bin.sig` | RSA-2048 PKCS#1 v1.5 SHA-256 signature |
| `firmware-esp32dev.bin.sha256` | SHA-256 hash for integrity check |
| `firmware-esp32s3nano.bin` | Compiled firmware for ESP32-S3 Nano |
| `firmware-esp32s3nano.bin.sig` | RSA-2048 PKCS#1 v1.5 SHA-256 signature |
| `firmware-esp32s3nano.bin.sha256` | SHA-256 hash for integrity check |
| `manifest.json` | Multi-board OTA manifest (version, channel, hashes) |

---

## 10. Verifying the Release

### Check the GitHub Release

```bash
gh release view v1.2.3
```

### Manually verify a signature

```bash
openssl dgst -sha256 \
    -verify public_key.pem \
    -signature release/firmware-esp32dev.bin.sig \
    release/firmware-esp32dev.bin
# Expected: Verified OK
```

### Test OTA from a running device

1. Open the device web portal
2. Navigate to **System → OTA**
3. Click **Check for Updates**
4. If a newer version is available, the device will download, verify, flash and reboot automatically

The device firmware verifies the ECDSA signature before writing a single byte. A bad or missing signature aborts the update with no flash written.

---

## 11. CI-Assisted Workflow (Optional)

GitHub Actions (`.github/workflows/build.yml`) builds unsigned binaries on every tag push. This is useful for reproducible builds on a clean environment but is **not required** — the local workflow above is self-contained.

If you use CI:

1. Push a tag — Actions builds both targets and uploads unsigned `.bin` artifacts
2. Find the workflow run ID: `gh run list --limit 5`
3. Download, sign and publish:

```bash
python3 scripts/deploy_release.py \
    --tag v1.2.3 \
    --run-id <ACTIONS_RUN_ID> \
    --private-key private_key.pem
```

The `--run-id` path downloads the unsigned CI artifacts, then signs them locally exactly as the `--local` path does. The private key never leaves your machine either way.

---

## 12. Key Rotation

Key rotation requires a **two-step transitional release** because of an inherent ordering constraint:

- Devices in the field verify OTA downloads using the **old public key** embedded in their running firmware
- A binary signed with the **new private key** will be rejected by those devices
- Therefore you must use the **old private key** to sign one final transitional release that ships the **new public key** embedded in firmware — only then can you switch to signing with the new key

### Step-by-step rotation procedure

**Step 1 — Generate the new key pair**

```bash
rm private_key.pem public_key.pem          # remove old keys
./scripts/generate_keys.sh                  # generate new pair (new passphrase)
```

**Step 2 — Embed the new public key in firmware**

```bash
xxd -i public_key.pem > include/publicKey.hpp
```

**Step 3 — Build the transitional release**

This firmware contains the **new public key** but must be **signed with the old private key**. You must still have your old `private_key.pem` (or a backup of it) at this point:

```bash
git tag v1.2.4-transition                   # or whatever your next version is
git push origin v1.2.4-transition
python3 scripts/deploy_release.py \
    --tag v1.2.4-transition \
    --local --build \
    --private-key old_private_key.pem        # sign with OLD key
```

**Step 4 — Deploy the transitional release to all devices**

All devices in the field must install `v1.2.4-transition` via OTA. Once they do, they carry the new public key. Monitor adoption before proceeding.

**Step 5 — Switch to the new key for all future releases**

```bash
python3 scripts/deploy_release.py \
    --tag v1.3.0 \
    --local --build \
    --private-key private_key.pem            # sign with NEW key from here on
```

> [!WARNING]
> Any device that misses the transitional release (`v1.2.4-transition`) will still carry the old public key and will be unable to accept OTA updates signed with the new private key. Those devices must be **manually re-flashed via USB**. Ensure maximum fleet adoption of the transitional release before discarding the old private key.

> [!CAUTION]
> Keep a secure backup of the **old** private key until you are confident all deployed devices have received the transitional update. After that, the old key can be permanently deleted.
