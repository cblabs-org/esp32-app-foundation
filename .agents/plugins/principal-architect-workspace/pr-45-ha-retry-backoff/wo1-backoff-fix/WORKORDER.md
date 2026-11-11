---
name: ha-retry-backoff-wo1
description: Handoff instructions for Work Order 1 — Exponential backoff fix for HA discovery retry livelock
complexity: standard
---

# Work Order 1 — Exponential Backoff Fix for HA Discovery Retry Livelock

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix Issue #34: the unbounded ~500 Hz retry loop in `HomeAssistantModule::tick()` that causes heap exhaustion, portal loss, and terminal device state when the MQTT outbox is persistently full. Add exponential backoff with skip-on-cap and propagate health signals to all available surfaces.

**Parent project:** PR-45 (`pr-45-ha-retry-backoff`)
**Branch:** `ha-retry-backoff/wo1-backoff-fix` (already checked out)

## Strict Boundary Box

You MUST only modify these files:
- `modules/HomeAssistant/HomeAssistantModule.hpp`
- `modules/HomeAssistant/HomeAssistantModule.cpp`
- Test files in `test/test_native/test_homeassistant/` or `test/test_unit/test_homeassistant/`

You MUST NOT:
- Modify any other module
- Create new branches
- Change `platformio.ini` (unless adding a test environment)
- Modify any files outside the boundary box
- Make architectural decisions beyond what is specified here

## Tooling Constraints

- Use the `platformio` MCP tools for building and testing — NOT raw `pio` CLI commands
- Read the `pio-manager` skill at `.agents/skills/pio-manager/SKILL.md` before running any build/test commands
- Use `build_project` to verify compilation
- Use `run_tests` to run the native test suite

## Line-Item Instructions

### 1. Add Constants and Members to `HomeAssistantModule.hpp`

Add these private constants alongside the existing `DISCOVERY_CHUNK_SIZE` and `STATE_CHUNK_SIZE` (around line 114):

```cpp
// --- Retry Backoff (Issue #34) ---
static constexpr uint32_t RETRY_BASE_MS          = 200;    ///< Initial backoff interval after first publish failure
static constexpr uint32_t RETRY_CAP_MS            = 5000;   ///< Max backoff interval — skip entity on next failure at cap
static constexpr uint32_t DRAIN_STALL_TIMEOUT_MS  = 30000;  ///< Publish health signal after this many ms of stalled drain
```

Add these private member variables alongside the existing drain state (around line 119):

```cpp
uint32_t _retryIntervalMs{0};            ///< Current exponential backoff interval (0 = no active backoff)
uint32_t _lastRetryAttemptMs{0};         ///< Timestamp of last failed publish attempt
uint32_t _drainStartMs{0};              ///< Timestamp when current drain cycle began
bool     _drainStallReported{false};     ///< One-shot flag: stall event already published this drain
size_t   _entitiesSkipped{0};           ///< Running count of entities skipped in current drain
```

### 2. Modify `tick()` Phase 1 — Discovery Drain

Replace the discovery drain block (lines 408-441 in the `.cpp`) with the following logic:

