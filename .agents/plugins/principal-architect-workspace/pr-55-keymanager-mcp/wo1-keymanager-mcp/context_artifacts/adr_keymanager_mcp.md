# ADR: KeyManager IMCPContributor & systemLocked Protection Model

## Context & Objectives
`KeyManager` provides encrypted credential storage via `DeviceCrypto` on LittleFS (`/etc/keys.json`).
PR-55 exposes key management capabilities to the JSON-RPC `MCPServer` so automated testing agents can inspect and manage keys without direct physical or web access.

## Architectural Decisions

1. **Interface Implementation**:
   - `KeyManager` implements `IMCPContributor` alongside `IWebContributor`.
   - Exposes 4 MCP tools: `key_list`, `key_get`, `key_add`, and `key_delete`.

2. **Two-Tier Lock Architecture**:
   - **`exclusive` (App Lock)**: Set when a module stores keys (e.g. WiFi, Security). Write-protected over REST API. Overridable over MCP *only* if the caller provides `managedBy` matching `getOwner(id)`.
   - **`systemLocked` (Framework Lock)**: Set on critical framework secrets (`portal_password_salt`, `portal_password_hash`, `wifi_password`). Strictly un-bypassable over both MCP and REST API.

3. **Plaintext Protection (`key_get`)**:
   - To align with the security model of the REST API (where `GET /api/keys` never exposes plaintext values), `key_get` returns metadata (`id`, `hasValue`, `managedBy`, `exclusive`, `systemLocked`) for a single key, but NEVER decrypts or returns the secret plaintext value.

4. **Length Validation**:
   - `key_add` enforces `id` length <= 64 characters and `value` length <= 1024 characters.
   - `key_delete` enforces `id` length <= 64 characters.
