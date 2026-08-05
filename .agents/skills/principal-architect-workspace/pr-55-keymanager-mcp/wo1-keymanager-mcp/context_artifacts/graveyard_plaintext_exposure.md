# Graveyard: Initial key_get Plaintext Exposure

## Issue Summary
During initial implementation of `key_get`, the handler retrieved and returned the decrypted secret value using `getKeyById(id)`.

## Why It Failed Security Guidelines
Returning plaintext key values over MCP violates the core framework security model (matching `GET /api/keys` which strictly returns metadata only). 

## Corrected Design
`key_get` was refactored to look up key metadata via `listKeysWithMetadata()` and return `{id, hasValue, managedBy, exclusive, systemLocked}`. Plaintext values are never returned over MCP or REST.
