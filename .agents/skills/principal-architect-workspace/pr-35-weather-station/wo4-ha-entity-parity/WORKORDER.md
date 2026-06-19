# Work Order 4: Home Assistant Entity Parity

**Objective:** Expand the `HomeAssistantModule` entity catalog to achieve full feature parity with the `arduino_ha` library.

**Strict Constraints:**
- **Hierarchical Object Model (HOM):** All entities must inherit from `IHaEntity` and correctly encapsulate their domain-specific Discovery JSON generation and state routing.
- **Memory Safety:** Optimize all `const char*` storage (avoiding `String` usage where possible) to prevent heap fragmentation across dozens of entities.

**Tasks:**
1. Implement `HaBinarySensor` (boolean state telemetry).
2. Implement `HaButton` (triggering stateless actions from HA).
3. Implement `HaLight` (supporting brightness, RGB/HSV color, and effect state routing).
4. Implement `HaFan` (speed and oscillation state routing).
5. Implement `HaClimate` (target temperature, current temperature, and HVAC mode routing).
6. Implement `HaCover` (position tracking and open/close/stop command routing).
7. Implement `HaNumber` and `HaSelect` (for arbitrary slider and dropdown configurations).
8. Implement `HaDeviceTrigger` (for stateless device-level events).
9. Add exhaustive native unit tests (`test/test_native/`) for every entity class to verify their Discovery JSON outputs strictly match the Home Assistant MQTT Discovery schema specifications.
