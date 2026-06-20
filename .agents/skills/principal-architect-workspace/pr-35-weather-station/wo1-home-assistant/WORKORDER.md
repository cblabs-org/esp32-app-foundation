# Work Order 1: Home Assistant Core Module

**Objective:** Build the `HomeAssistantModule` as a core framework component wrapping `esp-mqtt`.

**Strict Constraints:**
- **10ms Tick Budget:** `tick()` must remain strictly non-blocking. Offload all MQTT and TLS handling to `esp-mqtt` background tasks.
- **Hierarchical Object Model (HOM):** Implement `IHaEntity` and concrete entity classes. Ensure entities are explicitly registered via `registerEntity()`. Use the Domain Meaning Test.
- **Dependency Injection:** Applications wire the module via `setHomeAssistant(&haModule)`, not a framework getter. Matches existing patterns (e.g., `DiagnosticsBoard.configure(WifiManager*)`).
- **Memory Safety:** Use dynamically chunked JSON payloads with `ArduinoJson` to avoid 256-byte truncation limits.
- **TLS Configuration:** Use the `KeyManager` to securely store and retrieve Pinned CA Certificates for self-signed local HA servers.
- **Per-Entity State Topics:** Each entity gets its own `state_topic` (e.g., `homeassistant/sensor/aabbccddeeff/outside_temp/state`).
- **Unique IDs:** Full compact MAC prefix (`aabbccddeeff_entity_id`). Characters restricted to `[a-zA-Z0-9_-]`.

**Tasks:**
1. Create `modules/HomeAssistant/HomeAssistantModule.hpp` and `.cpp`.
2. Implement `ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`.
3. Build the `IHaEntity`, `HaSensor`, and `HaSwitch` concrete classes inside `modules/HomeAssistant/entities/`. (`HaSwitch` proves bidirectional inbound MQTT command routing.)
4. Integrate with `EventBus` for `wifi.connected`/`disconnected` events.
5. Generate Home Assistant Discovery JSON payloads with MAC-prefixed `unique_id` and per-entity `state_topic`.
6. Configure the shared Last Will and Testament (LWT) topic.
7. Implement post-reconnect state sync (re-publish all cached entity states on `MQTT_EVENT_CONNECTED`).
8. Add the "Home Assistant" top-level tab to the web portal with connection settings and TLS cert upload pills.
9. Implement `ha_get_state` and `ha_call_service` MCP tool handlers via `IMCPContributor`.
10. Add native unit tests for discovery JSON generation, MAC-prefixed unique IDs, LWT topic construction, and per-entity state topic formatting.
