# Configuration Gate Design

> ESP32 App Foundation (c) 2026 CB Labs
>
> Design record for how the framework decides whether a device is ready to run,
> what it tells the user when it is not, how it recovers without human help, and
> when a running device should fall back to configuring.

> [!NOTE]
> **STATUS: RFC — open for consumer review. Not approved, not scoped, not built.**
>
> This document proposes replacing the current `IConfigGate` contract. It is a
> **breaking interface change** for every downstream app that registers a gate.
> No work order exists; nothing here is authorised for implementation.
>
> It shares a health vocabulary with the `DataManager` work in
> [#128](https://github.com/matthewmcneill/esp32-app-foundation/issues/128) and
> should be read alongside it — §7 explains the coupling, and CG-12 retires an
> interface flag added there as a stopgap.

---

## 1. Motivation

### 1.1 What triggered this

Reviewing PR [#145](https://github.com/matthewmcneill/esp32-app-foundation/pull/145),
a consumer found that a data source backing a config gate could never satisfy
that gate. The mechanism was specific — `DataManager::executeLoop()` returned
early on `!_appRunning`, and gate regression clears `_appRunning` — but the shape
was general:

> A gate blocks RUNNING. RUNNING enables the machinery. The machinery is what
> would satisfy the gate.

Tracing that circle exposed a deeper problem, and the reference case that makes
it concrete is the most ordinary chain a device has:

**A clock needs the time. The time needs NTP. NTP needs WiFi. WiFi needs
credentials.**

Every link in that chain can fail in at least two ways that demand *opposite*
responses — one where the user must act, one where the device should simply keep
trying. The current contract cannot express the difference.

### 1.2 The diagnosis

**A gate answers a boolean. The system needs a supervised state machine with a
reason.**

`IConfigGate` today offers:

| Member | Type | Problem |
|---|---|---|
| `isSatisfied()` | `bool` | Serves two incompatible purposes — *may we boot* and *should we tear down* |
| `getActionStatus()` | 3-value enum | Hand-derived per gate; **defaults to blaming the user** |
| `getHelpText()` | fixed `const char*` | One string per gate, chosen at compile time, cannot vary by cause |
| `getDependencies()` | names only | No strength — every edge is implicitly identical |
| `getRegressionToleranceMs()` | one duration | One number for causes that want opposite handling |

Each gate computes the cause internally and the interface discards it at the
boundary. `WifiConfigGate` demonstrates this precisely: it distinguishes three
causes in `getActionStatus()` — no credentials, AP mode, still connecting — and
then returns the same sentence from `getHelpText()` for all of them.

Every symptom in §2 follows from that single discard.

---

## 2. What is broken today

Verified against `main` at `d353740f`. These are observations about the current
implementation, not hypotheticals.

### 2.1 The clock chain has a missing link

`TimeConfigGate::isSatisfied()` returns `_timeManager.isConfigured()` — true when
the **user has chosen a timezone**. It is not sync state. `TimeManager` tracks the
real thing:

```cpp
enum class TimeSyncStatus { UNSYNCED, IN_PROGRESS, SYNCED };
```

but **no gate reads it**. Its only consumer in the framework is a diagnostic
string in `MCPBuiltinTools.cpp:159`.

A device with a chosen timezone and zero NTP sync satisfies every gate and enters
RUNNING. The clock renders whatever the RTC holds. The gate whose *name* suggests
it covers this covers the user's preference, not the fact.

### 2.2 The default action status blames the user

```cpp
virtual GateActionStatus getActionStatus() const {
    return isSatisfied() ? GateActionStatus::Satisfied
                         : GateActionStatus::NeedsUserAction;
}
```

`WifiConfigGate` overrides this correctly. `TimeConfigGate` does not, and neither
would a downstream author writing the obvious NTP gate:

```cpp
bool isSatisfied() const override { return _tm.getSyncStatus() == TimeSyncStatus::SYNCED; }
```

That gate inherits the default, so a flaky network produces a device instructing
the user to fix something — when the honest answer is *waiting for the network*.
**The transient case must be opted into; blame is what you get for free.**

**Consumer evidence (#146) makes this stronger than "the default is wrong".**
`WiserHubConfigGate` *does* override `getActionStatus()`, carefully, with a
documented table — and still returns `NeedsUserAction` for a hub that is
configured, polled, and simply not answering. That is a hub mid-reboot, or a
router that dropped for 90 seconds, and the owner is told to check their firewall.

The author did not inherit that from the default; they hand-wrote it, because with
three statuses and no `Unreachable`, `NeedsUserAction` is **the only bucket that
surfaces anything to the user at all**. `InProgress` would have been more honest
and would have shown them nothing.

So the defect is not only the default. **The enum is too small, which pushes gate
authors toward blame even when they are paying attention.**

### 2.3 The cause is computed, then thrown away

`getHelpText()` takes no arguments. WiFi's three causes all yield:

> "Connect to the device AP and configure WiFi credentials"

- Never configured → correct
- Password rejected → misleading; they *did* configure it
- Router down or out of range → actively wrong; there is nothing to configure

### 2.4 One tolerance number for two opposite behaviours

`getRegressionToleranceMs()` defaults to 30 s. Flaky WiFi reconnecting should be
ridden out; wiped credentials should regress promptly. Both get the same number.
`TimeConfigGate` opts out entirely with `UINT32_MAX`.

### 2.5 `InProgress` has no budget

Nothing bounds it. WiFi connecting with saved credentials reports `InProgress`
indefinitely. There is no trial count, no escalation, and nothing distinguishing
*trying, give it a moment* from *trying, and has been for an hour*.

### 2.6 Failure is terminal, not a fallback

There is no notion of a **degraded but serviceable** mode. A device that cannot
reach its network should still be reachable by its owner. Today AP mode is
entered by `WifiManager` policy, and the gate layer models it only as
`NeedsUserAction` — a failure, not an operating mode with its own recovery path.

---

## 3. Prior art

Five mature systems each solve one part of this. The design below is largely an
application of their ideas rather than an invention.

| System | Transferable idea |
|---|---|
| **[Kubernetes probes](https://kubernetes.io/docs/concepts/workloads/pods/probes/)** | Split the *question*, not the state. Startup / readiness / liveness are three different questions. Readiness failure is **reversible and cheap** — step out of line; liveness failure is **terminal and expensive**. |
| **[systemd](https://www.freedesktop.org/software/systemd/man/latest/systemd.unit.html)** | Dependency strength belongs on the **edge**: `Wants=` (soft), `Requires=` (hard), `BindsTo=` (also stop me if it stops unexpectedly). |
| **[Erlang/OTP supervisors](https://www.erlang.org/doc/system/sup_princ.html)** | Restart **intensity** — more than `MaxR` restarts in `MaxT` seconds and the supervisor gives up *at this level* and escalates to the one above. Also: intensities **multiply** across levels. |
| **[Circuit breaker](https://learn.microsoft.com/en-us/azure/architecture/patterns/circuit-breaker)** | `Half-Open` is a **recovery probe** — after a cooldown, admit one trial; success closes, failure re-opens with a longer cooldown. Self-healing with no human. |
| **[BGP route flap damping](https://networklessons.com/bgp/bgp-route-dampening)** | **Penalty, decay, asymmetric thresholds.** Suppress at 2000, reuse at 750, half-life 15 min. Repeated brief flaps accumulate faster than they decay, so persistent instability is caught even though no single flap qualified. |

The [ESP32 provisioning convention](https://blog.hirnschall.net/esp32-iot-wifi-provisioning/)
— try saved credentials, fall back to AP so the device stays reachable — is the
domain-specific instance of the circuit breaker's fallback.

---

## 4. Goals & Non-Goals

### Goals

- A gate can express **why** it is unsatisfied, and the framework can act on it.
- The device **never stops trying**. Every failure has a retry path.
- Failure degrades to a **serviceable** mode rather than a dead one.
- Recovery is **automatic**: a changed password or an hour-long outage heals
  without anyone touching the device.
- Transient instability does **not** bounce the app between states.
- **One** evaluation path, not one for CONFIGURING and another for RUNNING.
- Gates and data sources share **one** health vocabulary.

### Non-Goals

- Not a general dependency-injection or service-manager framework. Gates are
  boot preconditions, not a systemd port.
- Not preemptive multitasking. Gate evaluation stays on the cooperative tick
  and inherits the 10 ms budget (MK-10).
- No dynamic gate registration at runtime. Gates remain registered pre-`begin()`.
- Not a replacement for `IBootReporter` (MK-8) — this feeds it, it does not
  supersede it.

---

## 5. Core Decisions

### CG-1 — A gate returns a status with a cause, not a boolean

```cpp
enum class GateCause : uint8_t {
    Satisfied = 0,
    NotConfigured,      ///< Nothing to work with — user must supply something
    Connecting,         ///< In progress, expected to resolve on its own
    Rejected,           ///< Config present but refused (bad password, bad key)
    Unreachable,        ///< Config present and plausible, peer not responding
    DependencyUnmet,    ///< Blocked upstream — not this gate's problem
    FallbackEngaged,    ///< Degraded but serviceable (e.g. AP mode)
    InternalError
};

struct GateStatus {
    bool        satisfied  = false;
    GateCause   cause      = GateCause::NotConfigured;
    const char* detail     = nullptr;  ///< Cause-specific text, chosen at runtime
};

virtual GateStatus getStatus() const = 0;
```

`isSatisfied()` remains as a convenience derived from `getStatus().satisfied`, so
the common read stays cheap and legible.

**Rationale.** `WifiConfigGate` already computes exactly this. The change is to
stop discarding it at the interface boundary.

### CG-2 — Severity is derived from cause, and the safe default is *Transient*

| Cause | Severity | Meaning |
|---|---|---|
| `Connecting`, `Unreachable` | **Transient** | Keep trying. Do not blame anyone. |
| `NotConfigured`, `Rejected` | **NeedsUser** | Only a person can resolve this. |
| `DependencyUnmet` | **Blocked** | Not actionable; the upstream gate owns the report. |
| `FallbackEngaged` | **Degraded** | Serviceable, still self-healing. |
| `InternalError` | **Persistent** | A defect, not a configuration problem. |

The default for an unimplemented or unrecognised cause is **Transient**, not
`NeedsUserAction`. A gate author who writes the obvious thing gets "we are working
on it" rather than an accusation. Blaming the user must be an explicit act.

This inverts §2.2 and is the single highest-value change in this document.

#### CG-2a — severity derives from cause **and health**, not cause alone

Consumer review (#146) showed the table above is insufficient for a peer on the
local network. `Unreachable` conflates two conditions that are the same case for
an internet service reached by hostname and **different cases** for a LAN peer at
a user-entered address:

| Condition | Right response |
|---|---|
| The peer isn't answering | Wait — it will come back |
| We may be **looking in the wrong place** | Only a person can fix this |

Getting it wrong in either direction is bad: as Transient, a mistyped IP retries
silently forever; as NeedsUser, a rebooting hub nags the owner.

The discriminator already exists. `SourceHealth::everSucceeded` (CG-10) is exactly
the bit: **a peer we have reached before and lost is Transient; a peer we have
never once reached is NeedsUser.**

```cpp
Severity severityOf(const GateStatus& s, bool everSucceeded);
```

Severity is therefore a function of `GateStatus` **plus** health, not of `cause`
alone. This is the never-connected principle (CG-10) arriving from a third
direction, and it is why CG-2 and CG-10 cannot be sequenced apart.

### CG-3 — Dependency edges carry strength

```cpp
enum class DependencyKind : uint8_t { Wants, Requires, BindsTo };
struct GateDependency { const char* name; DependencyKind kind; };
```

Following [systemd](https://www.freedesktop.org/software/systemd/man/latest/systemd.unit.html):

- **`Requires`** — cannot be satisfied without it. Blocks boot; while the
  dependency is unsatisfied this gate reports `DependencyUnmet`, is not
  actionable, accrues no penalty, and shows the user nothing.
- **`BindsTo`** — everything `Requires` does, **plus** propagation: if the
  dependency becomes unsatisfied at runtime, this gate does too. This is how a
  clock stops claiming to know the time when NTP is lost.
- **`Wants`** — ordering and presentation only.

`ConfigGateResolver::findActionableGate()` already skips gates whose dependencies
are unsatisfied. CG-3 makes that behaviour *specified* rather than incidental, and
adds the runtime propagation `BindsTo` needs.

### CG-4 — Separate the three questions

Following [Kubernetes](https://kubernetes.io/docs/concepts/workloads/pods/probes/),
one predicate cannot serve three purposes:

| Question | Today | Proposed |
|---|---|---|
| May we leave boot? | `isSatisfied()` | `Requires` gates satisfied |
| Should we be serving now? | *same bool* | Readiness — reversible, cheap |
| Is this broken enough to tear down? | *same bool* + timeout | Damped severity (CG-8) |

This is what makes §7's "don't write it twice" possible: the *question* splits,
the state machine does not.

### CG-5 — Each gate owns a retry policy that escalates

```cpp
struct GateRetryPolicy {
    uint32_t baseDelayMs   = 5000;
    uint32_t ceilingMs     = 300000;   ///< 5 min
    uint8_t  attemptsBeforeFallback = 5;
};
virtual GateRetryPolicy getRetryPolicy() const { return {}; }
```

**Fallback is requested over the EventBus, not through a gate virtual**
(resolves OQ-4, per #146). An `engageFallback()` method quietly encodes the
assumption that fallback means *the WiFi one*: it requires the gate to reach
`WifiManager`. One consumer's natural fallback is **hub re-discovery** — the
sideways analogue of AP mode, owned by their poller, with nothing to do with WiFi.

The framework publishes `gate.fallback.requested` carrying the gate name; the
module that owns the resource decides what its own degraded mode is. That is also
the only version testable without a `WifiManager` in the fixture.

Following [OTP restart intensity](https://www.erlang.org/doc/system/sup_princ.html):
after *N* failed attempts the gate stops retrying *at this level* and escalates by
invoking its fallback. Failure at one level becomes an event at the next, rather
than an unbounded retry loop or a dead device.

> [!WARNING]
> OTP's caution applies directly: retry budgets **multiply** through a dependency
> chain. A `Requires` chain three deep, each allowing 5 attempts, is 125 attempts
> before the top gives up. Budgets must be set with the chain in mind, and the
> framework should log the effective total at registration.

### CG-6 — Fallback is a degraded operating mode, not a failure state

AP mode is not "WiFi failed". It is **the device remaining serviceable while it
cannot reach the network** — the entire reason for entering it.

`FallbackEngaged` is therefore its own cause with its own severity (`Degraded`),
distinct from `NeedsUser`. The device reports: *"Can't reach your network. I've
opened my own access point so you can reach me, and I'll keep trying yours."*
That is a materially different message from *"WiFi failed, go fix it"*, and it is
the message that matches what the device is actually doing.

### CG-7 — Recovery is a half-open probe

Following the [circuit breaker](https://learn.microsoft.com/en-us/azure/architecture/patterns/circuit-breaker):

| Breaker state | Gate meaning |
|---|---|
| **Closed** | Normal operation; gate satisfied or retrying freely |
| **Open** | Retries exhausted; fallback engaged (AP mode) |
| **Half-Open** | Cooldown elapsed — **one** trial attempt at the real thing |

From AP mode with saved credentials, station mode is periodically re-attempted.
Success closes the breaker and returns to STA. Failure re-opens it with a longer
cooldown, capped at the ceiling.

> [!IMPORTANT]
> **The gate reports breaker state; the module that owns the resource performs the
> probe.** An earlier draft read as though the gate re-attempts the connection
> itself, which contradicts this document's own constraint (§7) that a gate reads
> cached state and never performs I/O. The half-open transition is a *request*
> (CG-5) — `WifiManager` retries WiFi, a poller retries its peer — and the gate
> observes the outcome.

This is the direct answer to *"if AP mode is sitting there for 5 mins and a WiFi
config exists it should periodically try it"* — a changed password or an
hour-long outage recovers with nobody present.

> [!NOTE]
> `DataManager`'s `Quarantined` state (#128, WO2) is already exactly this
> breaker: at the backoff ceiling, still probing, self-healing on success. The
> gate layer is adopting a pattern the data layer already has.

### CG-8 — Regression is damped, not timed

Replace `getRegressionToleranceMs()` with
[BGP-style damping](https://www.juniper.net/documentation/us/en/software/junos/routing-policy/topics/concept/policy-using-routing-policies-to-damp-bgp-route-flapping.html):

> [!CAUTION]
> **Penalty must accrue by elapsed time, not by attempt count.** The first draft of
> this section copied BGP's constants directly — `penaltyPerFailure = 1000` against
> `suppressThreshold = 2000` — which means **two failures suppress**. For a gate
> whose unit of failure is a 30-second poll timeout that is a 60-second fuse:
> dramatically more trigger-happy than the 5-minute tolerance it replaces, and it
> would regress on a blip that today's code rides out.
>
> The flaw is that **the consumer chooses the retry cadence and the framework
> chooses the penalty, and nothing connects them.** BGP does not have this problem
> because a route flap is an event with its own natural rate; here the rate is a
> downstream configuration choice.
>
> The policy is therefore expressed **per second unsatisfied**, so a slow-polling
> gate and a fast-polling one reach suppression after the same wall-clock outage.
> Raised by #146; the scenario table below is only true under this correction.

```cpp
struct GateDampingPolicy {
    uint16_t penaltyPerSecond   = 10;     ///< accrual rate while unsatisfied
    uint16_t suppressThreshold  = 2000;   ///< above → regress to CONFIGURING
    uint16_t reuseThreshold     = 750;    ///< below → return to RUNNING
    uint32_t halfLifeMs         = 900000; ///< 15 min decay
};
```

At 10/second, suppression follows roughly 200 seconds of continuous failure —
close to the 5-minute constant consumers converged on independently, and now
independent of poll cadence.

| Scenario | Behaviour |
|---|---|
| WiFi blips for 20 s | Penalty rises, decays, **stays RUNNING** |
| WiFi down 10 min | Crosses suppress → **regress to CONFIGURING** |
| WiFi flapping every 30 s for an hour | Accumulates faster than it decays → **eventually regresses**, though no single blip qualified |
| Recovery | Must fall below the *lower* reuse threshold → **no oscillation** |
| `Rejected` (bad password) | Penalty applied at full weight → regresses promptly, because only a user can fix it |

Severity scales the accrual rate: `Transient` causes accrue lightly, `NeedsUser`
causes accrue at full weight. One mechanism, both behaviours — this is what
removes the need for two code paths.

**Damping parameters are owned per-severity by the framework, with a per-gate
override that is available but not advertised** (resolves OQ-2, per #146). The
evidence is a single consumer gate whose regression behaviour differs *from
itself* depending on cause — its 5-minute constant carries a comment stating both
clauses wanting different numbers, and neither case is served by the compromise.
The meaningful axis is severity, not gate identity. If per-gate is made the
primary knob, every consumer re-derives the same bad compromise constant.

### CG-9 — One continuous evaluation loop; app state is a projection

Gates evaluate **every tick, in every state**, under one policy. `AppState` stops
being an independent machine and becomes a *derivation*:

```
all Requires gates satisfied          → RUNNING
any Requires gate suppressed (CG-8)   → CONFIGURING
unsatisfied but under threshold       → RUNNING, surfaced as degraded
```

That third row does not exist today, and its absence is why every transient
wobble currently has to be handled as a state transition.

Nothing is gated *on* `AppState` — which is what dissolves the circular
dependency in §1.1 rather than exempting it.

#### CG-9a — the projection must be readable as a **level**, not only as an edge

Consumer review (#146) resolves OQ-1 with a hazard I had not accounted for.

Downstream does not read `AppState` as a value at all — it subscribes to the
`APP_RUNNING` **transition**. Both state bugs that consumer shipped this month
were edge-versus-level bugs rather than cause bugs: one where `APP_RUNNING`
dispatched synchronously during registration and raced another handler, and one
where a `HUB_DISCONNECTED` event published only on a transition *from* connected,
so a device that had **never** connected published nothing and sat looking healthy.

That second bug is CG-10's principle arriving from yet another direction, and it
carries a warning for CG-9: **under CG-8 the projection can legitimately flap.** A
gate crossing suppress and later dropping below reuse emits two more transitions
to every consumer. CG-8 makes the projection oscillate *less often* but no longer
*monotonically*, and existing edge-consumers were written assuming boot-time
monotonicity.

So the projection is published as gate health **and** exposed as a cheap level
query a module can poll from `tick()`. These are not in tension; the edge-only API
is the thing consumers keep tripping over.

**The derivation ships as a framework helper.** If each display module derives
RUNNING from the gate table itself, three modules will produce three subtly
different answers, and the differences will surface only during a flap.

### CG-10 — Gates and data sources share one health vocabulary

`SourceHealth` (#128) already carries `state`, `lastStatus`,
`consecutiveFailures`, `everSucceeded` / `neverConnected()`, backoff and next
attempt. `GateStatus` is the same shape at a different layer.

A gate backed by a data source therefore **reads `SourceHealth` directly** rather
than inferring from a moving timestamp. `neverConnected()` maps to
`NotConfigured`-adjacent reporting; `Quarantined` maps to `Unreachable`;
`Suspended` maps to `Rejected`.

> The downstream principle that produced #128, restated: **a state that has never
> succeeded must be reported as loudly as one that has failed.** Silence and
> "still trying" are indistinguishable to the person in front of the device, and
> never-succeeded is the case they can actually act on.

### CG-11 — Help text is selected at runtime by cause

`getHelpText()` becomes `getHelpText(GateCause)`, or `detail` on `GateStatus`
carries it. WiFi's three causes finally give three answers:

| Cause | Message |
|---|---|
| `NotConfigured` | "Connect to the device AP and set your WiFi credentials" |
| `Rejected` | "The saved WiFi password was refused — re-enter it via the portal" |
| `Unreachable` | "Can't see your network. Still trying." |
| `FallbackEngaged` | "Can't reach your network. Connect to `<AP-SSID>` to reconfigure — I'll keep retrying." |

### CG-12 — `IDataSource::fetchesWhileConfiguring()` is retired

That flag was added under PR #145 as a stopgap for §1.1. It asks a source a
question it cannot answer — *am I a boot prerequisite?* — which is a property of
the application's composition, not of the source.

Under CG-9 nothing is gated on `AppState`, so there is nothing to opt out of. The
flag is deleted in the same breaking change that introduces `GateStatus`.

If a per-source readiness predicate is still wanted, the correct question is one
the source *can* answer — *do I have the configuration I need to try?* — and it
belongs with the transport work in #128, not here.

---

## 6. Reference case: the clock chain

The chain worked end to end. Four gates, three dependency edges of two strengths.

| Gate | Satisfied when | Depends on |
|---|---|---|
| `WiFi Station` | STA connected | — |
| `Time Sync` | `TimeSyncStatus::SYNCED` | `WiFi Station` (**Requires**) |
| `Timezone` | user has chosen a zone | — *(configurable in AP mode)* |
| `Clock` | time sync **and** timezone | both (**BindsTo**) |

Note `Timezone` has **no** dependency on WiFi: it is settable from the cached
database in AP mode. The current `TimeConfigGate` declares one purely for display
ordering, which CG-3's `Wants` expresses honestly.

### Walkthrough

**Fresh device, no credentials.** `WiFi Station` → `NotConfigured` / `NeedsUser`.
`Time Sync` → `DependencyUnmet`, silent. Portal shows one instruction, not three.

**Credentials entered, router down.** `WiFi Station` → `Connecting` /
**Transient**. The user is told *"connecting…"*, not blamed. After 5 attempts
(CG-5) the breaker opens and the fallback engages → `FallbackEngaged` /
**Degraded**: *"Can't reach your network. Connect to `esp32-setup` — I'll keep
trying."* The device is reachable throughout.

**Router returns 40 minutes later.** Half-open probe (CG-7) retries STA, succeeds,
breaker closes. `Time Sync` becomes actionable, NTP syncs, `Clock` satisfies.
**No user involvement.**

**Password changed at the router.** STA attempt → `Rejected` / **NeedsUser**, at
full damping weight. Fallback engages so the device stays reachable, and the
message is specific: *the saved password was refused* — which is the one thing
the user needs to know and the one thing today's fixed help text cannot say.

**Running for a month; WiFi blips for 30 s.** `Clock` `BindsTo` `Time Sync`, so it
reports stale rather than confidently wrong. Penalty rises and decays. **No
regression** — the device stays in RUNNING and the clock keeps ticking from the
RTC.

**Running; WiFi flaps every few minutes all afternoon.** Penalty accumulates
faster than the half-life clears it, crosses suppress, and the device regresses to
CONFIGURING — correctly, because something is genuinely wrong. Recovery requires
dropping below the *lower* reuse threshold, so it does not bounce.

### 6.1 What the clock chain does not cover: failing sideways

Raised by #146, and worth recording because it shaped CG-2a.

**Every link in the clock chain fails outward** — WiFi, then the internet, then
NTP, each one further away than the last. A peer on the local network **fails
sideways**: WiFi is healthy, the portal is reachable, the device is fully online,
and the peer is still unreachable because it is on another subnet, behind AP
isolation, or has moved by DHCP.

Under the cause enum alone that is `Unreachable` / Transient forever — correct,
and useless. The honest message is *"your network is fine, this specific peer
isn't, and it's the one you typed in."*

The chain is a good reference case because it has a genuine dependency, both
failure kinds and a missing link. It is a poor one for **peer-shaped**
preconditions, where reachability is not a function of distance. CG-2a exists to
cover that, and any future reference case should include one sideways link.

---

## 7. Consequences

**Breaking.** Every downstream gate implementation changes. There is no
default-delegating shim that preserves the old semantics honestly, because the
old default (`NeedsUserAction`) is the behaviour being fixed — silently keeping it
would preserve the bug.

**Ordering.** CG-10 means this should land *after* #128's health model, which is
already on `main` in part. CG-12 deletes an interface member added there.

**Not free.** Per-gate breaker and damping state is a handful of bytes per gate;
gate count is small and static. Evaluation stays on the cooperative tick.

**Testability.** Damping, breaker transitions and dependency propagation are pure
scheduling logic with an injected clock — the same shape as
`DataManager::recordFetchOutcome()`, and natively testable for the same reason.

**What this does not fix.** Gate evaluation still runs on the main loop and still
must not perform I/O. A gate reads cached state; something else does the work.
That constraint is unchanged and remains a documented contract.

---

## 8. Open questions

- **OQ-1 — RESOLVED (#146).** Both. Publish gate health *and* expose a level query;
  ship the derivation as a framework helper. Edge-only is what consumers keep
  tripping over, and CG-8 makes the projection flap. See CG-9a.
- **OQ-2 — RESOLVED (#146).** Per-severity defaults owned by the framework,
  per-gate override available but not advertised. See CG-8.
- **OQ-3** — Does `BindsTo` propagate transitively? A clock binds to time sync,
  which requires WiFi. If WiFi drops, does the clock regress immediately or wait
  for time sync's own damping? Erlang's multiplication warning suggests
  explicitness over inference.
- **OQ-4 — RESOLVED (#146).** EventBus request, not a gate virtual. At least one
  consumer's fallback is peer re-discovery owned by their poller, unrelated to
  WiFi. See CG-5.
- **OQ-5** — Should `Clock` be a gate at all, or a consumer of gate health?
  Nothing blocks boot on it; it just should not lie. This may be the boundary
  between "gate" and "degraded feature".

---

## 9. Related documents

- [`docs/microkernel-architecture-design.md`](microkernel-architecture-design.md) — MK-8 (`IBootReporter`), MK-10 (tick budget)
- [`docs/hardware-configuration-design.md`](hardware-configuration-design.md) — RFC/consumer-review precedent
- [`include/interfaces/IConfigGate.hpp`](../include/interfaces/IConfigGate.hpp) — current contract
- [`include/interfaces/IDataSource.hpp`](../include/interfaces/IDataSource.hpp) — `SourceHealth`, the vocabulary CG-10 shares
- [#128](https://github.com/matthewmcneill/esp32-app-foundation/issues/128) — DataManager resilience
- [#145](https://github.com/matthewmcneill/esp32-app-foundation/pull/145) — the review that surfaced this

## 10. Changelog

| Date | Change |
|---|---|
| 2026-08-21 | Initial draft. CG-1…CG-12, clock-chain reference case, OQ-1…OQ-5. Raised for consumer review. |
| 2026-08-21 | Consumer review round 1 ([#146](https://github.com/matthewmcneill/esp32-app-foundation/issues/146)). **CG-8 corrected** — penalty accrues per second unsatisfied, not per failed attempt; the drafted constants were a 60-second fuse for a 30-second poll and would have regressed on blips today's code rides out. **CG-2a added** — severity derives from cause *and* `everSucceeded`, because `Unreachable` conflates "not answering" with "looking in the wrong place" for a LAN peer. **CG-9a added** — the projection must be readable as a level, not only an edge, since CG-8 makes it legitimately flap. **CG-7 corrected** — the gate reports breaker state; the owning module performs the probe (the draft contradicted §7). OQ-1, OQ-2, OQ-4 resolved. §2.2 strengthened and §6.1 added. |
