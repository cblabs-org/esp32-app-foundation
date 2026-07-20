---
name: pr-42-wo1
description: Handoff instructions for Work Order 1 — Paced discovery/state publishing
complexity: standard
---

# WO1 — Paced Discovery/State Publishing

## Objective

Replace the synchronous `publishDiscovery()` tight loop with a tick-driven chunked state machine. Add return-value checking, outbox limit, drain-completion signal, and `addEntity()` tripwire.

## Files Modified

- `modules/HomeAssistant/HomeAssistantModule.hpp` — add state members, public getter, remove old declarations
- `modules/HomeAssistant/HomeAssistantModule.cpp` — rewrite tick(), update event handler, addEntity, republishDiscovery, connectMqtt
- `sdkconfig.defaults` — evaluate MQTT task stack size (optional)

## Checklist

- [x] Add paced-publishing state members to header
- [x] Add `isDiscoveryDrainActive()` public getter
- [x] Remove `_needsStateSync` atomic, `publishDiscovery()`, `publishAllStates()` declarations
- [x] Rewrite MQTT_EVENT_CONNECTED handler to set flags
- [x] Rewrite `tick()` as two-phase state machine
- [x] Add `ESP_LOGW` tripwire in `addEntity()`
- [x] Update `republishDiscovery()` to use paced mechanism
- [x] Add `outbox.limit = 8192` in `connectMqtt()`
- [x] Remove `publishDiscovery()` and `publishAllStates()` implementations
- [x] Build verification
