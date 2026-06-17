---
name: pr28-wo10
description: Handoff instructions for Work Order 10 — enable NVS encryption
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

# Objective

Enable NVS encryption via `CONFIG_NVS_ENCRYPTION` in sdkconfig. This protects
the `DeviceCrypto` master key (the key-encryption-key for KeyManager's LittleFS
credential store) at rest in the NVS partition.

# Strict Boundary Box

- **DO** modify only `sdkconfig.defaults` (and any board-specific sdkconfig
  delta files if they exist).
- **DO NOT** modify any `.cpp` or `.hpp` files.
- **DO NOT** modify `platformio.ini`.

# Background

- The NVS partition stores only the DeviceCrypto master key — a single 32-byte
  AES-256 key used to encrypt all credentials in the KeyManager's LittleFS store.
- The partition is small and a factory reset already wipes both NVS and LittleFS.
- NVS encryption uses ESP-IDF's flash encryption key derivation. On first boot
  after enabling, the NVS partition will be reformatted with encryption enabled.
- **This change requires a factory reset** — the device will not be able to read
  existing NVS data written without encryption.

# Checklist

- [ ] **Research:** Check current sdkconfig.defaults for any existing NVS or
  flash encryption settings:
  ```bash
  grep -n "NVS\|FLASH_ENC\|SECURE_BOOT" sdkconfig.defaults
  ```

- [ ] **Research:** Check ESP-IDF documentation for `CONFIG_NVS_ENCRYPTION`
  requirements:
  - Does it require `CONFIG_PARTITION_TABLE_CUSTOM=y`?
  - Does it require a specific NVS partition type?
  - Does it require flash encryption to be enabled first, or can NVS encryption
    work independently?
  - What is `CONFIG_NVS_SEC_KEY_PROTECT_USING_FLASH_ENC` vs
    `CONFIG_NVS_SEC_KEY_PROTECT_USING_HMAC`?

- [ ] **Evaluate:** Determine the simplest path to NVS encryption:
  - If NVS encryption can work with HMAC-based key protection (no full flash
    encryption needed), prefer that path.
  - If full flash encryption is required, document the implications and proceed
    only if the sdkconfig changes are self-contained.

- [ ] **`sdkconfig.defaults`** — Add the appropriate NVS encryption settings.
  Example (adjust based on research):
  ```ini
  # NVS Encryption — protects DeviceCrypto master key at rest (H-05)
  CONFIG_NVS_ENCRYPTION=y
  CONFIG_NVS_SEC_KEY_PROTECT_USING_HMAC=y
  ```

- [ ] **Check board-specific delta files:** If `sdkconfig.esp32dev` or
  `sdkconfig.esp32s3nano` exist, verify they don't override NVS settings.

- [ ] **Document:** Add a comment block in `sdkconfig.defaults` explaining:
  - What NVS encryption protects (the DeviceCrypto master key)
  - That enabling this requires a factory reset on existing devices
  - The key protection mechanism chosen and why

# Important Notes

- **This WO runs LAST** because enabling NVS encryption requires a factory
  reset. The device will lose all stored credentials and configuration.
  All prior WOs must be flash-verified before this change.
- After this change, the device will need to go through the WiFi setup wizard
  again and regenerate all keys.
- Do NOT attempt to flash and verify this change on the attached device unless
  you are prepared for a full factory reset. Build verification only.

# Verification

Use the PlatformIO MCP server tools (not the CLI directly):
1. `build_project` with `environment: "native"` — compiles cleanly
2. `build_project` with `environment: "esp32dev"` — compiles cleanly
3. `build_project` with `environment: "esp32s3nano"` — compiles cleanly

**No flash verification** — this WO intentionally skips flash testing because
it triggers a factory reset.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are
happy. Only after approval should you invoke the `@distillery` skill.