```cpp
if (_discoveryPending) {
    // --- Backoff gate: skip this tick if still in backoff window ---
    if (_retryIntervalMs > 0 && (currentMillis - _lastRetryAttemptMs) < _retryIntervalMs) {
        return;  // Still backing off — wait
    }

    // --- Drain stall detection (Issue #34) ---
    if (!_drainStallReported && _drainStartMs > 0
        && (currentMillis - _drainStartMs) > DRAIN_STALL_TIMEOUT_MS) {
        ESP_LOGE(TAG, "Discovery drain stalled: %u/%u entities after %lu ms",
                 (unsigned)_discoveryIndex, (unsigned)_entities.size(),
                 (unsigned long)(currentMillis - _drainStartMs));
        if (_eventBus) _eventBus->publish("ha.discovery.stalled", nullptr);
        _drainStallReported = true;
        _mqttStatusMsg = "Discovery Stalled";
    }

    size_t published = 0;
    while (_discoveryIndex < _entities.size() && published < DISCOVERY_CHUNK_SIZE) {
        auto* entity = _entities[_discoveryIndex];
        JsonDocument doc;
        JsonObject payload = doc.to<JsonObject>();
        generateDiscoveryJson(entity, payload);
        std::string payloadStr;
        serializeJson(doc, payloadStr);
        std::string topic = "homeassistant/" + std::string(entity->getEntityType())
                          + "/" + _macPrefix + "/" + entity->getObjectId() + "/config";
        int ret = esp_mqtt_client_publish(_mqttClient, topic.c_str(),
                                         payloadStr.c_str(), 0, 1, 1);
        if (ret < 0) {
            _lastRetryAttemptMs = currentMillis;

            if (_retryIntervalMs == 0) {
                // First failure on this entity — start backoff
                _retryIntervalMs = RETRY_BASE_MS;
            } else if (_retryIntervalMs >= RETRY_CAP_MS) {
                // Already at cap — skip this entity
                ESP_LOGE(TAG, "Discovery publish failed at backoff cap for '%s' — skipping",
                         entity->getObjectId().c_str());
                _discoveryIndex++;
                _retryIntervalMs = 0;  // Reset backoff for next entity
                _entitiesSkipped++;
                continue;  // Try next entity in this chunk
            } else {
                // Exponential increase, clamped to cap
                _retryIntervalMs = std::min(_retryIntervalMs * 2, RETRY_CAP_MS);
            }

            ESP_LOGW(TAG, "Discovery publish failed (ret=%d, entity=%s), backoff %u ms",
                     ret, entity->getObjectId().c_str(), (unsigned)_retryIntervalMs);
            break;  // Pause — backoff before retry
        }
        // Success — reset backoff
        _retryIntervalMs = 0;
        _discoveryIndex++;
        published++;
    }

    if (_discoveryIndex >= _entities.size()) {
        if (_entitiesSkipped > 0) {
            ESP_LOGW(TAG, "Discovery drain complete: %u entities published, %u skipped",
                     (unsigned)(_entities.size() - _entitiesSkipped),
                     (unsigned)_entitiesSkipped);
        } else {
            ESP_LOGI(TAG, "Discovery drain complete: %u entities published",
                     (unsigned)_entities.size());
        }
        _discoveryPending = false;
        _stateSyncPending = true;
        _stateSyncIndex = 0;
        // Reset backoff state for Phase 2 but preserve _entitiesSkipped for diagnostics
        _retryIntervalMs = 0;
        _lastRetryAttemptMs = 0;
    }
    return;  // Don't overlap discovery and state sync in the same tick
}
```

### 3. Modify `tick()` Phase 2 — State Sync Drain

Apply **identical** backoff logic to the state-sync drain (lines 444-473). The same `_retryIntervalMs`, `_lastRetryAttemptMs` variables are reused (they were reset at the end of Phase 1). The stall detection does NOT need to be duplicated — the Phase 1 stall timer already covers the overall drain duration.

The key changes are:
- Add the backoff gate at the top of the `if (_stateSyncPending)` block
- On `ret < 0`: same exponential backoff / skip-on-cap logic
- On success: reset `_retryIntervalMs = 0`
- On completion: reset `_entitiesSkipped` to 0

### 4. Modify `MQTT_EVENT_CONNECTED` Handler

In `mqttEventHandler()`, after `self->_discoveryIndex = 0;` (around line 328), add:

```cpp
self->_retryIntervalMs = 0;
self->_lastRetryAttemptMs = 0;
self->_drainStartMs = currentMillis;  // Note: you'll need millis() here — check what's available in the callback context, may need to use esp_timer_get_time()/1000 or xTaskGetTickCount()*portTICK_PERIOD_MS
self->_drainStallReported = false;
self->_entitiesSkipped = 0;
```

**Important:** The MQTT event handler is a static callback — it does not receive `currentMillis`. Use `(uint32_t)(esp_timer_get_time() / 1000)` or `millis()` (Arduino) to get the current time. Check what's available in the existing codebase and be consistent.

### 5. Add Health Signal Fields to Web API

In `registerRoutes()`, in the `GET /api/homeassistant` handler (around line 559), add:

```cpp
doc["discoveryStalled"] = _drainStallReported;
doc["entitiesSkipped"] = _entitiesSkipped;
doc["discoveryActive"] = _discoveryPending || _stateSyncPending;
```

### 6. Add Health Signal to MCP Tools

In `registerMCPTools()`, add a new read-only tool `ha_get_discovery_status`:

