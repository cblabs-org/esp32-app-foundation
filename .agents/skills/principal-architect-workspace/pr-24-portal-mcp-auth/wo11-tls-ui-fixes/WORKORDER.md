# PR-24 WO11 — Fix TLS Initialization & UI Auth Promise

## Context
1. **TLS Not Starting:** The server's TLS initialization was deferred via a hardcoded bypass (`useTls = false`) during an earlier PR phase in `WebServerManager.cpp`. As a result, the certificates are never loaded and port 443 remains closed in STA mode.
2. **UI Bearer Token Bug:** The `ApiQueue` interceptor handles `401 Unauthorized` by triggering the login overlay but immediately rejects the original `fetch` promise. This causes privileged UI actions to fail instantly instead of pausing and retrying after a successful login.

## Objective
Enable native TLS in STA mode and convert the Javascript authentication flow into a seamless, awaitable Promise architecture.

## Execution Checklist

- `[ ]` **Enable Native TLS (`WebServerManager.cpp`)**
  - In `WebServerManager::begin()`, replace the hardcoded `useTls = false` fallback block with dynamic certificate loading:
    ```cpp
    bool useTls = false;
    #ifndef NATIVE_BUILD
        if (!apMode && authManager && authManager->isAuthEnabled() && authManager->hasCert()) {
            uint8_t *certDer = nullptr, *keyDer = nullptr;
            size_t certLen = 0, keyLen = 0;

            if (authManager->loadCert(&certDer, &certLen, &keyDer, &keyLen)) {
                ESP_LOGI(TAG, "Auth enabled with TLS cert present — enabling HTTPS on port 443");
                useTls = true;
                _httpsServer = new PsychicHttpsServer();
                _httpsServer->setCertificate(certDer, certLen, keyDer, keyLen);
            } else {
                ESP_LOGE(TAG, "Failed to load cert from storage, falling back to HTTP");
            }
        }
    #endif
    ```

- `[ ]` **Awaitable Auth Flow (`web/portal/auth.js`)**
  - Update `showLoginOverlay()` to return a `Promise`.
  - Store the `resolve` and `reject` callbacks so they can be invoked later.
  - When the user successfully logs in (via `handleLoginSubmit`), call `resolve()`.
  - When the user clicks cancel/logout, call `reject(new Error("Login cancelled"))`.

- `[ ]` **ApiQueue Refactor (`web/portal/app.js`)**
  - In the `ApiQueue` where it intercepts `res.status === 401`:
    - Await `window.Auth.showLoginOverlay()`.
    - If it resolves, immediately retry the exact same `fetch` request (using the original URL and `opts`) and return that new fetch promise to the caller.
    - If it rejects, throw the error back to the caller.

- `[ ]` **Versioning**
  - **MANDATORY:** You must manually bump the version string in `include/Version.h` OR create a git tag (e.g., `v0.0.8-dev.7`) to comply with `.agents/rules/03-versioning.md` before flashing. Do NOT skip this step.

- `[ ]` **Verification**
  - Run `pio test -e native`.
  - Flash the firmware using the PIO Manager skill and monitor the boot sequence.

## Constraints & Rules
- Do NOT enable TLS if `apMode` is true (Captive Portal requirement).
- **You MUST ask the Principal Architect (user) for approval before distilling and terminating your session.**
