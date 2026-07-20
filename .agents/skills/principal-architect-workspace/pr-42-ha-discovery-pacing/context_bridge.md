# PR-42 — Issue #16: HA Discovery Pacing — Distillation

## One-line summary
Replaced synchronous `publishDiscovery()` tight loop with tick-driven chunked state machine to prevent esp-mqtt outbox saturation at scale (197+ entities).

## Commit
`866353f` on branch `issue-16-discovery-pacing/wo1-paced-publish`
GitHub PR: [#17](https://github.com/matthewmcneill/esp32-app-foundation/pull/17)
Closes: [#16](https://github.com/matthewmcneill/esp32-app-foundation/issues/16)

## Root Cause (5-factor compound failure)
1. **Unthrottled tight loop** in `publishDiscovery()` — N consecutive QoS-1 publishes with no pacing
2. **Runs on MQTT task context** — blocks MQTT task from draining its own outbox
3. **No return-value checking** — `-2` (outbox full) silently ignored
4. **Double burst** — `publishAllStates()` fires immediately after discovery
5. **No outbox limit** — unbounded heap growth until OOM (2–4 KB free at 197 entities)

## Key Design Decisions

### D-1: Tick-driven state machine over synchronous retry
Publishing was moved from the MQTT event handler callback (MQTT task context) into `tick()` (main loop context). This allows the MQTT task to drain its outbox between ticks. The alternative (back-pressure via return-value in the callback) was rejected because the MQTT task cannot drain its own outbox while blocked in our callback.

### D-2: Two-phase serialised drain
Discovery completes fully before state sync begins. This ensures HA has all entity definitions before receiving state values. States for not-yet-discovered entities are harmlessly dropped by HA and self-heal at the next reporting interval (≤30 s).

### D-3: Chunk sizes as compile-time constants
`DISCOVERY_CHUNK_SIZE=5`, `STATE_CHUNK_SIZE=10`. Conservative for the 10ms tick budget. Application team explicitly requested no runtime config surface.

### D-4: EventBus drain-completion signal
`"ha.discovery.complete"` published when both phases finish. Application team requested this for their verification framework (podCount=8 acceptance testing).

### D-5: Outbox limit as defence-in-depth
`outbox.limit = 8192` (8KB) set in `esp_mqtt_client_config_t`. Not the primary fix — the pacing alone would suffice — but prevents unbounded heap growth during disconnection scenarios.

## Files Changed
| File | Change |
|---|---|
| `modules/HomeAssistant/HomeAssistantModule.hpp` | +chunk constants, +drain indices, +`isDiscoveryDrainActive()`, −`_needsStateSync`, −`publishDiscovery()`/`publishAllStates()` decls |
| `modules/HomeAssistant/HomeAssistantModule.cpp` | Rewrite `tick()` as 2-phase state machine, flag-based MQTT connect handler, paced `republishDiscovery()`, `addEntity()` drain warning, `outbox.limit`, drain-cancel on disconnect |
| `test/stubs/platform/mqtt_client.h` | +`outbox.limit` member in config stub |
| `.agents/skills/principal-architect-workspace/MEMORY.md` | +PR-42 registry entries |
| `.agents/skills/principal-architect-workspace/pr-42-ha-discovery-pacing/` | PROJECT.md + WORKORDER.md |

## Rejected Approaches
- **Option B (back-pressure only):** Insufficient because `publishDiscovery()` runs on the MQTT task's own stack — the task cannot drain its outbox while blocked in our callback. Creates a deadlock-like scenario.
- **Option C (increase outbox size):** Only moves the cliff. At some entity count it will still fail. Does not address root cause.

## Gotchas for Future Work
- `getObjectId()` returns `std::string`, not `const char*` — use `.c_str()` in variadic `ESP_LOG*` macros
- `outbox` member not in original mqtt_client.h test stub — had to extend the stub
- `addEntity()` during active drain is warned but not prevented — matches the compact-and-lock architecture (entities registered at boot, never after MQTT connects)

## Verification
- esp32dev build: SUCCESS (932KB flash, 54KB RAM)
- Native tests: 361/361 passed
- Web tests: all passed
- Consumer verification (podCount=8, 197 entities) pending on power-monitor team (OQ-14)
