---
name: docs-excellence-wo4
description: Handoff instructions for Work Order 4 — HomeAssistant module + publicKey.hpp full house-style documentation
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Add complete house-style documentation to the HomeAssistant module (the one framework module that currently has zero documentation) and to `include/publicKey.hpp`. This is the most critical framework documentation gap.

# Strict Boundary Box

- **ONLY** modify files under `modules/HomeAssistant/` and `include/publicKey.hpp`
- Do NOT touch any other framework modules, sample-app, or template-app
- Do NOT modify any logic or behavior — documentation only
- Do NOT create git branches or make commits

# Tooling Constraints

- Use `platformio` MCP tools for builds: `build_project`
- Build the main firmware to verify changes compile

# House-Style Reference

Read `.agents/skills/house-style-docs/references/cpp-embedded.md` BEFORE starting.

# Context

The HomeAssistant module implements 4 interfaces: ITickable, IConfigurable, IWebContributor, IMCPContributor. It provides ESP-MQTT integration for Home Assistant auto-discovery. Read the module code thoroughly before writing documentation — accuracy is critical.

# Line-Item Instructions

## 1. HomeAssistantModule.hpp

- [ ] Add complete house-style header block:
  ```
  /*
   * ESP32 App Foundation (c) 2026 CB Labs
   *
   * https://github.com/matthewmcneill/esp32-app-foundation
   *
   * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
   * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
   *
   * Module: modules/HomeAssistant/HomeAssistantModule.hpp
   * Description: [Read the code and write an accurate multi-line description]
   *
   * Exported Classes:
   * - HomeAssistantModule: [description]
   *
   *   Lifecycle:
   *   - setKeyManager():     ...
   *   - setConfigManager():  ...
   *   - setEventBus():       ...
   *   - addEntity():         ...
   *   - connectMqtt():       ...
   *
   *   ITickable:
   *   - tick():              ...
   *
   *   IConfigurable ("homeassistant"):
   *   - getConfigNamespace(): ...
   *   - [list ALL IConfigurable methods]
   *
   *   IWebContributor:
   *   - [list ALL IWebContributor methods]
   *
   *   IMCPContributor:
   *   - registerMCPTools():  ...
   *
   *   State & Diagnostics:
   *   - publishState():      ...
   *   - isMqttConnected():   ...
   */
  ```
- [ ] Add class-level `/** @brief ... */` Doxygen above the class declaration
- [ ] Add `/** @brief ... */` to every public method declaration
- [ ] Add inline documentation comments (`///<`) to all private member variables
- [ ] Label interface sections with `// ITickable`, `// IConfigurable`, etc.

## 2. HomeAssistantModule.cpp

- [ ] Add complete house-style header block (same attribution format)
- [ ] Add Doxygen `@brief`/`@param`/`@return` to all method implementations
- [ ] Add `// --- Step N: Description ---` block comments to any methods longer than ~15 lines (likely: `connectMqtt()`, `publishDiscovery()`, `registerRoutes()`, `tick()`)

## 3. IHaEntity.hpp (entities/IHaEntity.hpp)

- [ ] Add complete house-style header block
- [ ] Convert all inline `//` comments on interface methods to proper Doxygen:
  ```cpp
  /** @brief Returns the entity's unique ID. */
  virtual const char* getId() const = 0;
  ```
- [ ] Add `@brief` to the interface class itself

## 4. All 11 entity files in entities/

For each of these files, add a house-style header block and Doxygen to all public methods:
- [ ] `HaBinarySensor.hpp`
- [ ] `HaButton.hpp`
- [ ] `HaClimate.hpp`
- [ ] `HaCover.hpp`
- [ ] `HaDeviceTrigger.hpp`
- [ ] `HaFan.hpp`
- [ ] `HaLight.hpp`
- [ ] `HaNumber.hpp`
- [ ] `HaSelect.hpp`
- [ ] `HaSensor.hpp`
- [ ] `HaSwitch.hpp`

Each entity file header should follow this pattern:
```
/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/HomeAssistant/entities/[FileName].hpp
 * Description: [Accurate description of what this HA entity type does]
 *
 * Exported Classes:
 * - [ClassName]: [description]
 *   - [list all public methods]
 */
```

## 5. include/publicKey.hpp

- [ ] Add house-style header block:
  ```
  /*
   * ESP32 App Foundation (c) 2026 CB Labs
   *
   * https://github.com/matthewmcneill/esp32-app-foundation
   *
   * Module: include/publicKey.hpp
   * Description: Auto-generated RSA-2048 public key for OTA firmware signature
   *              verification. Generated from public_key.pem via xxd. Used by
   *              the OTA verifier to validate firmware authenticity before applying
   *              updates.
   *
   * Exported Variables:
   * - public_key_pem:     PEM-encoded RSA public key as byte array
   * - public_key_pem_len: Length of the public key data in bytes
   */
  ```
- [ ] Add inline comments to the two variables:
  ```cpp
  unsigned char public_key_pem[] = { ... }; ///< PEM-encoded RSA-2048 OTA signing public key
  unsigned int public_key_pem_len = 451;    ///< Length of public_key_pem in bytes
  ```

# Verification

- [ ] Build the main firmware: `pio run -e esp32dev` — must succeed
- [ ] Build native tests if they exist: `pio test -e native` — must pass
- [ ] Visually verify all modified files have complete house-style headers and Doxygen
- [ ] **STOP and ask the user if they are happy before distilling**
