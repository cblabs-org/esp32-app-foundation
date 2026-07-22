# ADR: PA Session Design Decisions

## Q1 — Work Order Ordering
**Decision:** All 4 WOs execute as a serialized stack (WO1→WO2→WO3→WO4), no special prioritization.
**Rationale:** User preference — "We will do them all together." WO1 and WO2 both touch `app.js` so must serialize regardless. Stacking keeps the review clean.

## Q2 — `-mfix-esp32-psram-cache-issue` Flag
**Decision:** Remove entirely from all S3 environments (framework, sample-app, template-app).
**Rationale:** This is an ESP32-classic rev 0/1 hardware bug workaround. It is inert on the S3. User confirmed: "remove it if not needed, reduce technical debt."

## Q3 — PSRAM Heap Split on System Page
**Decision:** No change needed — already implemented.
**Rationale:** `system.js` already shows Internal DRAM and PSRAM as separate sections (lines 199–202). When PSRAM is silently disabled, the PSRAM section doesn't appear because `psram_total == 0`. The runtime guard from WO3 makes this absence explained rather than silent.

## PSRAM Guard Design — Dual Layer
**Decision:** Both compile-time `#error` AND runtime `ESP_LOGE` check.
**Rationale:** Compile-time catches 90% case (missing `CONFIG_SPIRAM`). Runtime catches 10% case (config present but PSRAM physically failed to initialize). User agreed with dual approach.

## ArduinoJson Doc Remediation — Reframing Rationale
**Decision:** Reframe allocation-bounding from "use `StaticJsonDocument<N>`" to "bounded by `getMaxPostBody()` middleware".
**Rationale:** ArduinoJson 7's `JsonDocument` is elastic — the old "fixed-size document = bounded memory" argument no longer applies. The correct bounding mechanism is the body-size middleware that caps input before parsing.
