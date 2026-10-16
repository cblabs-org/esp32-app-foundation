# include/types

## Invariants & contracts

### [docseed-3679cbec] **Configure `FrameworkConfig`** in your `main.cpp`:
`invariant · kind: semantic · anchors: include/types/FrameworkConfig.hpp · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-5b892141] `FrameworkConfig.otaManifestUrl` points to your `manifest-latest` release
`invariant · kind: semantic · anchors: include/types/FrameworkConfig.hpp · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-693cf001] `FrameworkConfig.otaPublicKey` is set to your PEM-encoded RSA public key
`invariant · kind: semantic · anchors: include/types/FrameworkConfig.hpp · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`

### [docseed-7e1370b8] **Fail-Closed Security Invariant**: When an RSA public key is configured in `FrameworkConfig.otaPublicKey`:
`invariant · kind: semantic · anchors: include/types/FrameworkConfig.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-8b6bddee] **`ctx.client`**: Pre-configured `WiFiClient*` using the ESP-IDF certificate bundle (`FrameworkSecureClient`) when `requiresTls() == true` (default), or standard `WiFiClient` when `false`. Consumers do NOT construct their own TLS clients or call `setInsecure()`.
`contract · kind: semantic · anchors: include/types/FrameworkSecureClient.hpp · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-957363b0] `types/` — Framework value types (FetchContext, AppState, etc.)
`invariant · kind: semantic · anchors: include/types · source_doc: .agents/skills/framework-expert/SKILL.md · evidence: doc:.agents/skills/framework-expert/SKILL.md`

### [docseed-aa9e7884] **Load FrameworkConfig:** Store internal `FrameworkConfig` and configure publishing throttles.
`invariant · kind: semantic · anchors: include/types/FrameworkConfig.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-b72d9513] **Verifier initialisation** — On the first firmware chunk, a `MbedTLSSignatureVerifier` is heap-allocated and initialized with the public key from `FrameworkConfig`.
`invariant · kind: semantic · anchors: include/types/FrameworkConfig.hpp · source_doc: docs/developer/security.md · evidence: doc:docs/developer/security.md`

### [docseed-c6c5e82a] Validating user credentials (defined via `FrameworkConfig` or stored in a secure partition)
`invariant · kind: semantic · anchors: include/types/FrameworkConfig.hpp · source_doc: docs/developer/extending-the-web-portal.md · evidence: doc:docs/developer/extending-the-web-portal.md`

### [docseed-d1ff705a] **Compiled-in URL** — set at build time in `FrameworkConfig.otaManifestUrl`, never overwritten, survives factory reset
`invariant · kind: semantic · anchors: include/types/FrameworkConfig.hpp · source_doc: docs/developer/ota-update-system.md · evidence: doc:docs/developer/ota-update-system.md`
