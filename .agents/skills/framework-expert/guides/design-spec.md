# Design Spec: Guided Application Design Consultation

> **Purpose:** Help a user shape their application concept, understand ESP32
> constraints, and produce a structured `docs/design-spec.md` artefact —
> informed by the full depth of the `framework-expert` skill reference library.
>
> **Trigger:** Read this guide whenever the user asks for help designing their
> application, creating a design specification, or thinking through what their
> app should do before writing code.

---

## Overview

This guide runs through six phases:

| Phase | Name | What happens |
|:---:|:---|:---|
| 1 | **Intake** | Accept the user's starting material |
| 2 | **Concept Exploration** | Draw out the full application picture |
| 3 | **Knowledge Loading** | Read relevant skill references to inform the spec |
| 4 | **Spec Generation** | Write `docs/design-spec.md` |
| 5 | **Review Loop** | Iterate until the user is satisfied |
| 6 | **Handoff** | Point to next steps |

> **Note on Phase 3 — Knowledge Loading:** This is the centrepiece of the
> guide. Do not write the spec from memory alone. Always read the relevant
> skill reference files first so the output reflects genuine framework
> expertise, not just what the user told you.

---

## Phase 1 — Intake

Determine what the user is starting with:

| Situation | Agent action |
|:---|:---|
| User provides a design brief or document (pasted text or file path) | Read it; extract key concepts; summarise your understanding back to the user; confirm before continuing |
| User has rough ideas but no document | Acknowledge and proceed to Phase 2 to draw them out |
| User gives a single-line description ("I want to build a weather station") | Use it as the seed concept; proceed to Phase 2 |

If the user provided a document, explicitly confirm:
> "I've read your brief. Here's what I've understood so far: [summary]. Is that
> right, and is there anything I've missed before we go deeper?"

---

## Phase 2 — Concept Exploration

Ask open, conversational questions to draw out the full picture. Work through
the themes below. You do not need to ask every question literally — adapt to
what the user has already told you and fill gaps naturally.

> **Tone:** Be curious and collaborative, not interrogative. The user may not
> know all the answers yet — "not sure yet" is a valid answer and should be
> noted as an open question in the final spec.

### Theme 1 — Problem & Purpose
- What problem does this device solve?
- What does "working well" look like for the person who uses it?
- Is this a one-off personal project, a production device, or something in between?

### Theme 2 — Operator Profile
- Who configures the device? A developer, or a non-technical end-user?
- Who monitors it day-to-day?
- Does it need to be usable by someone who has never seen the code?

### Theme 3 — Sensing & Actuation
- What does it measure, read, or receive? (sensors, GPIO inputs, serial data)
- What does it control or drive? (relays, servos, LEDs, displays)
- How frequently does it need to sample or update?

### Theme 4 — Data Lifecycle
- Where does captured data go? (displayed locally, sent to a remote API, published
  to MQTT, stored in LittleFS, broadcast via EventBus to other modules)
- How long does data need to be retained? Does it matter if it's lost on reboot?

### Theme 5 — Connectivity
- Is the device always connected to WiFi, or does it connect periodically?
- What happens when the network is unavailable — fail silently, queue, or alarm?
- Does it need to talk to external APIs? (note: TLS adds significant heap cost)

### Theme 6 — User Interaction
- How do people interact with the device? Display only? Web portal in a browser?
  Physical buttons? Voice? Mobile app?
- Does configuration need to survive a firmware update?

### Theme 7 — Update & Maintenance
- How does firmware get updated? USB cable, OTA push, or never?
- Who manages updates — the developer, or an end-user?
- Is rollback safety important?

### Theme 8 — Operating Environment
- Power source: mains-powered, battery, solar, PoE?
- Physical environment: indoor, outdoor, temperature extremes, enclosure?
- Number of devices: one prototype, or a fleet?

---

### Concept Summary

After working through the themes, synthesise answers into a **Concept Summary**
and confirm with the user:

> "Here's what I'm taking into the design. Let me know if anything is off or
> missing before I move on:"
>
> ```
> Application:  <name>
> Purpose:      <one sentence>
> Operator:     <who uses and configures it>
> Sensing:      <what it reads / controls>
> Data flow:    <where data goes>
> Connectivity: <always-on / periodic / offline>
> Interaction:  <display / portal / buttons / none>
> Updates:      <USB / OTA / none>
> Environment:  <power / physical context>
> Open Qs:      <anything the user wasn't sure about yet>
> ```

Wait for confirmation before proceeding to Phase 3.

---

## Phase 3 — Knowledge Loading

> **Do not skip this phase.** Read every file listed as applicable to the
> user's design. The quality of the spec depends on this step.

Using the confirmed Concept Summary, select and read the relevant references:

### Always read
- `references/architecture.md` — boot lifecycle, ADR-16 (static scope), 3-tier model
- `references/registration.md` — `registerModule()` rules and lifecycle

