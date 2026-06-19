# Graveyard — Build Failures and Dead Ends

## 1. ALPN Compile Guard (BLOCKING — fixed)

**What happened:** Setting `CONFIG_MBEDTLS_SSL_ALPN=n` caused a linker error.

**Root cause:** Arduino's `ssl_client.cpp` (in `NetworkClientSecure` library) calls `mbedtls_ssl_conf_alpn_protocols()` unconditionally — there is no `#ifdef CONFIG_MBEDTLS_SSL_ALPN` guard around the call.

**Fix:** Forced `CONFIG_MBEDTLS_SSL_ALPN=y` with a comment explaining the constraint.

**Lesson:** Always check Arduino's `ssl_client.cpp` for unconditional mbedTLS API calls before disabling any `CONFIG_MBEDTLS_SSL_*` feature. Same class as the PSK compile guard documented elsewhere.

## 2. Serial Port Locking During Flash

**What happened:** `pio run -e esp32s3nano -t upload` failed with `Could not exclusively lock port /dev/cu.usbmodem101: Resource temporarily unavailable`.

**Root cause:** The PlatformIO MCP serial monitor was holding the port.

**Fix:** Stop the monitor before flashing:
```python
call_mcp_tool("platformio", "stop_monitor", {"port": "/dev/cu.usbmodem101", "projectDir": "..."})
```

**Lesson:** Always stop the serial monitor before attempting USB flash. Restart after flash completes.

## 3. CDN CA Mismatch — Manifest Download Failure

**What happened:** OTA manifest download failed with `-9984 X509 - Certificate verification failed`.

**Root cause:** The manifest download redirects from `api.github.com` → `objects.githubusercontent.com`. The CDN uses **Let's Encrypt (ISRG Root X1)**, not the pinned **Sectigo E46**. With cert bundle `NONE`, no fallback CA exists.

**Serial output:**
```
[ssl_client.cpp:36] _handle_error(): [ssl_starttls_handshake():318]: (-9984) X509 - Certificate verification failed
[NetworkClientSecure.cpp:159] connect(): start_ssl_client: connect failed: -9984
[HTTPClient.cpp:1423] returnError(): error(-1): connection refused
OTAUpdateManager: Manifest download HTTP -1
```

**Temporary fix:** Changed manifest download to `setInsecure()` (commit `e2c8914`).

**Proper fix:** WO4 — dual-CA bundle with graceful fallback (handed off to OTA architect).

**Lesson:** GitHub API and GitHub CDN use DIFFERENT CA chains. Always verify the actual cert chain for redirect destinations, not just the initial URL.

## 4. GitHub Releases JSON IncompleteInput

**What happened:** During OTA check, `GITHUB: Page 1 JSON error: IncompleteInput (11532 bytes read)`.

**Root cause:** Pre-existing issue — the releases page response is larger than the JSON parser's streaming buffer. Not related to TLS optimization.

**Impact:** Falls back to `v0.0.5` (stable baseline), which is correctly identified as older than `dev.9`. No OTA triggered. Not a regression.

## 5. `gh` CLI Authentication Conflict

**What happened:** `gh release list` returned 401 Unauthorized.

**Root cause:** An invalid `GITHUB_TOKEN` environment variable overrides the valid keyring credential.

**Fix:** `unset GITHUB_TOKEN` before any `gh` command.

## 6. deploy_release.py Passphrase Input

**What happened:** `deploy_release.py` prompts for the private key passphrase. Background tasks can't handle interactive input securely.

**Workaround:** Kill the background task and have the user run the command directly in their terminal (without `--build` if binaries are already built in `./release/`).