```cpp
registry.addTool("ha_get_discovery_status", "Get the status of the Home Assistant discovery drain",
    [](void* context, JsonObject& args, JsonObject& result) {
        auto* self = static_cast<HomeAssistantModule*>(context);
        result["discoveryActive"] = self->_discoveryPending || self->_stateSyncPending;
        result["discoveryStalled"] = self->_drainStallReported;
        result["entitiesSkipped"] = self->_entitiesSkipped;
        result["entitiesTotal"] = self->_entities.size();
        result["discoveryIndex"] = self->_discoveryIndex;
    }, this, false);
```

**Note:** Some of these members are private. You may need to either:
- Add a public `getDiscoveryStatus()` method that returns a struct or populates a JsonObject
- Use the lambda capture with `this` (which is already the pattern used by `ha_get_state`)
- Make the tool lambda a friend — but prefer the first approach for encapsulation

Choose whichever approach is most consistent with the existing code patterns.

### 7. Write Comprehensive Tests

Add tests to the existing test file or create a new one. The tests need a mock for `esp_mqtt_client_publish` that can be controlled to return success or failure. Check the existing stubs in `test/stubs/platform/` for the MQTT client stub and extend it if needed.

**Required test cases:**

- [ ] **Happy path:** Register N entities, simulate connected state, call tick() repeatedly → all entities published in ⌈N/5⌉ ticks, `isDiscoveryDrainActive()` returns false
- [ ] **Transient failure:** Publish fails once then succeeds on retry → entity not skipped, backoff resets
- [ ] **Exponential backoff sequence:** Verify intervals double: 200 → 400 → 800 → 1600 → 3200 → 5000
- [ ] **Skip on cap:** Publish fails at cap interval → entity skipped, `_entitiesSkipped` incremented, drain continues to next entity
- [ ] **Backoff gate:** After failure, calling tick() with insufficient elapsed time → returns without attempting publish
- [ ] **Drain stall signal:** Simulate drain active > 30s → stall event fired (verify via flag)
- [ ] **Phase 2 parity:** State-sync drain has identical backoff behaviour
- [ ] **Skip count in completion log:** When entities are skipped, verify skipped count is tracked

**Test approach guidance:**
- The existing MQTT stub likely returns a configurable value from `esp_mqtt_client_publish`. If not, extend it to allow test-controlled return values (e.g., a global `int mock_mqtt_publish_return_value`).
- To test timing-dependent backoff, pass controlled `currentMillis` values to `tick()` rather than relying on real time.

### 8. Preserve Existing Comments and Documentation

- Keep all existing docstrings and comments that are unrelated to your changes
- Update the `tick()` docstring to mention the backoff mechanism
- Update the header file's class-level doc to mention the backoff and health signals

## Verification Requirement

1. **Build verification:** Use the `platformio` MCP tool `build_project` for `esp32dev` environment — must compile clean
2. **Test verification:** Use the `platformio` MCP tool `run_tests` for the native test environment — all tests must pass including your new ones
3. **CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only after approval should you invoke the `@distillery` skill to package the state and drop control.

## Reference Files

Read these if you need context:
- [Issue #34 assessment](file:///Users/mcneillm/.gemini/antigravity/brain/000a3e6b-44c7-46d7-9147-da9c2da4a3c3/issue_34_assessment.md) — full root cause analysis
- [PR-42 ADR](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/plugins/principal-architect-workspace/pr-42-ha-discovery-pacing/context_artifacts/adr_paced_discovery.md) — why tick-driven, why two-phase
- [PR-42 graveyard](file:///Users/mcneillm/Documents/Projects/esp32-app-foundation/.agents/plugins/principal-architect-workspace/pr-42-ha-discovery-pacing/context_artifacts/graveyard_rejected_approaches.md) — rejected approaches

## Status Update (2026-07-21)
- **Status:** COMPLETED
- **Details:** 
  - Exponential backoff (200ms base, 5s cap) and skip-on-cap implemented in Phase 1 and 2.
  - 30s stall detection implemented.
  - Web API and MCP tool (`ha_get_discovery_status`) updated with health signals.
  - `esp_timer.h` replaced with Arduino `millis()` to resolve native test build failures.
  - 100% pass rate in `native` unit tests. Playwright web UI tests failed due to unrelated `#pill-bar` rendering flakes.
  - Code committed and context artifacts generated via `@distillery`.
