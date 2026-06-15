# ADR: TLS Initialization & UI Auth Promise

## Architecture Decisions
1. **Dynamic TLS Initialization**: `WebServerManager::begin()` checks for the presence of certificates and if authentication is enabled. If true, it instantiates `PsychicHttpsServer` and passes the generated certificates to start the native TLS listener on port 443. The HTTP server on port 80 is configured strictly as a redirect-only proxy.
2. **Awaitable Javascript Auth Flow**: We refactored `showLoginOverlay()` to return a native Javascript `Promise`, bridging the gap between the asynchronous UI interaction and the blocking requirements of `ApiQueue`.
3. **Transparent Request Replay**: `ApiQueue.processNext()` intercepts 401 Unauthorized responses. It calls `await _authInterceptor()` (mapped to the login overlay), waits for the user to resolve the login, and then dynamically repeats the exact same `fetch()` request without bubbling the error back to the application logic.

## Technical Nuances
- We added a cancel button to the login overlay to allow users to abort the login prompt, triggering the `reject()` callback to gracefully fail the intercepted promise without deadlocking `ApiQueue`.
- Version `v0.0.8-dev.7` tagged.
