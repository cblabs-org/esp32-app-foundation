# PR-24 Graveyard

## 1. ApiQueue Deadlocks
**Symptom:** The System tab (or any subsequent API call) would permanently hang after an authentication failure.
**Cause:** The `ApiQueue` interceptor in `app.js` checked for a 401 response and triggered the login overlay. If the user cancelled or if the request dropped, the `catch` block executed an early `return` that completely bypassed the `finally` block. This left `isProcessing = true` forever.
**Solution:** Ensure `isProcessing = false` and `setTimeout(() => this.processNext(), 0)` are always called in the `catch` block.

## 2. Browser TLS Connection Storms (`0x7780`)
**Symptom:** Chrome showed `ERR_CONNECTION_CLOSED` randomly for `system.js` or API requests during the initial portal load. Serial logs showed `mbedtls_ssl_handshake returned -0x7780` and `session creation failed`.
**Cause:** The ESP32-S3 DRAM was exhausted. `loadScripts` fetches files sequentially, but `app.js` was immediately firing `loadAppInfo()` and `checkApMode()` concurrently. Additionally, `auth.js` fired `fetch('/api/auth/status')`. This created 4-5 parallel TLS handshakes.
**Solution:** Move `loadAppInfo()` and `checkApMode()` inside the `loadScripts` completion callback so they only fire after all static assets have finished downloading.

## 3. Chrome `ERR_CERT_INVALID`
**Symptom:** Self-signed certificates were completely rejected by modern Chrome with no option to bypass.
**Cause:** The mbedTLS certificate generation did not include a Subject Alternative Name (SAN) extension. Modern browsers require SANs; the Common Name (CN) is no longer sufficient.
**Solution:** Added the v3 SAN extension explicitly in `AuthManager.cpp` during cert generation to include the device IP and `esp32-device.local`.
