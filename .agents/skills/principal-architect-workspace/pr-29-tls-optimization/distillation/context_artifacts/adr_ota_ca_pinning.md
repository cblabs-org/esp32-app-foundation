# ADR: OTA CA Pinning Architecture

## Context

During live OTA testing, we discovered that `objects.githubusercontent.com` (GitHub CDN) uses **Let's Encrypt (ISRG Root X1)**, NOT Sectigo. The manifest download was using `setCACert(GITHUB_ROOT_CA_CERT)` (Sectigo E46), which works for `api.github.com` but fails on the CDN redirect. This was a pre-existing bug masked by the full cert bundle.

## CDN Certificate Chain

```
*.github.io → R12 (Let's Encrypt) → ISRG Root X1
```

Verified via: `openssl s_client -connect objects.githubusercontent.com:443`

## Decision: Dual-CA bundle with graceful fallback

### Architecture

1. Create `lib/GithubClient/ota_root_cas.h` with both PEM certs concatenated:
   - Sectigo Public Server Authentication Root E46 (GitHub API)
   - ISRG Root X1 (GitHub CDN / Let's Encrypt)

2. `WiFiClientSecure::setCACert()` handles multiple PEMs — `mbedtls_x509_crt_parse()` parses all certs from a single buffer.

3. If CA validation fails, fall back to `setInsecure()` with a warning log. The firmware cryptographic signature is the mandatory trust anchor, not transport TLS.

### Storage: Why compiled-in, not NVS/LittleFS

**Evaluated options:**
- **NVS**: Survives firmware OTA (separate partition), individual keys updatable. But adds boot-time complexity and the chicken-and-egg problem remains.
- **LittleFS**: `uploadfs` overwrites the ENTIRE partition — loses KeyManager encrypted credentials, WiFi config, everything. No selective file update via filesystem OTA.
- **Compiled (chosen)**: Always available, no filesystem dependency, zero boot cost. ~2.5 KB flash for two PEM certs. The graceful fallback to `setInsecure()` solves the chicken-and-egg problem (if CAs change, OTA still works via the fallback).

### Critical insight: Firmware OTA does NOT touch NVS or LittleFS

```
┌─────────────┐
│ nvs         │ ← untouched (DeviceCrypto master key safe)
│ otadata     │ ← boot selector (updated by OTA)
│ app0        │ ← active firmware
│ app1        │ ← OTA writes here, then switches boot
│ littlefs    │ ← untouched (KeyManager creds safe)
└─────────────┘
```

## WO4 Handoff

Work order written at: `.agents/skills/principal-architect-workspace/pr-29-tls-optimization/wo4-ota-ca-pinning/WORKORDER.md`

Handed off to the OTA architect for implementation. Boundary box: `lib/GithubClient/` + `modules/OTAUpdateManager/OTAUpdateManager.cpp` only.
