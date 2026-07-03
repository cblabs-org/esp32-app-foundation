# Bootstrap: From Blank Folder to First Build

> **This guide is self-contained.** It is designed to work even when the rest
> of the `framework-expert` skill reference library is not yet present. All
> instructions needed to get from zero to a running project are embedded here.
> The full reference library will be acquired as part of Phase 3.

---

## Overview

This guide runs an agent through four phases:

| Phase | Name | What happens |
|:---:|:---|:---|
| 1 | **Orientation** | Detect whether the full skill tree is present |
| 2 | **Elucidation** | Ask the user questions to understand their project |
| 3 | **Acquisition** | Clone the framework; copy the skill tree; re-read references |
| 4 | **Scaffold** | Generate `platformio.ini`, `src/main.cpp`, `src/<AppName>.h` |

---

## Phase 1 — Orientation

Before asking the user anything, check whether the full skill reference tree
is available in this session's workspace.

**Detection check:** Attempt to read `references/architecture.md` (relative to
this file). If it **exists**, the full skill library is already present —
skip Phase 3 (no clone needed) and proceed directly from Phase 2 to Phase 4.

If it **does not exist**, this is a standalone / blank-folder session. Inform
the user:

> "I can see you're starting from a blank folder. I'll ask you a few questions
> about what you want to build, then I'll pull down the ESP32 App Foundation
> framework and generate a starter project for you."

---

## Phase 2 — Elucidation Interview

Ask the following questions. You may ask them all at once (using a
`ask_question` style multi-part prompt) or conversationally — whichever the
harness supports best. Collect all answers before proceeding.

> **Tone:** Be conversational and encouraging. The user may not know all the
> answers yet — that is fine. Accept "not sure" and default gracefully.

### Question Set

**Q1 — Intent (required)**
> "What do you want to build? Describe it in a sentence or two — don't worry
> about technical details yet."

*Purpose: primes domain vocabulary for the rest of the session.*

---

**Q2 — Board (required)**
> "Which ESP32 board are you targeting?"

Present the following known options; also accept any free-text ESP32 variant:

| Selection | PlatformIO board ID | Notes |
|:---|:---|:---|
| ESP32-WROOM-32 (generic) | `esp32dev` | Most common; 4 MB flash |
| Waveshare ESP32-S3-Nano | `um_nanos3` | 8 MB flash, QSPI PSRAM |
| Other ESP32 variant | *(free text)* | Agent will look up board ID |

If the user names a board not in this table, use the PlatformIO board registry
to find the correct `board` value:
```bash
pio boards --json-output | grep -i "<user's board name>"
```
Add newly confirmed board IDs to this table in future skill updates.

---

**Q3 — Display (optional, default: none)**
> "Does your device need a physical display? (e.g. OLED, TFT, e-ink, or none)"

| Answer | Implication |
|:---|:---|
| OLED (SSD1306/SSD1322) | Include `IDisplayBoard`; note U8G2 lib required |
| TFT | Include `IDisplayBoard`; note display driver dependency |
| e-ink | Include `IDisplayBoard`; note slow refresh constraints |
| None / not sure | Omit display scaffolding |

---

**Q4 — Data source (optional, default: local sensor)**
> "Will your device fetch data from the internet, read from local sensors,
> or both?"

| Answer | Implication |
|:---|:---|
| Remote API / internet | Include `IDataSource` stub (background fetch task) |
| Local sensors only | `ITickable` tick loop is sufficient |
| Both | Include both |
| Not sure yet | Default to `ITickable` only; add comment hint |

---

**Q5 — User settings (optional, default: no)**
> "Will users need to configure anything — like API keys, thresholds, or
> device names — that should survive a reboot?"

| Answer | Implication |
|:---|:---|
| Yes | Include `IConfigurable` |
| No / not sure | Omit; add comment hint |

---

**Q6 — Web portal (optional, default: no)**
> "Do you want a browser-based web portal so users can view status or change
> settings over WiFi?"

| Answer | Implication |
|:---|:---|
| Yes | Include `IWebContributor` stub |
| No / not sure | Omit; add comment hint |

---

**Q7 — OTA updates (optional, default: no)**
> "Do you want the ability to update firmware over the air (OTA), without
> physically connecting the device?"

| Answer | Implication |
|:---|:---|
| Yes | Add OTA config note to scaffold + point at `systems/ota/` after clone |
| No / not sure | Omit; add comment hint |

---

**Q8 — Anything else? (optional)**
> "Is there anything else you know you'll need? (e.g. Bluetooth, MQTT,
> a specific sensor library, MCP device tools)"

*Parse free text; note any keywords for the scaffold comments.*

---

### Project Summary Card

After collecting all answers, synthesise and display a summary for the user
to confirm before proceeding:

