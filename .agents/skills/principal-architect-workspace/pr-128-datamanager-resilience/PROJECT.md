# Project: 128-datamanager-resilience

> Resolve #128 on its own terms: a data source that cannot be reached must degrade
> *that source*, never reset the device — and must say so loudly enough that the person
> standing in front of the device can act on it.

## Human Approval

- [x] Decomposition approved by human: 2026-08-20 — re-approved after consumer review folded in.
      Execution mode: PA-driven, sequential WO1→WO7, autonomous.
- [x] API-breaking changes to `IDataSource` authorised by human: 2026-08-20 — all three
      downstream consumers are owned by the same maintainer and will adapt.

## Problem Statement

`DataManager` has **no health model**. `SourceRecord` carries `{source, lastFetchTimeMs,
priorityRequested}` and the entire failure path in `executeLoop()` is one `ESP_LOGW`,
followed by a retry at full interval, forever. With no failure state, no backoff and no way
to surface a down source, the only escalation available when a fetch misbehaves is to reboot
the board — so the watchdog became the error handler by default.

PR #136 (WO2) mitigated the reported symptom by unsubscribing the worker from the TWDT for
the duration of the fetch and supervising with a 2 s `esp_timer` that calls `abort()` at
30 s. The boot loop still exists; its threshold moved from 5 s to 30 s.

The reporter's framing, restated because it drives the whole design: **the harm was never
the reboot.** It was that recovery required corrupting the config from outside the device.
Their bench unit is still running today with `hub_ip` deliberately pointed at its own IP so
the fetch fails fast — the only reason it stays up.

## Architectural Position

**Bound the I/O, supervise for liveness, and make failure a state — not a reset.**

### D1 — The framework owns the transport, and `FetchContext` is the seam

`DataManager` currently calls `fetchUpdate()` and receives control back whenever the consumer
chooses. It holds no handle on the client, no knowledge that a socket exists, no way to set a
timeout and no way to interrupt. That is the root of every other problem here.

`fetchUpdate(const FetchContext&)` becomes **pure virtual** — a hard API break, authorised
above. No default-delegating overload, no legacy population, no runtime detection of which
signature a source implements. Every source is bounded by construction.

Review surfaced a genuine tension on what `FetchContext` should carry. Reviewer 1 argued the
**deadline token** must be primary because `IDataSource` is deliberately transport-agnostic —
HTTP, MQTT, serial buses, custom protocols. Reviewer 2 countered that a token alone is *the
same class of trust contract* as today's timeout documentation: the framework is asking
implementations to cooperate, and a non-cooperative one is unbounded again.

Both are right, so `FetchContext` carries both:

- a **cooperative deadline/cancel token** every source can poll — the universal floor, and the
  only option for non-HTTP transports;
- a **framework-constructed HTTP client** with connect, handshake, read and total deadline set
  from one policy — hard enforcement for the common case, requiring no cooperation at all.

Additional value: `sample-app/src/WeatherModule.cpp:325-333` reaches the ESP-IDF certificate
bundle by forward-declaring a private Arduino symbol (`attach_ssl_certificate_bundle`) and
subclassing `WiFiClientSecure` to touch its protected `sslclient` member. Framework-owned
transport deletes the need for that, in this repo and downstream where it has been copied.

### D2 — Feed the watchdog on observed forward progress

House precedent already exists: `OTAUpdateManager` drives a chunked download and calls
`esp_task_wdt_reset()` from its progress callback every ~4 KB
(`OTAUpdateManager.cpp:615-625`). The framework transport adopts the same shape for
response-body reads. Forward progress is what a watchdog should measure.

**Correction to reviewer 3's phrasing, same conclusion:** feeding cannot come *from* the
supervising timer — `esp_task_wdt_reset()` acts on the calling task, so the esp_timer task
would reset its own subscription, not the worker's. But feeding from the framework transport,
which runs *on* the worker task, is legal and achieves exactly what they want: worker stays
subscribed, no global timeout change, panic lands on the worker with the worker's backtrace.

**The unresolved window:** connect + TLS handshake is a single blocking call that cannot be fed
from inside. Three candidate mechanisms, to be settled in WO4 against measured handshake times
on real hardware — not assumed:

1. Bound `connect + handshake` under the existing 5 s TWDT via `setConnectTimeout()` /
   `setHandshakeTimeout()`. No config change; tight on poor links.
2. `esp_task_wdt_reconfigure()` (IDF 5.x) to widen the window *only* across the handshake and
   restore after. Narrower exposure than a global raise — but note it reconfigures the whole
   TWDT instance, not one task, so idle-task supervision is still affected for that window.
