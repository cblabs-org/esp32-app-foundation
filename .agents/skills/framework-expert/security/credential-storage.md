# Credential Storage

The framework provides a two-layer credential storage system that ensures sensitive data (WiFi passwords, API tokens, encryption keys) is never stored in plaintext.

## DeviceCrypto — Hardware-Bound Encryption

`DeviceCrypto` provides AES-256-CBC encryption using a 256-bit master key stored in NVS (Non-Volatile Storage).

### API

| Method | Signature | Purpose |
|---|---|---|
| `begin()` | `void begin()` | Mounts the NVS partition, loads the master key. If no key exists, generates one using `esp_random()` and saves it to NVS. |
| `encrypt()` | `std::unique_ptr<char[]> encrypt(const char* plaintext, size_t inputLen, size_t* outLen)` | Encrypts plaintext using AES-256-CBC with PKCS#7 padding. Returns a Base64-encoded ciphertext string via RAII pointer, or `nullptr` on failure. |
| `decrypt()` | `std::unique_ptr<char[]> decrypt(const char* ciphertext, size_t inputLen, size_t* outLen)` | Decrypts a Base64-encoded ciphertext string back to plaintext. Returns RAII pointer, or `nullptr` on failure. |

Both `encrypt()` and `decrypt()` are `virtual`, allowing test mocks in native builds.

### Rules
- The master key is a 256-bit AES key generated once on first boot via `esp_random()` and persists across firmware updates in NVS.
- The master key is NOT exportable — it exists only in NVS on the device.
- If NVS is erased, all encrypted credentials become unrecoverable. This is by design.
- The `aesKey[32]` array is held in RAM for the lifetime of the process after `begin()` is called.

## KeyManager — Encrypted Credential Registry

`KeyManager` builds on `DeviceCrypto` to provide a named credential registry. It is a kernel service implementing `IWebContributor` (tab: "Keys", parent: "System", order: 20).

### API

| Method | Signature | Purpose |
|---|---|---|
| `begin()` | `void begin(DeviceCrypto* crypto)` | Injects the `DeviceCrypto` dependency for encryption/decryption. |
| `addKey()` | `bool addKey(const char* id, const char* secret, const char* managedBy = nullptr, bool exclusive = false)` | Encrypts and stores a credential with optional ownership metadata. |
| `getKeyById()` | `std::string getKeyById(const char* id) const` | Retrieves and decrypts a credential. Returns empty string if not found. |
| `deleteKey()` | `bool deleteKey(const char* id)` | Removes a credential from the registry. |
| `listKeys()` | `std::vector<std::string> listKeys() const` | Returns a list of stored key IDs (never plaintext values). |
| `listKeysWithMetadata()` | `std::vector<KeyInfo> listKeysWithMetadata() const` | Returns `KeyInfo` structs with `id`, `hasValue`, `managedBy`, `exclusive` fields. |
| `isExclusive()` | `bool isExclusive(const char* id) const` | Checks whether a key is exclusively owned by a module. |

### Ownership Metadata

Each key entry carries optional ownership attributes via the `KeyInfo` struct:
- **`managedBy`** — The owning module name (e.g., `"WiFi"`, `"Security"`). Empty for user-created keys.
- **`exclusive`** — When `true`, the key is write-protected: it cannot be modified or deleted via the REST API, only by the owning module.

### Rules
- **Never store plaintext credentials** in LittleFS config files. Always use `KeyManager.addKey()`.
- WiFi passwords are stored via `KeyManager`, not via `IConfigurable`. `WifiManager` retrieves them via `KeyManager` during `applyConfig()`.
- The REST API (`GET /api/keys`) never returns plaintext key values — only key IDs and metadata.
- Keys with `exclusive = true` cannot be modified or deleted via the REST API — only by the owning module.

## NVS vs LittleFS

| Storage | Use For | Security |
|---|---|---|
| NVS | Crypto master key (`DeviceCrypto`), `SECURITY_VERSION` counter | Hardware-level, survives LittleFS format |
| LittleFS (`/etc/keys.json`) | Encrypted credential blobs (`KeyManager` registry) | Encrypted via DeviceCrypto AES-256-CBC |
| LittleFS (`/mod/*.json`) | Module config (non-sensitive) | Plaintext — no credentials here |

## See Also
- [Security Overview](overview.md) — Layered security model
- [Web Portal Authentication](web-portal-auth.md) — How AuthManager uses KeyManager for password hashes
- [OTA Security](ota-security.md) — Public key storage for firmware verification
