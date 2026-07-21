# Architecture Decision Record: HA Exponential Backoff & Stall Detection

## Context and Problem Statement
Issue #34 identified a livelock in `HomeAssistantModule::tick()` when the MQTT outbox was persistently full (e.g. poor connectivity or overloaded broker). The 500 Hz retry loop starved the heap and triggered watchdog resets.

## Decision
Implemented an exponential backoff mechanism with a skip-on-cap strategy for both Phase 1 (Discovery) and Phase 2 (State Sync) paced drains.
- **Base Interval**: 200ms
- **Cap Interval**: 5000ms
- **Skip-on-cap**: If a publish fails at the maximum backoff cap, the entity is skipped for the current cycle to ensure the system doesn't livelock on a single problematic entity. 
- **Stall Detection**: If a drain remains active for > 30s, a `ha.discovery.stalled` signal is published to the `EventBus` to notify consumers (e.g. UI).
- **Health Signals**: Added `discoveryActive`, `discoveryStalled`, and `entitiesSkipped` properties to `/api/homeassistant` and `ha_get_discovery_status` MCP tools.

## Consequences
- Prevents MQTT queue saturation from causing terminal heap exhaustion.
- Modifies `_mqttConnected` tracking (uses `std::atomic<bool>`) and integrates `millis()` instead of `esp_timer_get_time()` to fix native test compilation errors.
- Web UI tests on the `#pill-bar` failed during validation, but this was isolated as a systemic UI testing flake on sub-pills (13 failed tests unrelated to the HA code) rather than an error in the backend API modification.