3. Drive the handshake incrementally (`mbedtls_ssl_handshake_step()`) and feed between steps.
   Fully correct, most work.

Reviewer 3 argued hard against the blanket raise of `CONFIG_ESP_TASK_WDT_TIMEOUT_S` 5 → 20,
and they are right: with `CHECK_IDLE_TASK_CPU0/1=y` it quadruples the window in which a wedged
main loop or starved idle task goes undetected, on a device where `PANIC=y` was set
deliberately for OTA rollback. **A global raise is now the last resort, not the plan.**

### D3 — Failure becomes health state, with ownership of backoff stated

Per-source consecutive-failure count, last error, last success, state and next-attempt time,
with exponential backoff, a ceiling, and an explicit **recovery probe** at the ceiling so a
hub that reboots or a VLAN rule that gets fixed heals without intervention. Quarantine exit
must be defined, not implied.

`UpdateStatus` already distinguishes cases the scheduler should treat differently:
`AuthError` should quarantine hard and *not* retry on a timer; `RateLimited` should back off;
`FetchError` should retry. That differentiation is free and currently unused.

**Backoff ownership must be stated in `IDataSource.hpp`, or two layers compound.** The
reporter already runs 1 s → 60 s ×2 inside `WiserClient`; adding 30 s → 15 min at the
DataManager layer multiplies two independent policies. The split:

- **Framework owns scheduling backoff** — *when do we call `fetchUpdate()` again?*
- **Consumer owns transport retry** — *what happens inside one fetch?*

Consumers who already built scheduling backoff need to be told to remove it.

### D4 — Cancellation replaces `abort()`, and it is enabled by D1

`LWIP_NETCONN_FULLDUPLEX` is hardcoded to `1` in the IDF port, so closing a descriptor from
another task *does* return a blocked `recv()` — the mechanism is sound at the lwIP layer.

But **cancellation is not independently sequenceable**: nothing can reach that descriptor until
the framework owns the transport. And for TLS a bare `close()` is not enough — closing the fd
under a blocked handshake leaves the mbedtls session undefined. The framework-owned client must
expose **cancel-and-reset as a first-class operation** with a defined teardown path.

*Rejected:* recreating the worker task on deadline. `vTaskDelete()` on a task blocked in lwIP
leaks the socket and, for TLS, the mbedtls session (~30–40 KB) — trading an unrecoverable boot
loop for an unrecoverable heap leak.

### D5 — Health must reach the gates and the display, or this fix breaks the device

**The most important review finding, and it inverts part of the original design.**

`WiserHubConfigGate` derives its state from whether polling is progressing — `getLastPollMs()`
advancing, stamped on *every* attempt, success or failure. Under a naive D3, a quarantined
source stops being called at all, `lastPollMs` freezes at `0`, and the gate reports
**`InProgress` — "connecting…" — for the entire quarantine window.** On a first-run
misconfiguration that is a wall-mounted device saying "connecting" for fifteen minutes.