```
Project Summary
───────────────────────────────────────────────
Name:      <derived from Q1, e.g. "WeatherStation">
Board:     <Q2 answer + PlatformIO board ID>
Display:   <Q3 answer>
Data:      <Q4 answer>
Settings:  <Q5 answer>
Web portal: <Q6 answer>
OTA:       <Q7 answer>
Notes:     <Q8 free text if any>
───────────────────────────────────────────────
Shall I go ahead and set up the project?
```

**Wait for explicit confirmation** before proceeding to Phase 3.

---

## Phase 3 — Repo Acquisition

> Skip this phase entirely if Phase 1 detected that the full skill tree is
> already present.

### Step 1 — Clone the framework

```bash
git clone https://github.com/matthewmcneill/esp32-app-foundation.git framework/
```

This places the entire framework repository into a `framework/` subdirectory
of the user's project. This is intentional — the user owns this directory,
can modify framework source, and can pull upstream changes at any time with
`git pull` inside `framework/`.

### Step 2 — Copy the skill tree into the project

```bash
mkdir -p .agents/skills
cp -r framework/.agents/skills/framework-expert .agents/skills/framework-expert
```

This makes the full skill reference library available in the project's own
`.agents/` tree, so future agent sessions do not need to navigate into
`framework/` to find them. The skill files inside `framework/` remain as
the canonical copy; the copied tree is a convenience snapshot for the agent
harness.

> **Note:** If you later pull upstream framework changes that include skill
> updates, re-run the `cp` command above to refresh the snapshot.

### Step 3 — Re-read the skill

Now that the full reference library is available, read the following files
to refresh in-context knowledge before generating the scaffold:

- `references/architecture.md` — boot lifecycle, ADR-16 (static scope)
- `references/registration.md` — `registerModule()` rules
- `guides/app-developer-quickstart.md` — interface code patterns

Proceed to Phase 4 once read.

---

## Phase 4 — Scaffold Generation

Generate the following three files in the user's project root. Base all
decisions on the confirmed answers from Phase 2.

### Naming convention

Derive a `<AppName>` class name from Q1. Use PascalCase, strip spaces and
special characters. Examples:
- "weather station" → `WeatherStation`
- "plant watering controller" → `PlantController`
- "air quality monitor" → `AirQualityMonitor`

---

### File 1 — `platformio.ini`

Use the board ID from Q2. Emit the environment block(s) appropriate for the
selected board. The `lib_extra_dirs` line points at the cloned `framework/`
directory so `#include "AppFoundation.h"` resolves at build time.

**Template (adapt per board):**

```ini
; =============================================================================
; <AppName> — PlatformIO Configuration
; Built on ESP32 App Foundation (https://github.com/matthewmcneill/esp32-app-foundation)
; =============================================================================

[platformio]
default_envs = <board_env>

; ---------------------------------------------------------------------------
; Shared base
; ---------------------------------------------------------------------------
[env:base_esp32]
platform              = espressif32
framework             = arduino, espidf
monitor_speed         = 115200
upload_speed          = 460800
build_flags           =
    -DCORE_DEBUG_LEVEL=3
    -DCONFIG_ASYNC_TCP_RUNNING_CORE=1
    -DCONFIG_ASYNC_TCP_STACK_SIZE=4096
    -DAPP_TITLE=\"<AppName>\"
    -DSECURITY_VERSION=1
    -std=gnu++17
    -I include
    -I framework/include
build_unflags         = -std=gnu++11
lib_compat_mode       = strict
lib_extra_dirs        =
    framework/modules
    framework/lib
extra_scripts         = pre:framework/scripts/inject_version.py

; ---------------------------------------------------------------------------
; <board_env> — <Board human name>
; ---------------------------------------------------------------------------
[env:<board_env>]
extends               = env:base_esp32
board                 = <pio_board_id>
board_build.partitions = <partitions_file>
board_build.filesystem = littlefs
build_flags           =
    ${env:base_esp32.build_flags}
    -DBOARD_ID=\"<board_env>\"
    <display_flags_if_applicable>
lib_deps              =
    bblanchon/ArduinoJson@^7.4.3
    hoeken/PsychicHttp@^3.0.0
    olikraus/U8g2@^2.36.18
    squix78/JsonStreamingParser@^1.0.5
```

**Known board substitution values:**

| Board | `<board_env>` | `<pio_board_id>` | Partitions file |
|:---|:---|:---|:---|
| ESP32-WROOM-32 | `esp32dev` | `esp32dev` | `framework/partitions.esp32dev.csv` |
| ESP32-S3-Nano | `esp32s3nano` | `um_nanos3` | `framework/partitions.esp32s3nano.csv` |

For any other board: set `<board_env>` to a snake_case version of the board
name and look up `<pio_board_id>` from the PlatformIO registry.

> **Display flags:** If Q3 selected a display, copy the appropriate
> `-DU8G2_DISPLAY_TYPE` and pin flags from the matching environment block in
> `framework/platformio.ini`. If no display, omit the U8g2 lib_dep and the
> display flags.

