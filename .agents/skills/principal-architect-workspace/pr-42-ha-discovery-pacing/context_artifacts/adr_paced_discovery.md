# ADR: Paced Discovery Publishing

## Context

`HomeAssistantModule::publishDiscovery()` ran as a synchronous tight loop inside the `MQTT_EVENT_CONNECTED` callback, which executes on the **MQTT task's stack**. At 197 entities, this queued ~197 QoS-1 messages into the esp-mqtt outbox without giving the MQTT task any opportunity to drain them, causing outbox saturation, silent message drops (~51%), and heap exhaustion (2–4 KB free vs. 50 KB TLS safety floor).

## Decision 1: Tick-Driven Over Synchronous Retry

**Chosen:** Move all publishing into `tick()` on the main loop, publishing bounded chunks per iteration.

**Why:** The MQTT event handler runs on the MQTT task's own stack. While our callback is executing, the MQTT task cannot process its outbox. Even with return-value checking and retry delays, we'd be spinning on the MQTT task — a deadlock-like scenario where we fill the queue and block the only thread that can drain it.

**Alternative rejected:** Option B (back-pressure in callback) — see graveyard.

## Decision 2: Two-Phase Serialised Drain

**Chosen:** Complete all discovery publishes (Phase 1) before starting state sync (Phase 2).

**Why:** Home Assistant drops state values for entities it hasn't seen a discovery config for yet. By completing discovery first, we ensure HA has all entity definitions before receiving state values. Any states published by consumers during the discovery phase are harmlessly dropped and self-heal at the next reporting interval (≤30 s for the power-monitor app).

## Decision 3: Compile-Time Chunk Constants

**Chosen:** `static constexpr size_t DISCOVERY_CHUNK_SIZE = 5` and `STATE_CHUNK_SIZE = 10`.

**Why:** Application team explicitly requested no runtime config surface — "these are framework internals, not user-facing tunables." Values are conservative for the 10ms tick budget (ADR-17). Discovery payloads are ~400 bytes each; 5 × 400 = 2 KB per tick is well within outbox headroom.

## Decision 4: Outbox Limit as Defence-in-Depth

**Chosen:** `mqtt_cfg.outbox.limit = 8192` (8 KB).

**Why:** Not the primary fix — tick pacing alone would prevent the burst. But the outbox limit provides a safety net during disconnection scenarios (messages queue up when the broker is unreachable). Without it, a device publishing state every 30 seconds during a 10-minute broker outage would accumulate ~160 KB of outbox, approaching OOM on a device with 320 KB total DRAM. The 8 KB limit bounds the worst case to ~20 messages.

## Decision 5: EventBus Drain-Completion Signal

**Chosen:** Publish `"ha.discovery.complete"` to the EventBus when both phases finish. Add `isDiscoveryDrainActive()` getter.

**Why:** Application team uses a podCount=8 acceptance test that verifies all 197 entities appear in Home Assistant. They need a signal to know when discovery is complete so they can start the verification timer. The getter provides synchronous polling; the EventBus event provides async notification.
