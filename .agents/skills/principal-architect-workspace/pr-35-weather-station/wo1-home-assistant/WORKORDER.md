# Work Order 1: Home Assistant Core Module

**Objective:** Build the `HomeAssistantModule` as a core framework component wrapping `esp-mqtt`.

**Strict Constraints:**
- **10ms Tick Budget:** `tick()` must remain strictly non-blocking. Offload all MQTT and TLS handling to `esp-mqtt` background tasks.
- **Hierarchical Object Model (HOM):** Implement `IHaEntity` and `HaSensor` classes. Ensure entities are explicitly registered via `registerEntity()`. Use the Domain Meaning Test.
- **Memory Safety:** Use dynamically chunked JSON payloads with `ArduinoJson` to avoid 256-byte truncation limits.
- **TLS Configuration:** Use the `KeyManager` to securely store and retrieve Pinned CA Certificates.

**Tasks:**
1. Create `modules/HomeAssistant/HomeAssistantModule.hpp` and `.cpp`.
2. Implement `ITickable`, `IConfigurable`, `IWebContributor`, `IMCPContributor`.
3. Build the `IHaEntity`, `HaSensor`, and `HaSwitch` concrete classes inside `modules/HomeAssistant/entities/`. (Implementing `HaSwitch` ensures we prove out bidirectional inbound MQTT command routing).
4. Integrate with `EventBus` for `wifi.connected`/`disconnected` events.
5. Generate Home Assistant Discovery JSON payloads, prefixing `unique_id` with the MAC address to avoid collisions.
6. Configure the Last Will and Testament (LWT) topic.
7. Add the "Home Assistant" tab to the web portal with connection settings and TLS cert upload pills.
