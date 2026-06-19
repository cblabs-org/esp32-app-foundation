# PAT Management

GitHub Personal Access Tokens (PATs) are used to authenticate OTA update requests against private repositories. This document covers token scope, expiration policy, security tradeoffs, and the repository split strategies that mitigate source-code exposure risk.

## PAT Scope Requirements

The OTA pipeline requires a **fine-grained** GitHub Personal Access Token with minimal permissions:

- **Token type:** Fine-grained Personal Access Token (not classic)
- **Repository access:** Scoped to a single repository only
- **Repository permissions:** `Contents: Read-only`
- **Account permissions:** None — no user-level permissions granted
- **Expiration:** No expiration (see rationale below)

Fine-grained tokens are strictly superior to classic tokens because they enforce single-repository scope and granular permissions. Classic tokens grant access to all repositories the user can access, which is unacceptable for a compiled-in credential.

## Expiration Policy Rationale

The PAT is compiled into the firmware binary and shipped to devices. This binary distribution model makes token expiration impractical:

- **Offline devices:** Deployed devices may be disconnected from the network for weeks or months. An expired PAT means the device can never perform OTA updates again — effectively bricking the update capability.
- **No renewal mechanism:** Consumer IoT devices have no way to interactively renew a token. NVS provisioning is impractical (see Design Decision D2 below).
- **Security boundary is scope, not expiry:** The fine-grained token's single-repo, read-only scope is the primary security control. Expiration adds operational fragility without meaningfully reducing the already-narrow blast radius.

## Blast Radius Analysis

If an attacker extracts the PAT from a firmware binary (e.g., via flash dump or binary analysis), the following table shows what they can and cannot do:

| Action | Possible? |
|---|---|
| Read repository source code | Yes |
| Read repository releases/assets | Yes |
| Write/push to repository | No |
| Access other repositories | No |
| Access user account settings | No |
| Create issues/PRs | No |

The blast radius is confined to **read-only access to a single repository**. The attacker cannot modify code, push malicious releases, or pivot to other repositories or account settings.

## Source-Code Exposure Vulnerability

GitHub fine-grained PATs with `Contents: Read` permission grant access to **all repository contents**, not just releases. GitHub does not offer a "releases-only" read permission.

**Implication:** An attacker who extracts the PAT from the firmware binary can clone the entire private repository, including source code, commit history, and any secrets committed to the repo.

This is the primary motivation for the repository split strategy — separating OTA release assets from source code so the PAT can only expose release binaries, not proprietary source.

## Repository Split Strategy

Three deployment patterns are available, offering increasing levels of source-code isolation:

### Option A — Single Private Repo with PAT

The simplest setup. Source code and OTA releases live in the same private repository. The PAT compiled into firmware grants read access to both.

**When to use:** Acceptable when source-code exposure is tolerable (e.g., the code is not commercially sensitive, or the threat model does not include firmware reverse-engineering).

**Configuration:**

```cpp
// main.cpp
config.otaGithubRepo = "owner/my-firmware";
config.otaGithubToken = OTA_GITHUB_TOKEN;  // from otaSecrets.hpp
```

```ini
# otaSecrets.hpp (gitignored)
#define OTA_GITHUB_TOKEN "github_pat_..."
```

```bash
# Deploy release to the same repo
python3 scripts/deploy_release.py --local --build --private-key private_key.pem
```

### Option B — Separate Public Releases Repo

Source code stays in a private repo. OTA releases are published to a **public** repository. The firmware does not need a PAT at all — public repos require no authentication for read access.

**When to use:** Maximum security. Eliminates the PAT from the firmware binary entirely. The release binaries are public, but they are signed — an attacker cannot substitute a malicious binary.

**Configuration:**

```cpp
// main.cpp
config.otaGithubRepo = "owner/my-firmware-releases";
config.otaGithubToken = nullptr;  // no token needed for public repos
```

```bash
# otaSecrets.hpp — not needed, do not create the file
```

```bash
# Deploy release to the public releases repo
python3 scripts/deploy_release.py --local --build --private-key private_key.pem \
    --repo owner/my-firmware-releases
```

### Option C — Separate Private Releases Repo with PAT

Source code stays in a private repo. OTA releases are published to a **separate private** repository. The PAT is scoped to the releases repo only, so extracting it exposes only release binaries, not source code.

**When to use:** When release binaries must remain private (e.g., commercial firmware) but source-code exposure is unacceptable. The PAT's blast radius is limited to the releases repo.

**Configuration:**

```cpp
// main.cpp
config.otaGithubRepo = "owner/my-firmware-releases";
config.otaGithubToken = OTA_GITHUB_TOKEN;  // PAT scoped to releases repo only
```

```ini
# otaSecrets.hpp (gitignored) — PAT for the releases repo, NOT the source repo
#define OTA_GITHUB_TOKEN "github_pat_..."
```

```bash
# Deploy release to the private releases repo
python3 scripts/deploy_release.py --local --build --private-key private_key.pem \
    --repo owner/my-firmware-releases
```

### Strategy Comparison

| Aspect | Option A | Option B | Option C |
|---|---|---|---|
| PAT in firmware | Yes | No | Yes |
| Source code exposed if PAT extracted | Yes | N/A | No |
| Release binaries public | No | Yes | No |
| Setup complexity | Low | Medium | Medium |
| `deploy_release.py --repo` needed | No | Yes | Yes |

## Design Decisions

### D1: Why No Third-Party OTA Library Was Adopted

The custom OTA stack (streaming signature verification, anti-rollback, release channel selection, multi-board manifest) already exceeds the capabilities of available third-party libraries. Libraries like ArduinoOTA provide basic HTTP pull but lack cryptographic verification, anti-rollback counters, GitHub release integration, and board-specific asset selection. Adopting one would require wrapping it heavily, negating the simplicity benefit, while introducing a dependency that may not track ESP-IDF API changes.

### D2: Why PAT Stays Compiled in Binary

NVS-based provisioning (writing the PAT to device storage at manufacturing or via a setup app) is impractical for consumer IoT devices:

- **No provisioning step:** Consumer devices are flashed once and shipped. There is no factory provisioning line or setup app that could inject a PAT into NVS.
- **NVS is per-device:** Each device would need its own provisioning pass, adding manufacturing complexity.
- **Credential rotation:** If the PAT needs to change, there is no mechanism to update NVS on deployed devices — the only update mechanism is OTA, which itself requires the PAT.

Compiling the PAT into the binary is the only practical approach. The fine-grained scope (single repo, read-only) keeps the blast radius small, and the repository split strategy (Options B/C) can further reduce exposure.

### D7: Why OTA Repo Is Configurable

`FrameworkConfig.otaGithubRepo` is a runtime-configurable field rather than a build-time constant because:

- It enables the split-repo pattern (Options B/C above) without requiring firmware source changes beyond `main.cpp`.
- `deploy_release.py --repo` mirrors this on the tooling side — releases can be published to any repo.
- Downstream applications that build on the framework can each point to their own OTA repository without forking or patching the framework.

## See Also
- [OTA Security](ota-security.md) — Signature verification, anti-rollback, transport security
- [TLS Tradeoffs](tls-tradeoffs.md) — Transport security cost/benefit analysis
- [Credential Storage](credential-storage.md) — DeviceCrypto and KeyManager