### Read based on design scope

| Design involves... | Read |
|:---|:---|
| Any interface decision | `interfaces/<interface>.md` for each relevant interface |
| Module structure / class design | `coding-patterns/overview.md` |
| Service module pattern | `coding-patterns/service-module-pattern.md` |
| Hierarchical sub-objects | `coding-patterns/hierarchical-object-model.md` |
| Any risk of polling / orchestration | `coding-patterns/common-anti-patterns.md` |
| Network fetches, blocking I/O | `embedded-safety/thread-safety.md` |
| Memory allocation decisions | `embedded-safety/memory-rules.md` |
| Battery / low-power operation | `embedded-safety/power-management.md` |
| Heap budget concerns | `performance/heap-management.md` |
| PSRAM-equipped board (S3-Nano) | `performance/psram.md` |
| Large binary / flash size worries | `performance/flash-size.md` |
| Display (any type) | `systems/display/overview.md`, `systems/display/custom-boards.md` |
| Non-OLED display | `systems/display/replacing-the-display.md` |
| Web portal or REST API | `systems/web-portal/overview.md`, `systems/web-portal/routes.md` |
| Portal styling / UI components | Sibling skill: `../../framework-design-system/readme.md` |
| Persistent settings | `systems/config/overview.md`, `systems/config/persistence.md` |
| EventBus pub/sub | `systems/event-bus/overview.md`, `systems/event-bus/custom-topics.md` |
| OTA firmware updates | `systems/ota/` (all files in directory) |
| Credential storage (API keys, tokens) | `security/credential-storage.md` |
| TLS / HTTPS connections | `security/tls-tradeoffs.md`, `performance/tls-memory-optimization.md` |
| Web portal authentication | `security/web-portal-auth.md` |
| OTA signing / anti-rollback | `security/ota-security.md` |
| MCP device tools | `systems/mcp-server/` (all files in directory) |

### After loading — flag conflicts

Review the loaded content against the Concept Summary and identify any
**design/constraint conflicts**. Surface these explicitly before writing the spec:

> "Before I write the spec, I've spotted a few things worth discussing:
>
> - [e.g.] You described polling a sensor every 50ms in the tick loop. The tick
>   budget is 10ms max (ADR-17). We should move sensor reads to a state machine
>   or IDataSource background task.
> - [e.g.] TLS for your remote API will consume ~40–60 KB of heap. On a WROOM-32
>   with 320 KB total, that's significant. Worth discussing the TLS tradeoffs.
> - [e.g.] You mentioned API keys — these need to go through IConfigurable +
>   DeviceCrypto, not hardcoded in source."

Resolve conflicts conversationally before proceeding to Phase 4. If a conflict
cannot be resolved at design time (e.g. a hardware or schedule constraint the
user cannot work around right now), do not block — note it as a **high-priority
item in Section 12 (Open Questions)** and continue to spec generation.

---

## Phase 4 — Spec Generation

Create `docs/design-spec.md` in the project root. If the directory does not
exist yet, create it first:
```bash
mkdir -p docs/
```
If a `docs/design-spec.md` already exists, offer to update it rather than
replace it.

Write the following as **plain markdown** directly to `docs/design-spec.md`.
Do not wrap it in a code block or add any preamble — the file should start
at `# <AppName>`. Every section should be substantive — populated with real
content from the conversation and the loaded references, not placeholder text.

---

```markdown
# <AppName> — Application Design Specification

_Generated by the ESP32 App Foundation framework-expert skill._
_Last updated: <date>_

---

## 1. Overview

<One paragraph: what this device does, the problem it solves, and what
success looks like. Written for someone unfamiliar with the project.>

---

## 2. Operator Profile

<Who configures, monitors, and maintains the device. Include technical
competency level — this informs web portal UX and config complexity decisions.>

---

## 3. Hardware Context

| Property | Value |
|:---|:---|
| Board | <Name + PlatformIO board ID> |
| Custom `board.h` | `hardware/<id>/board.h` (Author your own; do not use stock boards) |
| Flash | <size> |
| PSRAM | <size or none> |
| Display | <type + driver, or none> |
| Sensors / actuators | <list> |
| Power source | <mains / battery / solar / PoE> |
| Connectivity | <WiFi always-on / periodic / offline-first> |

---

## 4. Component Breakdown

<For each module class the design requires:>

| Module class | Interfaces | Responsibility |
|:---|:---|:---|
| `<ClassName>` | `ITickable`, `IConfigurable` | <What it does> |

<Narrative: explain why the design is decomposed this way. Reference the
Service Module Pattern and Hierarchical Object Model where applicable.>

---

## 5. Interface Selection Rationale

<For each interface selected, one paragraph explaining why this interface
is appropriate and what the module will do with it. Include any constraints
the developer must observe (tick budget, static scope, thread safety rules).>

### ITickable
<Rationale + tick budget constraint note>

### IConfigurable _(if selected)_
<Rationale + namespace + what settings are persisted>

### IWebContributor _(if selected)_
<Rationale + what endpoints and tabs are needed>

### IDataSource _(if selected)_
<Rationale + what background task it runs + thread safety notes>

### IDisplayBoard _(if selected)_
<Rationale + board name + what it renders>

### IPreconditionContributor _(if selected)_
<Rationale for precondition. Cause vocabulary selection. Role (Startup vs Operational). Dependency edges if any. describe() callback content.>

### IOTAVerifier _(if selected)_
<Rationale + signing scheme + what the verifier checks>

### IMCPContributor _(if selected)_
<Rationale + what JSON-RPC tools it registers + tool naming conventions>

### IStatusReporter _(if selected)_
<Rationale + what boot-phase status messages it reports + error conditions it signals>

> **Add a section for any other interface selected in Section 4 that is not
> listed above.**

---

## 6. Data Flow

<Narrative or ASCII diagram showing how data moves through the system.
From source (sensor / API / EventBus) through modules to destinations
(display / web portal / remote endpoint / LittleFS).>

Example flow:
```
[SensorModule.tick()] → reads GPIO
    → publishes "sensor.reading" on EventBus
        → [DisplayBoard.render()] consumes → shows on OLED
        → [WebContributor] caches → serves on GET /api/sensor
