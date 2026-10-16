# modules/KeyManager

## Invariants & contracts

### [docseed-00ae23a1] **Lightweight arrays** (`KeyManager` entries, web routes, config consumers) — `std::vector` that may grow at any time since additions do not affect loop scheduling or carousel timing. No `shrink_to_fit()` required.
`invariant · kind: semantic · anchors: modules/KeyManager/KeyManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-46a1e0c0] **`managedBy`** — The owning module name (e.g., `"WiFi"`, `"Security"`). Empty for user-created keys.
`invariant · kind: semantic · anchors: modules/KeyManager/KeyManager.hpp, lib/DeviceCrypto/DeviceCrypto.hpp, include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-573db0ee] The REST API (`GET /api/keys`) never returns plaintext key values — only key IDs and metadata.
`invariant · kind: semantic · anchors: modules/KeyManager/KeyManager.hpp, lib/DeviceCrypto/DeviceCrypto.hpp, include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-84909e48] **Never store plaintext credentials** in LittleFS config files. Always use `KeyManager.addKey()`.
`invariant · kind: semantic · anchors: modules/KeyManager/KeyManager.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-864c8262] Stored as `SHA-256(salt || password)` inside `KeyManager` under the keys `"portal_password_salt"` and `"portal_password_hash"`.
`invariant · kind: semantic · anchors: modules/KeyManager/KeyManager.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-950acef3] `POST /api/weather/config` — saves settings, stores API key via `KeyManager`, triggers immediate background fetch
`invariant · kind: semantic · anchors: modules/KeyManager/KeyManager.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-b496d609] **Grep for `addKey` / `getKeyById`** → Verify credentials flow through `KeyManager`, not raw NVS or plaintext config files.
`invariant · kind: semantic · anchors: modules/KeyManager/KeyManager.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-e9b5fba6] **Why Two Mechanisms?**: `registerModule()` handles *contracts* — "this module is tickable, configurable, web-contributing." But it doesn't know *which* framework services your module needs at runtime. A weather module needs `KeyManager` for API key storage; a sensor module might not. The setter pattern keeps dependencies explicit and avoidable.
`contract · kind: semantic · anchors: modules/KeyManager/KeyManager.hpp · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-f3aa6733] **Secrets kept out of plaintext config**: WiFi password lives in `KeyManager` (encrypted), not config files; SSID/password split is deliberate.
`invariant · kind: semantic · anchors: modules/KeyManager/KeyManager.hpp · source_doc: docs/archive/security-evaluation-report.md · evidence: doc:docs/archive/security-evaluation-report.md`

### [docseed-f3e6be00] Keys with `exclusive = true` cannot be modified or deleted via the REST API — only by the owning module.
`invariant · kind: semantic · anchors: modules/KeyManager/KeyManager.hpp, lib/DeviceCrypto/DeviceCrypto.hpp, include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-fb38f20b] **`exclusive`** — When `true`, the key is write-protected: it cannot be modified or deleted via the REST API, only by the owning module.
`invariant · kind: semantic · anchors: modules/KeyManager/KeyManager.hpp, lib/DeviceCrypto/DeviceCrypto.hpp, include/interfaces/IWebContributor.hpp, modules/WebServer/WebServer.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

## Ruled out, failure signatures and gotchas

### [docseed-32b9eab6] **Fast-fail on missing prerequisites**: If credentials (e.g. API keys in `KeyManager`) are missing, return `UpdateStatus::AuthError` immediately without opening a network connection.
`failure-signature · kind: procedural · anchors: modules/KeyManager/KeyManager.hpp · source_doc: docs/developer/fetch-context-contract.md · evidence: doc:docs/developer/fetch-context-contract.md`
