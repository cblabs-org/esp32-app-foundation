# Architecture Decision Record: Key Manager

## Encrypted Credential Store
To protect sensitive credentials (such as Wi-Fi passwords and API keys) on the LittleFS filesystem, `KeyManager` encrypts payloads using AES-256 keys derived from `deviceCrypto` (hardware MAC addresses).

## Key CRUD Operations
`KeyManager` exposes explicit CRUD contracts: `addKey(id, secret)`, `getKeyById(id, buffer, size)`, and `deleteKey(id)`. This keeps caller interfaces extremely clean and prevents exposure of encryption logic to upper application layers.
