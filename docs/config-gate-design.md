# Configuration Gate Design

> ESP32 App Foundation (c) 2026 CB Labs
>
> Design record for how the framework decides whether a device is ready to run,
> what it tells the user when it is not, how it recovers without human help, and
> when a running device should fall back to configuring.

> [!NOTE]
> **STATUS: RFC — round 2. Restructured after three consumer reviews. Not approved,
> not scoped, not built.**
>
> This is a **breaking interface change**. Round 1 was reviewed by all three
> downstream programs (DraytonWiserController, departures-board-v3, PowerMonitor)
> on [#146](https://github.com/matthewmcneill/esp32-app-foundation/issues/146).
> Their evidence moved the keystone decision, so this is a rewrite rather than an
> amendment — decision numbers have changed. See §14 for the mapping.
>
> Shares a health vocabulary with [#128](https://github.com/matthewmcneill/esp32-app-foundation/issues/128);
> CG-17 retires an interface flag added there as a stopgap.

---

## 1. Motivation

### 1.1 What triggered this

Reviewing PR [#145](https://github.com/matthewmcneill/esp32-app-foundation/pull/145),
a consumer found that a data source backing a config gate could never satisfy that
gate. The mechanism was specific — `DataManager::executeLoop()` returned early on
`!_appRunning`, and gate regression clears `_appRunning` — but the shape was
general:

> A gate blocks RUNNING. RUNNING enables the machinery. The machinery is what
> would satisfy the gate.

### 1.2 What the consumer reviews then established

The round-1 document proposed a richer gate status as its central change. Three
independent reviews said the central problem is one level down, and two of them
reached the same conclusion from unrelated domains:

> **departures-board-v3:** shallow graphs are a consequence of the contract, not
> the domain. Every edge today is boot-blocking, so consumers rationally refuse to
> stack them.

> **PowerMonitor:** yes, there is a chain we would build — and not one node of it
> belongs in the boot-blocking set. Richer status alone does not enable it.

Both then described a concrete two-deep chain they cannot express, and both named
the same reason. That reframes everything: **the framework conflates "this must be
true before we boot" with "this must be true for the device to work properly",
and because only the first exists, consumers with the second either avoid the
interface entirely or model nothing at all.**

The evidence that this is happening, not merely possible:

| Observed | Evidence |
|---|---|
| A consumer decomposes `registerModule()` into five calls purely to *escape* becoming a gate | `departures-board-v3/src/main.cpp:142-148` — comment: *"skipping IConfigGate (API keys are optional — shouldn't block app from entering RUNNING)"* |
| A consumer with many preconditions authors **zero** gates, by written decision | `PowerMonitor/docs/…Software System Design.md:145` — *"Not used: … IConfigGate (device is useful with defaults out-of-the-box)"* |
| A consumer deferred a **wanted feature for a month** solely because its gate blocks boot | `PowerMonitor/.agents/…/pr-10-hw-bringup/PROJECT.md:53-56` — *"once the framework's gate no longer hard-blocks RUNNING on NTP"* |
| A gate was designed, then abandoned | `departures-board-v3/src/NationalRailModule.cpp:61-63` — fossil `// IConfigGate` heading |

Four consumers (including the sample app) and not one two-deep chain between them.
That is not a domain fact. It is a contract fact.

### 1.3 The diagnosis

**Two conflations, one on each side of the interface.**

**On the framework's side:** one predicate answers three different questions —
*may we boot*, *should we be serving*, and *is this broken enough to tear down*.
The three want different answers, different reversibility and different costs.

**On the gate's side:** a gate computes *why* it is unsatisfied and the interface
discards it. `WifiConfigGate` distinguishes three causes in `getActionStatus()`
and then returns one fixed string from `getHelpText()` for all of them.

Everything in §2 follows from those two.

---

## 2. What is broken today

Verified against `main` at `d353740f`. Every claim below was read in the code.

### 2.1 The clock chain has a missing link

`TimeConfigGate::isSatisfied()` returns `_timeManager.isConfigured()` — true when
the **user has chosen a timezone**. It is not sync state. `TimeManager` tracks the
real thing (`TimeSyncStatus { UNSYNCED, IN_PROGRESS, SYNCED }`) but **no gate reads
it**; the only consumer in the framework is a diagnostic string at
`MCPBuiltinTools.cpp:159`.

A device with a chosen timezone and zero NTP sync satisfies every gate, enters
RUNNING, and renders whatever the RTC holds.

### 2.2 The enum is too small, so even attentive authors blame the user

```cpp
virtual GateActionStatus getActionStatus() const {
    return isSatisfied() ? GateActionStatus::Satisfied
                         : GateActionStatus::NeedsUserAction;
}
```

The default is wrong. But the sharper finding is that overriding it does not save
you. `WiserHubConfigGate` overrides it *carefully, with a documented table*, and
still returns `NeedsUserAction` for a hub that is configured, polled and simply not
answering — a hub mid-reboot, or a router down for 90 seconds. The owner is told to
check their firewall.

The author did not inherit that from the default. They hand-wrote it, because with
three statuses and no `Unreachable`, `NeedsUserAction` is **the only bucket that
surfaces anything to the user at all**. `InProgress` would have been more honest
and would have shown them nothing.

### 2.3 The cause is computed, then thrown away — three independent instances

| Where | Causes | Messages |
|---|---|---|
| `WifiConfigGate` | 3 (no credentials / AP mode / connecting) | 1 |
| `WeatherModule` (sample app) | 2 (no API key / no location) | 1 — *"Set your API key and location via the Weather tab"* |
| `WiserHubConfigGate` | 3 | 3, via a `mutable char _helpText[96]` and `snprintf` **inside a `const` method** |

The third is the telling one: a consumer paid for per-cause text out of pocket, at
the cost of const-correctness, because the interface left no alternative.

### 2.4 A gate cannot say "I cannot evaluate"

From the sample app — the code consumers copy:

```cpp
bool WeatherModule::isSatisfied() const {
    if (!_keyManager) return true;  // No KeyManager wired — don't block boot
```

The gate **claims to be satisfied when it cannot determine its own state.** It lies
deliberately, because every alternative is worse: `NotConfigured` blames a user who
has done nothing wrong, and there is no third option. "Don't block boot" is exactly
the wrong default for a framework — a precondition that cannot evaluate itself
waving boot through is how you ship a device running without the thing the gate
existed to guarantee.

### 2.5 One tolerance number for causes that want opposite handling

`getRegressionToleranceMs()` defaults to 30 s. `WiserHubConfigGate` uses 5 minutes,
and the comment beside it states the whole problem in prose:

> 5 minutes: long enough that brief hub blips stay on the Offline screen rather than
> tearing down the UI on a wall thermostat; short enough that a genuinely
> dead/unplugged hub surfaces the actionable gate modal rather than sitting silently
> offline forever.

Two clauses, two different numbers wanted, one constant available. Neither case is
served: a 6-minute router outage tears down the UI, and a hub unplugged for 4
minutes still looks fine.

`TimeConfigGate` opts out entirely with `UINT32_MAX`.

### 2.6 `InProgress` has no budget

Nothing bounds it. WiFi connecting with saved credentials reports `InProgress`
indefinitely — no trial count, no escalation, nothing distinguishing *trying, give
it a moment* from *trying, and has been for an hour*.

### 2.7 Failure is terminal, not a fallback

There is no notion of a **degraded but serviceable** mode. A device that cannot
reach its network should still be reachable by its owner. AP mode is entered by
`WifiManager` policy and the gate layer models it only as `NeedsUserAction` — a
failure, not an operating mode with its own recovery path.

### 2.8 Dependency edges are all presentation, and are served as hard blocks

Every dependency edge in existence, across the framework and three consumers:

| Edge | Declared meaning |
|---|---|
| `TimeConfigGate` → `WIFI_STATION` | *"gate ordering ensures WiFi is presented first"* (`TimeConfigGate.hpp:19-21`, repeated at `:87-89`) |
| `WeatherModule` → `WIFI_STATION` | *"don't show until WiFi is connected"* |
| `BoardRegistry` → `WIFI_STATION` | presentation; predicate reads persisted config only |

**All three are `Wants`.** All are depth one. All point at the same node. And
`ConfigGateResolver::findActionableGate()` serves them as `Requires` — which is a
live defect, not merely unspecified: departures-board reports that a user sitting
on the device's own AP, who could configure a board right now, is never shown the
option.

### 2.9 The framework's own display code assumes gates never un-satisfy

`ConfiguringStatusBoard.cpp:200`:

```cpp
_currentPercent = _targetPercent; // Snap down (gates don't un-satisfy normally)
```

The progress bar lerps forward smoothly and snaps backward, because the author
assumed backward would not happen. Any design that makes un-satisfaction ordinary
breaks this.

### 2.10 Being a gate is expensive, and the cost is not obvious

Entering CONFIGURING calls `carousel.suspend()` and swaps the base board
(`U8G2DisplayModule.cpp:1250-1253`), while `Application::tick()` continues ticking
every module (`Application.cpp:282-289`). So on a factory-fresh PowerMonitor the
OLED reads *"Set your timezone"* while sampling runs perfectly — for a setting
nothing in its power path reads.

That cost is why §1.2's four evasions happened.

---

## 3. Prior art

| System | Idea taken |
|---|---|
| **[Kubernetes probes](https://kubernetes.io/docs/concepts/workloads/pods/probes/)** | Split the *question*, not the state. Startup / readiness / liveness are different questions; readiness failure is **reversible and cheap**, liveness failure **terminal and expensive**. |
| **[systemd](https://www.freedesktop.org/software/systemd/man/latest/systemd.unit.html)** | Dependency strength belongs on the **edge**: `Wants=`, `Requires=`, `BindsTo=`. |
| **[Erlang/OTP](https://www.erlang.org/doc/system/sup_princ.html)** | Restart **intensity** and escalation to the level above. Also the warning that budgets **multiply** through a chain — which CG-8 resolves by refusing to multiply at all. |
| **[Circuit breaker](https://learn.microsoft.com/en-us/azure/architecture/patterns/circuit-breaker)** | `Half-Open` as a **recovery probe**: after a cooldown, admit one trial; success closes, failure re-opens with a longer cooldown. |
| **[BGP flap damping](https://networklessons.com/bgp/bgp-route-dampening)** | **Penalty, decay, asymmetric thresholds** — repeated brief flaps accumulate faster than they decay, catching instability no single flap would. |

The [ESP32 provisioning convention](https://blog.hirnschall.net/esp32-iot-wifi-provisioning/)
— try saved credentials, fall back to AP so the device stays reachable — is the
domain instance of the circuit breaker's fallback.

---

## 4. Goals & Non-Goals

### Goals

- A precondition can be **operational without blocking boot**. This is the goal
  everything else serves.
- It can express **why** it is unmet, and the framework can act on that.
- The device **never stops trying**, and failure degrades to a **serviceable** mode.
- Recovery is **automatic** — a changed password or an hour-long outage heals with
  nobody present.
- Transient instability does **not** bounce the device between states.
- **One** evaluation path, not one per app state.
- A one-cause precondition stays **as cheap to write as it is today**.

### Non-Goals

- Not a service manager. This is not a systemd port; the graph is small and static.
- Not preemptive. Evaluation stays on the cooperative tick under the 10 ms budget
  (MK-10), reads cached state, and performs no I/O.
- No dynamic registration at runtime.
- Not a replacement for `IBootReporter` (MK-8) — this feeds it.
- **Not a correctness oracle.** See CG-5a: the framework can describe the state of a
  conversation; it cannot judge whether an answer is true.

---

## 5. Part A — What a precondition is

### CG-1 — Three questions, answered separately *(keystone)*

One predicate cannot serve three purposes. Following Kubernetes:

| Question | Today | Proposed | Reversibility |
|---|---|---|---|
| May we leave boot? | `isSatisfied()` | `role` includes `Startup` **and** unmet | Expensive, rare |
| Is this precondition met right now? | *same predicate* | `getStatus()`, evaluated every tick in every state | — |
| Is it unmet badly enough to tear down? | *same predicate* + timeout | damped severity (CG-12) | Expensive |

```cpp
enum class PreconditionRole : uint8_t {
    Operational = 0,  ///< Features degrade while unmet. Does NOT block boot. (default)
    Startup,          ///< Must be met to leave boot.
    Both
};
virtual PreconditionRole getRole() const { return PreconditionRole::Operational; }
```

**The default is `Operational`.** Today the only available answer is "blocks boot",
and §1.2 records four consumers routing around it. Making non-blocking the default
inverts that: declaring a precondition becomes cheap, and blocking boot becomes a
deliberate act.

> **This is the decision the other eighteen depend on.** Chaining (CG-6) has nothing
> to describe until preconditions exist that do not block boot, because consumers
> will not stack edges that each cost them a boot.

### CG-2 — A precondition reports a status, not a boolean

```cpp
struct PreconditionStatus {
    bool          met     = false;
    Cause         cause   = Cause::NotConfigured;
    const char*   detail  = nullptr;   ///< Optional; nullptr is valid and cheap
};
virtual PreconditionStatus getStatus() const = 0;
```

`isSatisfied()` remains, derived, so the common read stays cheap.

### CG-3 — The cause vocabulary

```cpp
enum class Cause : uint8_t {
    Met = 0,
    NotConfigured,    ///< Nothing to work with. A person must supply something.
    Connecting,       ///< In progress; expected to resolve unaided.
    Rejected,         ///< Config present and refused — bad password, revoked token.
    Unreachable,      ///< Config present and plausible; the peer is not answering.
    RateLimited,      ///< The peer is answering and asking us to slow down.
    Implausible,      ///< An answer arrived, well-formed, and a domain check rejects it.
    DependencyUnmet,  ///< Blocked upstream. Not this precondition's problem.
    FallbackEngaged,  ///< Degraded but serviceable, and still self-healing.
    Unavailable,      ///< Not present in this deployment, by configuration. Not a fault.
    Indeterminate,    ///< Cannot evaluate — a dependency was not wired.
    InternalError     ///< A defect, not a configuration problem.
};
```

Four of these are additions the reviews demanded:

- **`RateLimited`** — exists in `UpdateStatus` (`IDataSource.hpp:34`) with no gate
  counterpart. A rate-limited peer is *up and talking*; treating it as `Unreachable`
  is wrong on the authorization axis.
- **`Unavailable`** — a PowerMonitor on an isolated LAN will **never** reach NTP.
  That is the deployment, not a fault. Under a cause set without it, this reports
  `Unreachable` → *"still trying"* forever. The discriminator is a config value the
  device already holds, which is why this earns an enum member where §5a's case does
  not.
- **`Indeterminate`** — §2.4. A precondition whose dependency was not wired has no
  opinion, and must not be forced to lie. Severity `Blocked`; the **framework**
  decides policy, not each author quietly returning `true`.
- **`Implausible`** — the transport succeeded and a **domain check rejected the
  answer**. Every other cause is a shape of *not getting an answer*; this is the only
  one that means *got one, don't trust it*. See CG-5a.

### CG-4 — Severity is derived, and the safe default is *Transient*

| Cause | Severity | Meaning |
|---|---|---|
| `Connecting`, `Unreachable`, `RateLimited` | **Transient** | Keep trying. Blame nobody. |
| `Implausible` | **Transient** or **NeedsUser** | Discriminated by trust history — see CG-4a. |
| `NotConfigured`, `Rejected` | **NeedsUser** | Only a person resolves this. |
| `DependencyUnmet`, `Indeterminate` | **Blocked** | Not actionable; someone else owns the report. |
| `FallbackEngaged` | **Degraded** | Serviceable, still self-healing. |
| `Unavailable` | **Inactive** | Correct as-is. Report once, never nag. |
| `InternalError` | **Persistent** | A defect. |

Unknown or unimplemented causes default to **Transient**, never `NeedsUser`.
Blaming the user must be an explicit act. §2.2 is the whole argument.

#### CG-4a — Where health can discriminate, severity uses it

`Unreachable` conflates two conditions that are identical for an internet service
reached by hostname and **different** for a LAN peer at a user-entered address:

| Condition | Right response |
|---|---|
| The peer is not answering | Wait — it will come back |
| We may be **looking in the wrong place** | Only a person can fix this |

`SourceHealth::everSucceeded` (CG-15) discriminates: **a peer reached before and
lost is Transient; a peer never once reached is NeedsUser.**

```cpp
Severity severityOf(const PreconditionStatus&, const SourceHealth* health);
```

`health` is optional. Where it is absent, severity falls back to the CG-4 table.

#### CG-5a — The framework supplies the vocabulary; the consumer supplies the predicate

> [!IMPORTANT]
> **The framework cannot judge whether an answer is true. It must still give the
> consumer somewhere to say so.**

Every cause except `Implausible` describes a shape of *not getting an answer*. A
local sensor fails a third way — neither outward like the clock chain nor sideways
like a LAN peer. **It does not fail at all.** `Mcp320xBus.hpp:190` substitutes
`kMidRailCounts = 2048` for an absent chip, so an unplugged pod reads 0.00 A on every
channel — identical to a correctly-wired pod watching eight switched-off circuits.
Every read succeeds. Worse, PowerMonitor's own plausibility check is defeated by the
same substitution: 2048 counts is 1.650 V against a 1.65 V mid-rail, deviation
**0.000 V**. *An absent pod scores better than any real hardware ever will.*

The framework has no way to compute that verdict. But a **consumer** very often can —
a mains voltage reference that shows no AC waveform is conclusively dead, whatever the
ADC returns. The division is therefore:

| Responsibility | Owner |
|---|---|
| Deciding whether a reading is credible | **The consumer.** Domain knowledge the framework does not have. |
| Carrying that verdict, and acting on it | **The framework.** `Cause::Implausible` + `detail`. |
| Making driver-level substitution visible | **The framework.** See below. |

Without the third row a consumer with a perfectly good heuristic has nowhere to put
it, and keeps the logic outside the framework — which is exactly what §12 records
happening today.

**This is also what makes CG-4a work here.** `everSucceeded` cannot discriminate a
silent failure while it means *"ever completed a transaction"*. Once a consumer can
report `Implausible`, it means **"ever produced a result we trust"**, and the LAN-peer
rule applies unchanged:

| Trust history | Severity | Message |
|---|---|---|
| Never once produced a credible reading | **NeedsUser** | *"No AC waveform on Grid ZMPT — check the CT orientation."* |
| Produced one before, now implausible | **Transient**, escalating | *"Voltage reference lost. Retrying."* |

##### Choosing the signal to check

> [!WARNING]
> A plausibility predicate is only sound on a signal with a **known-nonzero
> expectation**.

Mains voltage must always show a waveform, so flat DC is conclusive. A current channel
legitimately reads zero — an idle circuit and a disconnected clamp are genuinely
indistinguishable, which is the `kMidRailCounts` problem above and is **not** fixable
by any amount of framework vocabulary.

So: **put the plausibility predicate on the signal that cannot legitimately be quiet,
and let everything else depend on it.** That is why the chain in §9.3 roots at
`Voltage Reference` rather than per-channel current — the root is checkable, and
`Requires`/`BindsTo` carries the verdict to channels that are not.

##### The remaining framework affordance

Where a *driver* substitutes a fallback value — as `Mcp320xBus` does — that
substitution should be **visible in `SourceHealth`**, not inferred. A flag
distinguishing *this value came from the device* from *this value came from a
fallback* costs nothing at the point of substitution and is impossible to reconstruct
afterwards. It composes with `Implausible`: the driver reports what it did, the
consumer reports what it concluded. Tracked as OQ-5.

## 6. Part B — Relationships between preconditions

### CG-6 — Edges carry strength, typed by *when* the dependency matters

```cpp
enum class EdgeKind : uint8_t {
    Wants,     ///< Presentation only: don't offer me while that is unmet.
    Requires,  ///< I cannot be met without it. Evaluated every tick.
    BindsTo    ///< Requires, plus: if it becomes unmet at runtime, so do I.
};
struct Edge { const char* name; EdgeKind kind; };
```

Derived from our problem rather than inherited wholesale: the three are
distinguished by **when** the dependency is consulted — at presentation, at
evaluation, or continuously.

Today every edge is `Wants` in intent (§2.8) and `Requires` in effect. Naming the
distinction fixes departures-board's live defect directly.

### CG-7 — Edges are resolved once at registration, and validated

Names are retained — a consumer cannot hold a pointer to a framework-owned
precondition it never constructed — but resolution moves out of the hot path:

- Resolve each name to an index **at registration**, not per evaluation. Today
  `findActionableGate()` runs `strcmp` over the whole set on every publish.
- **Reject unknown names.** Today a typo silently degrades to "no dependency".
- **Reject cycles** at registration, loudly. A cycle is a programming error, and
  §1.1 is what one costs when it is invisible.
- Log the resolved graph once at boot, so the shape is inspectable.

### CG-8 — Satisfaction propagates immediately; penalty never propagates *(resolves OQ-3)*

Both consumers who answered OQ-3 gave the same answer, and PowerMonitor gave the
reason that generalises:

> Damping exists to avoid expensive, **reversible** churn — tearing down a UI,
> dropping to AP mode, re-running discovery. Where the consequence of acting late is
> **irreversible**, damping is not conservative, it is harmful.

Their worked case: with per-level damping at ~200 s, an `ADC Bus` failure reaches
`Energy Accumulation` at T+400 s, having integrated and flushed **six and a half
minutes of fabricated kWh to flash**. *"You cannot un-bill a phantom kilowatt-hour."*

So:

1. Every precondition computes its **raw** status each tick.
2. `Requires` / `BindsTo` propagate **raw satisfaction immediately**, undamped.
3. Damping (CG-12) is applied **per precondition, to its own raw unmet time**, and
   is **never** inherited across an edge.

Chain teardown time is therefore `max()` of the individual damping windows, not a
product — which **dissolves** Erlang's multiplication warning rather than trading
against it. Set each precondition's damping for its own consequence and the chain
composes correctly by construction.

---

## 7. Part C — Behaviour over time

### CG-9 — Retry policy escalates rather than looping forever

```cpp
struct RetryPolicy {
    uint32_t baseDelayMs           = 5000;
    uint32_t ceilingMs             = 300000;   // 5 min
    uint8_t  attemptsBeforeFallback = 5;
};
```

Following OTP: after *N* attempts, stop retrying *at this level* and escalate by
requesting a fallback. Failure at one level becomes an event at the next.

### CG-10 — Fallback is a degraded operating mode, requested over the EventBus

AP mode is not "WiFi failed". It is **the device remaining serviceable while it
cannot reach the network** — the entire reason for entering it. Hence
`FallbackEngaged` / `Degraded`, distinct from `NeedsUser`.

**The framework publishes `precondition.fallback.requested`; it does not call a
virtual.** An `engageFallback()` method silently assumes fallback means *the WiFi
one*: it requires the precondition to reach `WifiManager`. PowerMonitor's natural
fallback is re-discovery of a sensor bus; DraytonWiser's is hub re-discovery.
Neither is WiFi's business, and the event form is the only one testable without a
`WifiManager` in the fixture.

### CG-11 — Recovery is a half-open probe, performed by the resource owner

| Breaker state | Meaning |
|---|---|
| **Closed** | Normal operation |
| **Open** | Retries exhausted; fallback engaged |
| **Half-Open** | Cooldown elapsed — **one** trial attempt |

> [!IMPORTANT]
> **The precondition reports breaker state; the module owning the resource performs
> the probe.** A precondition reads cached state and never performs I/O (§4). The
> half-open transition is a *request*.

### CG-12 — Damping: per second, per severity, runtime-configurable

> [!CAUTION]
> **Penalty accrues by elapsed time, not by attempt count.** The round-1 draft copied
> BGP's constants — 1000 per failure against a 2000 threshold — which means **two
> failures suppress**. For a 30-second poll that is a 60-second fuse, replacing a
> 5-minute tolerance with something far more trigger-happy. The flaw: **the consumer
> chooses the retry cadence and the framework chooses the penalty, and nothing
> connects them.** BGP does not have this problem because a route flap has its own
> natural rate.

```cpp
struct DampingPolicy {
    uint16_t penaltyPerSecond  = 10;      ///< while unmet, scaled by severity
    uint16_t suppressThreshold = 2000;    ///< above → regress
    uint16_t reuseThreshold    = 750;     ///< below → return
    uint32_t halfLifeMs        = 900000;  ///< 15 min decay
};
```

At 10/second, suppression follows ~200 s of continuous failure. Three consumers
independently converged on 5-minute-ish constants (300 s, 5 min, and this ~200 s)
without knowledge of each other, which is about as good as this evidence gets.

**Owned per severity by the framework; per-gate override available but not
advertised.** The evidence is a single gate whose regression behaviour differs
*from itself* by cause (§2.5). If per-gate is the primary knob, every consumer
re-derives the same bad compromise constant.

**The override must be runtime-settable, not a `constexpr` returned from a
virtual.** PowerMonitor's equivalent is a persisted, portal-editable value: a house
with an EV charger and a UPS that cuts over has genuinely different quiet periods
from a flat. A compile-time-only knob means consumers keep their own timer alongside
ours — which is the status quo this replaces.

**Damping is inert for config-shaped preconditions.** A precondition reading
persisted config does not "fail over time"; it is unmet until a person acts. Its
policy is `NeedsUser` severity, which reaches suppression fast, and that is correct.
Stated so authors do not go looking for a knob that cannot help them.

### CG-13 — Publish the damped *and* undamped status

One condition can carry both a reversible and an irreversible consequence, needing
different speeds. PowerMonitor already runs this split outside the gate system:

| Consequence | Speed |
|---|---|
| Suppress kWh accumulation | **undamped** |
| Show `STALE` on the OLED | damped, 300 s |

A single accumulator producing a single outcome cannot express that, and consumers
will keep their logic outside the framework if it does not. Both views are already
computed; publishing both costs nothing and makes CG-8 a non-question.

---

## 8. Part D — Consuming the result

### CG-14 — App state is a projection, readable as a **level**

```
all Startup-role preconditions met     → RUNNING
any Startup-role precondition suppressed → CONFIGURING
unmet but under threshold              → RUNNING, surfaced as degraded
```

That third row does not exist today, and its absence is why every transient wobble
must currently be handled as a state transition.

**Published as a level, not only an edge.** CG-12 makes the projection legitimately
flap, so an edge-only API becomes *more* dangerous, not less — every existing
edge-consumer was written assuming boot-time monotonicity (§2.9 shows the framework's
own display code assuming exactly that).

Four independent instances of an edge event needing a downstream timer were reported
across two consumers, including a 60-second OTA hang-guard built in an unrelated
subsystem before this RFC existed. Every one used a different timeout.

**The framework ships the derivation as a helper, and that helper is the supported
path.** Three display modules deriving RUNNING independently would produce three
subtly different answers, visible only during a flap.

### CG-15 — One health vocabulary with `SourceHealth`

`SourceHealth` (#128) carries `state`, `lastStatus`, `consecutiveFailures`,
`everSucceeded` / `neverConnected()`, backoff and next attempt.
`PreconditionStatus` is the same shape at a different layer, and a precondition
backed by a data source **reads `SourceHealth` directly** rather than inferring from
a moving timestamp.

> The principle that produced #128, restated: **a state that has never succeeded
> must be reported as loudly as one that has failed.** Silence and "still trying" are
> indistinguishable to the person in front of the device, and never-succeeded is the
> case they can act on.

#### CG-15a — `UpdateStatus` has the same hole, one layer down

```cpp
ParseError = 4, ///< Response received but JSON/payload parsing failed
```

That is **structural** invalidity. There is no member for **semantic** invalidity —
parsed cleanly, values are nonsense. A sensor module reporting into `SourceHealth`
today must return `Success`, which makes `everSucceeded` permanently true and
disables CG-4a's discriminator at the source.

`UpdateStatus::Implausible` is the counterpart to `Cause::Implausible`, and the two
must land together: the gate-layer vocabulary is useless if the health record
underneath it cannot express the same verdict. With both, `everSucceeded` means
**"ever produced a result we trust"** rather than "ever completed a transaction",
which is what CG-4a needs to work for sensors as well as peers.

This is an additive change to `IDataSource.hpp` and belongs with the #128 migration
alongside CG-17.

### CG-16 — Per-cause text is **optional**, with a zero-cost default

```cpp
virtual const char* getHelpText(Cause) const { return getHelpText(); }
virtual const char* getHelpText() const { return ""; }
```

Consumers disagreed here, and the disagreement is the design constraint. DraytonWiser
has three causes and paid for per-cause text by hand. Departures and PowerMonitor each
have **exactly one** cause, where the blaming default is correct and one fixed string
is a complete, honest answer.

> If `getStatus()` is pure-virtual and `detail` has no sensible default, **a one-cause
> precondition becomes more expensive to write than it is today.** That is a
> regression, and it is how you get a fifth evasion in §1.2.

`detail` may be `nullptr`; `getHelpText(Cause)` falls back to the single-string form.
Multi-cause authors opt in.

### CG-17 — `IDataSource::fetchesWhileConfiguring()` is retired

Added under #145 as a stopgap. It asks a source a question it cannot answer — *am I
a boot prerequisite?* — which is a property of the application's composition.

Under CG-1 preconditions are `Operational` by default and under CG-14 nothing is
gated on `AppState`, so there is nothing to opt out of. Deleted in the same breaking
change. All three consumers confirmed zero uses; all three endorsed folding it into
the #128 transport migration so downstream adapts once.

---

## 9. Worked examples

### 9.1 Fails outward — the clock chain

| Precondition | Role | Met when | Edges |
|---|---|---|---|
| `WiFi Station` | **Startup** | STA connected | — |
| `Time Sync` | Operational | `TimeSyncStatus::SYNCED` | `Requires` WiFi Station |
| `Timezone` | Operational | user has chosen a zone | *(none — settable in AP mode)* |
| `Clock` | Operational | time sync **and** timezone | `BindsTo` both |

Only WiFi blocks boot — without it there is no portal, so nothing else is fixable.

**Fresh device, no credentials.** `WiFi Station` → `NotConfigured` / NeedsUser.
`Time Sync` → `DependencyUnmet` / Blocked, silent. One instruction shown, not three.

**Credentials entered, router down.** `Connecting` / **Transient** — *"connecting…"*,
not blame. After 5 attempts (CG-9) the breaker opens, `precondition.fallback.requested`
fires, `WifiManager` engages AP mode → `FallbackEngaged` / Degraded: *"Can't reach your
network. Connect to `esp32-setup` — I'll keep trying."* Device reachable throughout.

**Router returns 40 minutes later.** Half-open probe (CG-11) succeeds; breaker closes.
`Time Sync` becomes evaluable, NTP syncs, `Clock` is met. **No user involvement.**

**Password changed.** `Rejected` / **NeedsUser** at full damping weight. Fallback keeps
the device reachable, and the message is specific — *the saved password was refused* —
which is the one thing the user needs and today's fixed help text cannot say.

**Running a month; WiFi blips 30 s.** `Clock` `BindsTo` `Time Sync`, so it reports stale
rather than confidently wrong — **immediately** (CG-8), before any damping. Penalty rises
and decays. **No regression**; the clock keeps ticking from the RTC.

**WiFi flaps every few minutes all afternoon.** Penalty accumulates faster than the
half-life clears it, crosses suppress, device regresses — correctly. Recovery requires
falling below the *lower* reuse threshold, so it does not bounce.

### 9.2 Fails sideways — a LAN peer

`Wiser Hub` is `Operational`, `Requires` `WiFi Station`. WiFi is healthy, the portal is
up, the device is fully online — and the hub is unreachable because it is on another
subnet, behind AP isolation, or has moved by DHCP.

`Unreachable` alone would report Transient forever: correct, and useless. **CG-4a
discriminates:**

| History | Severity | Message |
|---|---|---|
| Reached before, now lost | Transient | *"Can't reach your hub. Still trying."* |
| **Never once reached** | **NeedsUser** | *"Never reached the hub at 192.168.1.50 — check the address."* |

The second is a mistyped IP, and today it retries silently forever.

### 9.3 Fails silently — a local sensor chain

| Precondition | Role | Met when | Edges |
|---|---|---|---|
| `ADC Bus` | Operational | SPI3 up, pods answering | — |
| `Voltage Reference` | Operational | ZMPT bias within 0.15 V of 1.65 V | `Requires` ADC Bus |
| `Energy Accumulation` | Operational | voltage reference trustworthy | `BindsTo` Voltage Reference |

**No node blocks boot.** A power monitor with a loose ribbon cable must still boot,
serve its portal, and let the owner run diagnostics to discover *why* it reads zero.
Gating this would brick the device precisely when it most needs to be diagnosable —
which is why, under today's contract, this chain has never been written.

**Ribbon works loose.** `ADC Bus` → unmet. `Voltage Reference` → `DependencyUnmet`
**immediately** (CG-8). `Energy Accumulation` → unmet **immediately** via `BindsTo`, and
**stops integrating at once** by reading the undamped status (CG-13) — no phantom kWh.
Meanwhile the OLED shows `STALE` only after the damped window, so a two-second glitch
does not flicker the display. One condition, two speeds.

**Pod absent, reading a perfect mid-rail.** `ADC Bus` is met and every read succeeds, so
no transport-shaped cause fires. `Voltage Reference` runs the consumer's own predicate —
no AC waveform on the ZMPT — and reports `Implausible` with
`detail = "no AC waveform on Grid ZMPT"`. Because the reference has **never** produced a
credible reading, CG-4a resolves that to **NeedsUser**: *"check the CT orientation"*,
rather than *"still trying"* forever.

`Energy Accumulation` `BindsTo` it and stops integrating **immediately** via the undamped
status (CG-13). No phantom kWh, and the owner is told the one thing they can act on.

**What remains genuinely unsolvable.** A *current* channel reading zero is legitimately
ambiguous — an idle circuit and a disconnected clamp are the same measurement. No
framework vocabulary fixes that; the answer is structural, per CG-5a: root the chain at
the signal that cannot legitimately be quiet.

### 9.4 Fails invisibly — a revoked token behind a quorum

`Feed Credentials` (`Operational`, `Requires` WiFi) → `Board Data` (`BindsTo`).

A revoked API token today yields an empty board forever, silently — structurally
identical to §2.1's missing NTP link. Under this design it is `Rejected` / NeedsUser
with specific text.

**The unsolved part**, and it is worth naming: departures-board has a *passing test*
asserting the registry is satisfied with **2 of 3 boards broken**, and the user told
nothing. A fan-out precondition where partial failure is invisible is not covered by
this design — `met` is a boolean, and "met for 1 of 3" has no expression. Recorded as
**OQ-1**.

### 9.5 Correct as-is — an intentionally offline device

A PowerMonitor on an isolated LAN will **never** reach NTP. Not a fault; the deployment.

Without `Unavailable` this is `Unreachable` → Transient → *"still trying"* forever, or
`NeedsUser` nagging about something the owner chose. Neither is honest. `Unavailable` /
**Inactive** reports once and never nags — and the discriminator is a config value the
device already holds.

---

## 10. Deciding what your precondition is

Two rules, from two consumers, that compose into a procedure.

**Is it a precondition at all?** *(departures-board)*

> If it has a typed accessor that can refuse, it is not a precondition — it is a
> feature that degrades.

`TimeManager::getLocalTime()` returns `false` unless synced, and `ScheduleModule`
already handles that refusal by degrading. Nothing needs modelling.

**Does it block boot?** *(PowerMonitor)*

> Can the user act on it **from this device, right now**, and is the device **useless**
> until they do?

Both clauses must hold. Timezone fails the second. Time sync fails both. Sensing
hardware fails the first — you fix a ribbon cable with your hands, and you need the
portal up to know which one.

On a typical device exactly one condition passes: **WiFi**, because without it there is
no portal, so nothing else is fixable. **The boot-blocking set is close to a fixed point
of "what makes the device configurable."**

```
Does it have a typed accessor that can refuse?
  ├─ yes → not a precondition; degrade at the call site
  └─ no  → Can the user fix it from this device, now, AND is the device useless until then?
            ├─ both → Startup
            └─ else → Operational
```

---

## 11. Consequences

**Breaking, with no honest shim.** The old default (`NeedsUserAction`) is the behaviour
being fixed; silently preserving it would preserve the bug.

**`TimeConfigGate` must change.** It is the only gate the framework offers downstream to
opt into, and opting in costs a consumer their display for a setting nothing reads
(§2.10). It becomes `Operational`, and gains a separate `Time Sync` precondition for the
fact it currently fails to model (§2.1).

**`ConfiguringStatusBoard` must change.** Its snap-down assumption (§2.9) is invalidated
by design.

**Ordering.** CG-15 means this lands after #128's health model. CG-17 deletes an
interface member added there.

**Cost.** Per-precondition breaker and damping state is a few bytes; the set is small and
static. Edge resolution moves from per-publish `strcmp` to a registration-time index —
strictly cheaper at runtime.

**Testability.** Damping, breaker transitions and edge propagation are pure scheduling
logic with an injected clock — the same shape as `DataManager::recordFetchOutcome()`, and
natively testable for the same reason.

---

## 12. Migration

Reported cost, from the consumers themselves:

| Consumer | Classes to change | Notes |
|---|---|---|
| DraytonWiserController | 1 | Deletes a `mutable char[96]` and an `snprintf` in a `const` method |
| departures-board-v3 | 2 + 1 test | Can stop decomposing `registerModule()` to escape gatehood |
| PowerMonitor | 0 | Authors no gates — *because the current shape fits nothing they have* |

PowerMonitor's zeros are the most informative number here, and should be read as they
asked: not "this app is simple", but *"this app looked at `IConfigGate`, decided every
one of its preconditions was the wrong shape, and wrote its own health, staleness,
degradation and hang-guard logic instead."*

All three endorsed folding CG-17 into the #128 migration. One break beats two.

---

## 13. Open questions

- **OQ-1 — Fan-out preconditions.** `met` is boolean; "met for 1 of 3 boards" has no
  expression, and a *passing test* today asserts satisfaction with two of three broken
  (§9.4). Does this need a quorum notion, or should a fan-out be N preconditions?
- **OQ-2 — Empty-but-correct.** *"No trains at 3 a.m."* is not a failure. `UpdateStatus`
  has no `NoData`, and `Cause` has no equivalent. Is that a precondition concern at all,
  or purely a data-layer one?
- **OQ-3 — Naming.** If most preconditions are `Operational`, `IConfigGate` is a
  misnomer. `IPrecondition` is more honest and costs a rename across four repos. Severable
  — the model works either way.
- **OQ-4 — Does `BindsTo` propagate transitively?** CG-8 makes this cheap (satisfaction
  only, never penalty), but it is still unstated whether A `BindsTo` B `Requires` C means
  A follows C. Erlang's multiplication warning argues for explicitness over inference.
- **OQ-5 — Does the framework need a "value came from a fallback" flag** in
  `SourceHealth` (CG-5a)? A driver that substitutes knows it did; that fact is
  impossible to reconstruct afterwards and costs nothing to record at the point of
  substitution. It composes with `Implausible` — the driver reports what it *did*, the
  consumer reports what it *concluded* — but it is a second mechanism and may be
  redundant if consumers are checking plausibility anyway.

---

## 14. Decision mapping from round 1

| Round 1 | Round 2 | Change |
|---|---|---|
| CG-4 | **CG-1** | Promoted to keystone; `Operational` is now the default role |
| CG-1 | CG-2 | Unchanged in substance |
| — | CG-3 | New: cause vocabulary, adding `RateLimited`, `Unavailable`, `Indeterminate` |
| CG-2, CG-2a | CG-4, CG-4a | Unchanged in substance |
| — | CG-5a | New: the conversation/truth boundary |
| CG-3 | CG-6 | Reframed — edges typed by *when* they matter |
| — | CG-7 | New: resolve once at registration; validate; reject cycles |
| — | CG-8 | New: resolves round-1 OQ-3 |
| CG-5 | CG-9 | Fallback moved to EventBus |
| CG-6 | CG-10 | Unchanged |
| CG-7 | CG-11 | Probe performed by the resource owner |
| CG-8 | CG-12 | Per-second accrual; runtime-configurable; inert-for-config noted |
| — | CG-13 | New: publish damped *and* undamped |
| CG-9, CG-9a | CG-14 | Level query is the supported path |
| CG-10 | CG-15 | Subject to CG-5a |
| CG-11 | CG-16 | **Demoted to optional** with a zero-cost default |
| CG-12 | CG-17 | Unchanged |

Round-1 OQ-1, OQ-2 and OQ-4 were resolved into CG-14, CG-12 and CG-10. Round-1 OQ-3 and
OQ-5 were resolved into CG-8 and §10. §13 lists five new questions raised by round 1.

---

## 15. Related documents

- [`docs/microkernel-architecture-design.md`](microkernel-architecture-design.md) — MK-8 (`IBootReporter`), MK-10 (tick budget)
- [`docs/hardware-configuration-design.md`](hardware-configuration-design.md) — RFC/consumer-review precedent
- [`include/interfaces/IConfigGate.hpp`](../include/interfaces/IConfigGate.hpp) — current contract
- [`include/interfaces/IDataSource.hpp`](../include/interfaces/IDataSource.hpp) — `SourceHealth`
- [#128](https://github.com/matthewmcneill/esp32-app-foundation/issues/128) — DataManager resilience
- [#145](https://github.com/matthewmcneill/esp32-app-foundation/pull/145) — the review that surfaced this
- [#146](https://github.com/matthewmcneill/esp32-app-foundation/issues/146) — this RFC

## 16. Changelog

| Date | Change |
|---|---|
| 2026-08-21 | Initial draft. CG-1…CG-12, clock-chain reference case, OQ-1…OQ-5. |
| 2026-08-21 | Round-1 amendments: CG-8 corrected to per-second accrual; CG-2a, CG-9a added; CG-7 corrected. |
| 2026-08-21 | Added `Cause::Implausible` and `UpdateStatus::Implausible` (CG-5a, CG-15a). CG-5a had conflated *the framework cannot compute this verdict* (true) with *the model cannot carry it* (false) — a consumer with a working heuristic, such as "no AC waveform on a mains voltage reference", had nowhere to report it and would keep the logic outside the framework. With the verdict expressible, `everSucceeded` can mean "ever produced a result we trust", which restores CG-4a's discriminator for sensors. Added the rule that a plausibility predicate is only sound on a signal with a known-nonzero expectation, and that chains should root there. |
| 2026-08-21 | **Round-2 rewrite** after reviews from all three consumer programs. Keystone moved: the three-question split is now CG-1, and `Operational` is the default role — four consumers were found routing around the boot-blocking-only contract, so chaining had nothing to describe. Added CG-3 cause vocabulary (`RateLimited`, `Unavailable`, `Indeterminate`), CG-5a (cause describes the conversation, not the truth of the answer), CG-7 (registration-time edge resolution and cycle rejection), CG-8 (satisfaction propagates immediately, penalty never), CG-13 (publish damped *and* undamped). CG-16 demoted to optional after two consumers reported one-cause preconditions. Damping made runtime-configurable. §9 worked examples and §10 decision procedure added. See §14 for the full mapping. |
