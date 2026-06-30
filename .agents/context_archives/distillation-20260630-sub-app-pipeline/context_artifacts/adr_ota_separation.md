# ADR: OTA Repository Separation

## Status
Accepted

## Context
The sample-app's `main.cpp` had a hardcoded `otaManifestUrl` pointing to the **framework's** releases repo (`esp32-app-foundation-releases`). If deployed to a device, OTA updates would pull the framework's firmware — not the sample-app's. The sample-app also included the framework's `publicKey.hpp` for signature verification.

## Decision
Replaced the hardcoded OTA configuration with commented-out TODO guidance explaining the 5 steps to set up OTA for a downstream app:
1. Create a releases repo
2. Generate a signing keypair
3. Extract the public key
4. Convert to a header
5. Set the manifest URL and public key

The `publicKey.hpp` include was removed from sample-app since it references the framework's key, not the app's.

## Rationale
- A downstream app MUST have its own signing keypair — sharing the framework's private key would be a security violation.
- The manifest URL must point to the app's own releases repo so OTA delivers the correct firmware.
- The template-app already had OTA disabled (nullptr) which was correct — we just added the same guidance comments.
- The root project's OTA config remains unchanged — it correctly points to its own releases repo.
