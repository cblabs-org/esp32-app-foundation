# Configuration Gate & Precondition Design — Rationale & Review Record

> ESP32 App Foundation (c) 2026 CB Labs
>
> Historical design record, motivation, consumer review history ([#146](https://github.com/matthewmcneill/esp32-app-foundation/issues/146)), and architectural rationale for the PreconditionEngine and application configuration gate subsystem.
>
> **Normative Specification:** The normative architecture, interfaces, and lifecycle integration are specified in [`docs/design_specification.md`](../design_specification.md) §8 (*Configuration Architecture*) and §9 (*Application Lifecycle*).

> **STATUS: IMPLEMENTED** — rationale and review record for the ConfigManager-as-Gateway
> design. Normative specification in [`docs/design_specification.md`](../design_specification.md) §8/§9.

---

## 1. Motivation

### 1.1 What triggered this

Reviewing PR [#145](https://github.com/matthewmcneill/esp32-app-foundation/pull/145), a consumer found that a data source backing a config gate could never satisfy that gate. The mechanism was specific — `DataManager::executeLoop()` returned early on `!_appRunning`, and gate regression clears `_appRunning` — but the shape was general:

> A gate blocks RUNNING. RUNNING enables the machinery. The machinery is what would satisfy the gate.

### 1.2 What consumer reviews established

The round-1 document proposed a richer gate status as its central change. Three independent reviews said the central problem is one level down, and two reached the same conclusion from unrelated domains:

> **departures-board-v3:** shallow graphs are a consequence of the contract, not the domain. Every edge today is boot-blocking, so consumers rationally refuse to stack them.

> **PowerMonitor:** yes, there is a chain we would build — and not one node of it belongs in the boot-blocking set. Richer status alone does not enable it.

Both described a chain they could not express, and both named the same reason: **the framework conflated "this must be true before we boot" with "this must be true for the device to work properly", and because only the first existed, consumers with the second either avoided the interface entirely or modeled nothing at all.**

| Observed Evasion / Tax | Evidence |
|---|---|
| A consumer decomposed `registerModule()` into five calls purely to *escape* becoming a gate | `departures-board-v3/src/main.cpp:142-148` — comment: *"skipping IConfigGate (API keys are optional — shouldn't block app from entering RUNNING)"* |
| A consumer with many preconditions authored **zero** gates, by written decision | `PowerMonitor/docs/…Software System Design.md:145` — *"Not used: … IConfigGate (device is useful with defaults out-of-the-box)"* |
| A consumer deferred a **wanted feature for a month** solely because its gate blocked boot | `PowerMonitor/.agents/…/pr-10-hw-bringup/PROJECT.md:53-56` — *"once the framework's gate no longer hard-blocks RUNNING on NTP"* |
| A gate was designed, then abandoned | `departures-board-v3/src/NationalRailModule.cpp:61-63` — fossil `// IConfigGate` heading |
| A consumer **complied and paid** rather than evading — wanting the *level* semantics a gate provides, and accepting the boot-block as a tax | `DraytonWiserController/include/wiser/WiserHubConfigGate.hpp:9-16` — *"unsatisfied from boot, so there is no transition to miss"* |

### 1.3 The Diagnosis: Two Conflations

1. **On the framework's side:** One predicate answered three different questions — *may we boot*, *should we be serving*, and *is this broken enough to tear down*. The three require different answers, different reversibility, and different operational costs.
2. **On the gate's side:** A gate computed *why* it was unsatisfied and the interface discarded it. `WifiConfigGate` distinguished three causes in `getActionStatus()` and then returned one fixed string from `getHelpText()` for all of them.

---

## 2. What Was Broken in the Legacy Gate Architecture

1. **Missing NTP clock chain link:** `TimeConfigGate::isSatisfied()` checked whether the user had configured a timezone string, not whether NTP was synchronized (`TimeSyncStatus::SYNCED`). A device with a timezone set and zero NTP sync satisfied all gates, entered RUNNING, and rendered garbage RTC time.
2. **Coarse enum forced blaming the user:** Legacy `GateActionStatus` offered only `Satisfied`, `InProgress`, and `NeedsUserAction`. Attentive authors were forced to return `NeedsUserAction` even for transient network blips because `InProgress` surfaced nothing to the user.
3. **Computed cause was discarded:** Causes were computed internally across multiple modules (`WifiConfigGate`, `WeatherModule`, `WiserHubConfigGate`) but collapsed into single, static help text strings. `WiserHubConfigGate` resorted to a `mutable char _helpText[96]` written inside a `const` method.
4. **Gates could not express "cannot evaluate":** When dependencies were missing, gates returned `true` ("don't block boot") to avoid falsely blaming the user. This masked developer wiring errors (`Indeterminate`) under the guise of intentional absence (`Unavailable`).
5. **Single tolerance constant for conflicting needs:** A single regression tolerance (default 30s) forced compromises between brief blips (which should stay on offline screens) and persistent outages (which require actionable modals).
6. **Unbounded `InProgress`:** No trial counts or escalation existed; connecting could loop forever without triggering a fallback.
7. **Failure was treated as terminal rather than fallback:** AP mode is a degraded, serviceable operating mode that keeps the device reachable, not a terminal error.
8. **Dependency edges were hard boot-blocks:** All edges were treated as hard `Requires`, preventing users in AP mode from configuring independent boards.
9. **Monotonicity assumption in UI:** Display code assumed gates never un-satisfied once met.
10. **High cost of gatehood:** Entering CONFIGURING suspended the display carousel and replaced the base screen, even if background sampling worked fine.

---

## 3. Prior Art & Influences

| System | Concept Adopted |
|---|---|
| **[Kubernetes probes](https://kubernetes.io/docs/concepts/workloads/pods/probes/)** | Split the question: Startup / Readiness / Liveness. Readiness failure is reversible and cheap; liveness failure is terminal. |
| **[systemd](https://www.freedesktop.org/software/systemd/man/latest/systemd.unit.html)** | Dependency strength belongs on edges: `Wants`, `Requires`, `BindsTo`. |
| **[Erlang/OTP](https://www.erlang.org/doc/system/sup_princ.html)** | Restart intensity and escalation to higher tiers. Damping windows must not multiply through chains. |
| **[Circuit breaker](https://learn.microsoft.com/en-us/azure/architecture/patterns/circuit-breaker)** | Half-open recovery probes after cooldowns. Fallback operating modes. |
| **[BGP flap damping](https://networklessons.com/bgp/bgp-route-dampening)** | Penalty accrual, decay half-life, and asymmetric suppression/reuse thresholds. |

---

## 4. Goals & Non-Goals

### Goals
- A precondition can be **operational without blocking boot**.
- Preconditions express **why** they are unmet with structured cause and severity.
- Devices continuously retry with exponential escalation and serviceable fallbacks.
- Automatic recovery without requiring user intervention.
- Flap damping prevents oscillation between application states.
- Single unified evaluation path in the cooperative tick loop.
- One-cause preconditions remain zero-cost and trivial to implement.

### Non-Goals
- Not a full service manager; graph is small, static, and resolved at registration.
- Not preemptive; evaluation runs synchronously under the ≤10ms cooperative tick budget without blocking I/O.
- No dynamic runtime node creation.
- Not a replacement for `IBootReporter` (which feeds early splash progress).
- Not a correctness oracle (the framework carries the verdict; the consumer supplies domain validation).

---

## 5. Summary of Keystone Decisions (CG-1 to CG-18)

- **CG-1: Three questions answered separately.** Default role is `Operational` (non-blocking). `Startup` blocks initial boot.
- **CG-2 & CG-2a: Frequency-based status split.** `getStatus()` runs every tick (hot path, 2 bytes: `met` + `Cause`). `describe()` runs cold on publish/render into caller-supplied buffer (no `mutable`, no leaks).
- **CG-2b: Registry/visitor pattern.** Modules implement `IPreconditionContributor::registerPreconditions(IPreconditionRegistry&)` — prevents silent omissions.
- **CG-3: Structured cause vocabulary.** 12 distinct causes including `RateLimited`, `Unavailable`, `Indeterminate`, `Implausible`.
- **CG-4 & CG-4a: Derived severity and framework-owned trust history.** Safe default is `Transient`. `everMet` and `identityEpoch` track historical success per precondition.
- **CG-5a: Domain plausibility.** Consumer provides domain validation; framework carries `Cause::Implausible`.
- **CG-6 & CG-7: Typed edges resolved at registration.** `Wants`, `Requires`, `BindsTo`. Cycles and unknown targets rejected at startup.
- **CG-8: Immediate satisfaction propagation; non-propagating penalty.** Avoids Erlang multiplicative delay blowup.
- **CG-9 & CG-10: Escalating retry and EventBus fallback.** Fallback requested via `precondition.fallback.requested` over EventBus.
- **CG-11: Half-open recovery probe.** Resource owner performs the probe; open breaker does not block manual user retry.
- **CG-12 & CG-13: Flap damping and dual status publication.** Per-second penalty accrual, exponential decay. Both damped and undamped statuses published.
- **CG-14: App state as a level projection.** Readable as level and edge.
- **CG-15: Unified health vocabulary with `SourceHealth`.** Data sources and preconditions align.
- **CG-16: Zero-cost default for single-cause preconditions.** `describe()` is optional.
- **CG-17: Conditional retirement of `fetchesWhileConfiguring()`.** Gating moved to precondition role.
- **CG-18: Multi-channel surfacing.** Mandatory web portal `/api/preconditions` route and `precondition.health` EventBus topic for display modules.

---

## 6. Consequences & Downstream Impact

1. **Breaking change:** The legacy `IConfigGate` and `GateActionStatus::NeedsUserAction` default were removed.
2. **Time and WiFi separation:** `TimeConfigGate` split into `Timezone` and `Time Sync` operational preconditions, with only WiFi remaining as a default boot-blocking gate.
3. **Display module awareness:** `ConfiguringStatusBoard` counts only `Startup`-role preconditions in its progress bar denominator and handles un-satisfaction smoothly.
4. **Reference pattern alignment:** The sample app was updated so missing dependencies report `Indeterminate` (programming error) rather than returning `true`.
5. **Runtime efficiency:** Edge resolution executes once during `begin()`, replacing per-tick `strcmp` scans with indexed lookups.

---

## 7. Migration Notes from Downstream Consumers

From consumer reviews on [#146](https://github.com/matthewmcneill/esp32-app-foundation/issues/146):
- **DraytonWiserController:** Converted 1 gate class; deleted the `mutable char[96]` workaround.
- **departures-board-v3:** Unified module registration; removed artificial decomposition that was used to evade `IConfigGate`.
- **PowerMonitor:** Integrated operational preconditions for sampling and voltage reference without blocking boot.

---

## 8. Review Changelog & Evolution

- **2026-08-21 (Initial Draft):** Introduced CG-1…CG-12, clock chain case, OQ-1…OQ-5.
- **2026-08-21 (Round-1 Amendments):** Corrected CG-8 to per-second penalty accrual; resolved OQ-7 into CG-2a (frequency-based status/describe split).
- **2026-08-21 (Round-2 Rewrite):** Promoted three-question split to keystone (CG-1); added `RateLimited`, `Unavailable`, `Indeterminate`, `Implausible`; added registration edge validation (CG-7); introduced dual status publication (CG-13); resolved OQ-6 as CG-18 (portal + event topic surfacing).
- **2026-08-22 (Round-4 Amendments):** Classified `Indeterminate` severity as `Persistent` (programming error, loud); clarified `Unavailable` vs `Indeterminate` boundary.
- **2026-08-23 (Implementation Complete):** Added CG-2b (registry/visitor pattern) via [#148](https://github.com/matthewmcneill/esp32-app-foundation/issues/148). Folded into framework across PR-146 work orders WO1–WO7.