```

---

## 7. Configuration Schema

<For each IConfigurable module: settings it persists, types, defaults,
and any validation rules. This becomes the basis for the NVS/LittleFS
config file at /mod/<namespace>.json>

### `<ModuleNamespace>`

| Key | Type | Default | Notes |
|:---|:---|:---|:---|
| `apiKey` | string | `""` | Required; stored via DeviceCrypto |
| `intervalMs` | uint32 | `30000` | Polling interval in ms |

---

## 8. Web Portal Tabs _(if applicable)_

<For each IWebContributor: tab name, REST endpoints, what the UI shows.
All portal UI must use the project design system tokens and components
(see `../../framework-design-system/readme.md`).>

### `<TabName>` tab

- **Endpoint:** `GET /api/<path>` — returns `{ ... }`
- **UI:** Shows <what>; allows user to <action>
- **DS components used:** <list design system components, e.g. InfoRow, ToggleSwitch, PillBar>

---

## 9. Display Boards _(if applicable)_

<For each IDisplayBoard: board name, what it renders, update strategy.>

### `<BoardName>` board

- **Renders:** <description of what appears on screen>
- **Update strategy:** Subscribes to `<event.topic>` via EventBus; re-renders on change

---

## 10. External Dependencies

| Name | Source | Version | Purpose |
|:---|:---|:---|:---|
| `bblanchon/ArduinoJson` | PlatformIO registry | `^7.4.3` | JSON serialisation |
| `<other library>` | <source> | <version> | <purpose> |

---

## 11. Constraints & Risks

<Specific ESP32 constraints relevant to this design, with notes on how
the design addresses or accepts each one. Populated from Phase 3 knowledge
loading — not generic boilerplate.>

| Constraint | Impact | Mitigation |
|:---|:---|:---|
| Tick budget: 10ms (ADR-17) | <impact on this design> | <how addressed> |
| Heap: ~200 KB free on WROOM-32 | <impact> | <mitigation> |
| TLS heap cost: 40–60 KB | <impact if applicable> | <mitigation> |
| Static scope requirement (ADR-16) | All module instances must be global/static | Enforced in main.cpp scaffold |
| <other> | <impact> | <mitigation> |

---

## 12. Open Questions

<Anything unresolved that will need a decision before or during implementation.>

- [ ] <Question 1>
- [ ] <Question 2>
```

---

## Phase 5 — Review Loop

Present the generated spec to the user:

> "Your design spec is saved to `docs/design-spec.md`. Does anything look
> wrong, missing, or need expanding? I can revise any section — just tell me
> what to change."

Iterate until the user is satisfied. Common revision requests:
- Adding a module that was forgotten
- Adjusting the data flow
- Expanding a constraint note
- Resolving an open question that the user can now answer

---

## Phase 6 — Handoff

Once the user is happy with the spec, offer next steps based on context:

**If called from the bootstrap flow** (scaffold already exists):
> "Your spec is ready. Open `src/<AppName>.h` — the interfaces in Section 4
> are already stubbed out. Fill in `tick()` first, then layer in the other
> interfaces as you're ready. See `guides/app-developer-quickstart.md` for
> step-by-step walkthroughs of each one."

**If called standalone** (no scaffold yet):
> "Your spec is ready. Say 'bootstrap my project' and I'll set up the
> project structure and generate starter code aligned with this spec."

**Always:**
> "Commit `docs/design-spec.md` alongside your source code — it's a living
> document. Update it as the design evolves so it stays in sync with the
> implementation."
