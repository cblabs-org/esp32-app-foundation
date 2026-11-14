# Security Policy

## Reporting a vulnerability

Please report security problems **privately**, not in a public issue, pull
request or discussion.

Use GitHub's private vulnerability reporting:
[**Report a vulnerability**](https://github.com/cblabs-org/esp32-app-foundation/security/advisories/new)
(the *Security* tab → *Report a vulnerability*).

Include what you can of:

- the affected module or file, and the version or commit
- the board and PlatformIO environment, if it matters
- steps to reproduce or a proof of concept
- the impact as you understand it (for example: portal auth bypass, OTA
  signature bypass, credential disclosure, remote crash)

We aim to acknowledge a report within 7 days and to agree a disclosure date
with you once the issue is confirmed. We credit reporters in the advisory and
the changelog unless you would rather not be named.

## Supported versions

During the 1.0 public beta, fixes go into the next release on the newest
series only.

| Version | Supported |
|---|---|
| `1.0.0-beta.N` (latest) | ✅ |
| Older betas and all `0.x` releases | ❌ — please upgrade |

## Scope

In scope: the framework code in this repository (`modules/`, `lib/`,
`include/`, `src/`, `adapters/`, `web/`, `scripts/`) and the sample and
template apps.

Out of scope: vulnerabilities in ESP-IDF, Arduino-ESP32 or other third-party
libraries (report those upstream; tell us too if the framework's use of them
makes things worse), and attacks that need physical access to an unencrypted
device's flash.

The threat model the framework is designed against is described in
[Security § Threat Model](docs/developer/security.md#1-threat-model).

## OTA signing keys

Devices trust the RSA public key compiled into their firmware. If you ship
products on the framework:

- Keep the private signing key out of the repository and off CI runners; the
  `.gitignore` excludes `*.pem` and `*.key`, but check before every commit.
- Protect the key with a passphrase (`scripts/generate_keys.sh` does this) and
  sign releases on a trusted machine (`scripts/deploy_release.py`).
- If a key may have leaked, follow the rotation steps in
  [OTA Update System § Key Rotation](docs/developer/ota-update-system.md#63-key-rotation)
  and raise `SECURITY_VERSION` so devices refuse older images.