That is [DraytonWiserController#13](https://github.com/matthewmcneill/DraytonWiserController/issues/13)
verbatim — the issue that produced #128 in the first place. A health model that surfaces only to
the portal would fix the reported symptom and re-create its cause.

So health is not portal-facing. `IConfigGate`, the boot reporter and the display must all read
it, and **a source that has never succeeded must be distinguishable from one that has failed** —
the downstream principle, arrived at after three separate instances: *a state that has never
succeeded must be reported as loudly as one that has failed.* Silence and "still trying" are
indistinguishable to a user, and never-succeeded is the case where they can actually act.

### D6 — The mechanism that can reboot the device has zero test coverage

`startFetchWatchdog()`, `stopFetchWatchdog()` and `fetchWatchdogCb()` all sit inside
`#ifdef ESP_PLATFORM`. `processNext()` — the native seam — compiles the entire path out. The
748-test suite asserts nothing about the one code path whose failure mode is "reset the board".

The replacement must be built testable: the deadline supervisor calls an **injectable action**
rather than `abort()` directly, and takes its clock from `getCurrentTimeMs()`. "Deadline
exceeded → source quarantined" then becomes a native test, and binding that action to a reset
or a cancel becomes a one-line policy choice visible in review.

## Work Order Ledger

| WO | Branch | Complexity | Status | Summary |
|----|--------|-----------|--------|---------|
| **[ ] [wo1-race-fix-twdt-audit](wo1-race-fix-twdt-audit/WORKORDER.md)** | `128-datamanager-resilience/wo1-race-fix-twdt-audit` | standard | Pending | Fix the live abort-on-success race; audit every TWDT interaction, task budget and `CONFIG_ESP_TASK_WDT_*` across the framework |
| **[ ] [wo2-source-health-backoff](wo2-source-health-backoff/WORKORDER.md)** | `128-datamanager-resilience/wo2-source-health-backoff` | standard | Pending | Health state, `UpdateStatus`-differentiated policy, backoff + recovery probe + defined quarantine exit; backoff ownership documented |
| **[ ] [wo3-fetch-context-transport](wo3-fetch-context-transport/WORKORDER.md)** | `128-datamanager-resilience/wo3-fetch-context-transport` | architect | Pending | **BREAKING**: `fetchUpdate(const FetchContext&)` pure virtual; deadline/cancel token + framework-owned bounded client; WeatherModule migrated; cert-bundle hack deleted |
| **[ ] [wo4-cancellation-supervisor](wo4-cancellation-supervisor/WORKORDER.md)** | `128-datamanager-resilience/wo4-cancellation-supervisor` | architect | Pending | Cancel-and-reset with defined TLS teardown; retire `abort()` and the `esp_timer`; settle the handshake/TWDT mechanism on hardware |
| **[ ] [wo5-health-surfacing](wo5-health-surfacing/WORKORDER.md)** | `128-datamanager-resilience/wo5-health-surfacing` | architect | Pending | Health to `IConfigGate`, boot reporter, display, portal, MCP + EventBus; never-succeeded distinguishable from failed |
| **[ ] [wo6-job-fetch-isolation](wo6-job-fetch-isolation/WORKORDER.md)** | `128-datamanager-resilience/wo6-job-fetch-isolation` | standard | Pending | `urgent` jobs must not queue behind a scheduled fetch. Mechanism (second task vs scheduler deferral) decided after WO3 measures the real bound |
| **[ ] [wo7-docs-migration](wo7-docs-migration/WORKORDER.md)** | `128-datamanager-resilience/wo7-docs-migration` | standard | Pending | Rewrite the fetch contract; **downstream migration guide for the breaking change**; framework-expert skill |

**Base:** WO1 branches from `main` at `8f493a76`.

**Sequencing note:** cancellation (WO4) was originally listed as independent. Review established
it is the *payoff* of the transport change, not a peer of it — hence WO3 → WO4.

**Conflict advisory:** WO1, WO2, WO4 and WO6 all edit `DataManager.hpp`/`.cpp` and
`test/test_unit/test_dataManager/test_main.cpp`; WO2, WO3 and WO4 all edit
`include/interfaces/IDataSource.hpp`. The stack is **strictly serialised** — no parallelisation
is available without guaranteed conflicts.

## Verification

WO4 and WO5 cannot close on native tests alone. The reporter has offered their CrowPanel unit,
which reproduces the original 5 s boot loop deterministically across a VLAN boundary (packets
dropped, not refused) and **has never successfully reached its data source** — making it the
right adversarial test for the never-connected path in D5 specifically. Per
`.agents/rules/03-versioning.md`, flashing requires a `dev` tag → build → flash sequence.

## Modules & Components

| Module / Area | WOs | Notes |
|---|---|---|
| `modules/DataManager/` | WO1, WO2, WO4, WO6 | Race fix, health model, supervisor retirement, job isolation |
| `include/interfaces/IDataSource.hpp` | WO2, WO3, WO4 | `SourceHealthState`; `FetchContext`; pure-virtual signature change; backoff-ownership contract |
| New: framework fetch transport | WO3 | Bounded client + cert-bundle helper + cancel-and-reset. Placement (`DataManager/` vs shared `include/net/`) is a WO3 decision — OTA is a second potential consumer |
| `modules/OTAUpdateManager/` | WO1 (audit), WO3 (read-only) | Owns the second TWDT subscription; source of the feed-on-progress precedent. Its own unfed handshake window is observed, not in scope |
| `include/interfaces/IConfigGate.hpp` | WO5 | Must be able to read source health — D5 |
| `modules/EventBus/` | WO5 | `data.source.health` topic |
| `modules/WebServer/`, `modules/MCPServer/`, `web/portal/system.js` | WO5 | REST + MCP + portal card |
| `modules/LVGLDisplay/`, `modules/U8G2Display/` | WO5 | Degraded/quarantined card — a down source must not render as stale data |
| `sample-app/src/WeatherModule.*` | WO3 | Reference adopter for the new signature; deletes the private-symbol cert-bundle hack |
| `test/test_unit/test_dataManager/` | WO1, WO2, WO4, WO6 | Primary surface. D6: the supervisor must become natively testable via injectable action + `getCurrentTimeMs()` |
| `docs/developer/scheduling-data-fetches.md` | WO7 | 413-line guide; §1 method table and the duration contract both change |
| `.agents/skills/framework-expert/interfaces/data-source.md` | WO7 | Consumer-facing contract + migration guide |

## Session Log

### Session 1 — 2026-08-20 (Analysis & Decomposition)
- #128 architecture comment posted during the #136 close-out; this project is its execution.
- Established current implementation state from `origin/main` (local `main` was stale).
- Verified TWDT subscriber inventory: `esp_task_wdt_add()` appears only in `DataManager.cpp`
  and `OTAUpdateManager.cpp`; OTA holds its own independent subscription, so the
  `CONFIG_ESP_TASK_WDT_PANIC=y` rollback path from PR-14 is unaffected by changes here.
- Confirmed `DataManager` is a **core** module (`Application.hpp:532`), so surfacing needs no
  optional-module guards.
- Backoff should follow the `HomeAssistantModule` house precedent (`RETRY_BASE_MS` /
  `RETRY_CAP_MS`, reset-on-success), not invent a new shape.
- Found the feed-on-forward-progress precedent in `OTAUpdateManager.cpp:615-625`.
- Found the private-symbol cert-bundle hack in `WeatherModule.cpp:325-333`.
- First cut deferred the transport contract; human directed it back in scope as a live
  downstream blocker, then authorised outright API breakage.

### Session 2 — 2026-08-20 (Consumer review folded in)
Three reviews on #128, one from the consumer that filed it, verified against `origin/main`.
Changes to the plan:
- **New WO1.** A live bug: `esp_timer_stop()` does not wait for an already-dispatched callback,
  so `fetchWatchdogCb` can read `_fetchStartMs` in the window between `stopFetchWatchdog()` and
  `store(0)` and `abort()` a fetch **that already returned successfully**. Confirmed at both call
  sites (`DataManager.cpp:183-185`, and the `executeLoop()` site). Inverting the two lines makes
  the existing `if (start == 0) return;` guard cover it. Reviewer 2's "step 0 — audit TWDT
  interactions before changing them" folded into the same WO.
- **D5 added, and it changes the shape of the fix.** Quarantining a source freezes the signal
  `IConfigGate` uses for liveness, so a naive health model makes the display report "connecting…"
  for the whole quarantine window — re-creating DraytonWiserController#13, the issue that
  produced #128. Health surfacing was WO4-and-optional; it is now WO5-and-mandatory, spanning
  gates, boot reporter and display, not just the portal.
- **D6 added.** The entire reboot path is `#ifdef ESP_PLATFORM` and compiles out of the native
  seam — 748 tests assert nothing about it. Replacement must use an injectable action and
  `getCurrentTimeMs()`.
- **D1 rebalanced.** Reviewer 1: the deadline token must be primary because `IDataSource` is
  transport-agnostic. Reviewer 2: a token alone is the same trust contract we already have.
  Resolved by carrying both — cooperative floor for any transport, hard enforcement where the
  framework owns the pipe.
- **D2 hardened, and reviewer 3's mechanism corrected.** Feeding cannot come from the supervising
  timer (`esp_task_wdt_reset()` acts on the calling task) but *can* come from the framework
  transport running on the worker task — same outcome they wanted. Their argument against the
  global 5 → 20 raise is accepted: with idle-task checks enabled it weakens supervision
  everywhere, so it drops to last resort behind two narrower mechanisms.
- **D3 gained backoff-ownership.** The reporter already runs 1 s → 60 s inside `WiserClient`;
  two uncoordinated layers multiply. Framework owns scheduling, consumer owns transport retry,
  stated in the header. `UpdateStatus` drives differentiated policy (AuthError quarantines hard).
- **WO4 resequenced.** Cancellation is the payoff of the transport change, not a peer —
  nothing can reach the socket descriptor until the framework owns it, and TLS needs a defined
  teardown rather than a bare `close()`. `LWIP_NETCONN_FULLDUPLEX=1` confirmed in the IDF port,
  so the mechanism itself is sound.
- **WO6 softened.** Reviewer 2 asked whether a second task earns its stack and synchronisation
  cost once the fetch is bounded to ~10 s; reviewer 3 gave the concrete case (encoder dial turn →
  debounced PATCH → user waits). Kept, but the mechanism is chosen after WO3 measures the bound.
- **API break authorised**, removing the default-delegating overload, the legacy-source
  detection and the two-population watchdog policy from the previous draft.