---

### File 2 — `src/main.cpp`

```cpp
// =============================================================================
// <AppName> — main.cpp
// ESP32 App Foundation application entry point.
//
// All module instances MUST be declared in global/static scope (ADR-16).
// The framework holds raw pointers to these objects for the device lifetime.
// =============================================================================
#include "AppFoundation.h"
#include "<AppName>.h"

// ---------------------------------------------------------------------------
// Module instances (global scope — do NOT move inside setup())
// ---------------------------------------------------------------------------
Application     app;
<AppName>       myApp;

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    app.registerModule(&myApp);

    FrameworkConfig config;
    config.appTitle = APP_TITLE;
    app.begin(config);
}

void loop() {
    app.tick();
}
```

---

### File 3 — `src/<AppName>.h`

Generate the class header. Include **only** the interfaces selected during
elucidation. For each interface the user did **not** select, emit a commented-out
block with a one-line hint pointing at the relevant quickstart step.

**Full template (agent: uncomment/remove as appropriate):**

```cpp
// =============================================================================
// <AppName>.h
// =============================================================================
#pragma once
#include "AppFoundation.h"

class <AppName>
    : public ITickable            // Always included — periodic logic
// , public IConfigurable         // Uncomment for persistent user settings  → quickstart Step 3
// , public IWebContributor       // Uncomment to add a web portal tab        → quickstart Step 4
// , public IDataSource           // Uncomment for background network fetches → quickstart Step 5
// , public IDisplayBoard         // Uncomment to render a display slide      → quickstart Step 6
{
public:
    // -------------------------------------------------------------------------
    // ITickable — called every main-loop cycle. MUST return within 10 ms.
    // -------------------------------------------------------------------------
    void tick(uint32_t currentMillis) override {
        // TODO: Add your periodic logic here.
        // - Read sensors
        // - Update state
        // - Publish events via EventBus
        // DO NOT: make blocking network calls, delay(), or hold mutexes here.
    }

// ---- IConfigurable (uncomment if selected) ----------------------------------
//  void serializeConfig(JsonObject& section) const override {
//      section["mySetting"] = m_mySetting;
//  }
//  void applyConfig(const JsonObject& section) override {
//      m_mySetting = section["mySetting"] | "default";
//  }

// ---- IWebContributor (uncomment if selected) --------------------------------
//  void registerWebEndpoints(PsychicHttpServer* server) override {
//      server->on("/api/<appname>/status", HTTP_GET, [this](PsychicRequest* req) {
//          return req->reply(200, "application/json", "{\"ok\":true}");
//      });
//  }

// ---- IDataSource (uncomment if selected) ------------------------------------
//  UpdateStatus performUpdate() override {
//      // Runs in a background FreeRTOS task. Safe to block for network I/O.
//      // Update shared state, then return UpdateStatus::SUCCESS or FAILED.
//      return UpdateStatus::SUCCESS;
//  }

// ---- IDisplayBoard (uncomment if selected) ----------------------------------
//  const char* getBoardName() const override { return "<AppName>"; }
//  void render(U8G2& display) override {
//      display.drawStr(0, 10, "Hello from <AppName>");
//  }

private:
    // TODO: Add member variables here.
    // uint32_t m_lastUpdate = 0;
    // String   m_mySetting;
};
```

---

### Handoff message

After generating all three files, display this message to the user:

```
✅ Project scaffold ready!

Files created:
  platformio.ini         — build configuration
  src/main.cpp           — framework wiring
  src/<AppName>.h        — your module (add logic here)

Framework cloned to:   framework/
Skill library copied to: .agents/skills/framework-expert/

Next steps:
  1. Open src/<AppName>.h and fill in your tick() logic.
  2. Read guides/app-developer-quickstart.md for step-by-step interface walkthroughs.
  3. Build with:  pio run -e <board_env>
  4. To pull future framework updates:  cd framework && git pull
```

---

### Design spec invitation

After displaying the handoff message above, append this soft invitation:

```
💡 Want to think through your application design before writing code?
   Say "help me design my app" and I'll guide you through a structured
   design session — exploring your use case, surfacing ESP32 constraints,
   and producing a design spec saved to docs/design-spec.md.
   (Takes about 10–15 minutes and can save a lot of rework later.
   Easy to skip if you'd rather dive straight in.)
```

---

## Known Boards Registry

This table is the canonical source for the elucidation board question. Add
new confirmed entries here when supporting additional ESP32 variants.

| Board name (user-facing) | PlatformIO `board` ID | Flash | PSRAM | Notes |
|:---|:---|:---|:---|:---|
| ESP32-WROOM-32 (generic) | `esp32dev` | 4 MB | Optional | Most common dev board |
| Waveshare ESP32-S3-Nano | `um_nanos3` | 8 MB | 8 MB QSPI | Requires `board_build.arduino.memory_type = qio_opi` |
