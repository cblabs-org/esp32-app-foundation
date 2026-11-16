# App Template — Minimal Starting Point

> ESP32 App Foundation (c) 2026 CB Labs

A minimal, buildable skeleton for creating a new application on the ESP32 App Foundation. The `MyApp` module demonstrates all five primary extension points with clear, stub logic.

## What's Included

| File | Purpose |
|------|---------|
| `src/main.cpp` | Entry point — registers `MyApp` and wires framework services |
| `src/MyApp.hpp` | Skeleton module header — implements 5 interfaces |
| `src/MyApp.cpp` | Skeleton module implementation with stub logic |
| `sdkconfig.defaults*` | ESP-IDF configuration defaults for the build |
| `platformio.ini` | Build configuration referencing the framework |

## Interfaces Demonstrated

| Interface | What It Does |
|-----------|-------------|
| `ITickable` | Periodic main-loop logic (interval-based timing) |
| `IConfigurable` | Persistent settings via ConfigManager |
| `IWebContributor` | REST API endpoints + web portal tab |
| `IMCPContributor` | MCP tool registration for AI agents |
| `IPreconditionContributor` | Operational readiness precondition |

> **Note:** The sample-app demonstrates two additional interfaces not shown here: `IDataSource` (structured data providers) and `IDisplayBoard` (OLED display contributions). See [`sample-app/`](../sample-app/) for a production example.

## Getting Started

### Option A: Inside the Framework Repo

The template works as-is when located inside the framework repository (paths reference `../modules`, `../include`, etc.).

```bash
cd template-app
pio run -e esp32dev
```

### Option B: Standalone Project (Recommended)

1. Create a new repository and add the framework as a Git submodule at `framework/`:
   ```bash
   mkdir my-device && cd my-device && git init
   git submodule add https://github.com/cblabs-org/esp32-app-foundation.git framework
   ```
2. Copy the contents of this directory to your project root:
   ```bash
   cp -r framework/template-app/. .
   ```
3. Point the `../` paths in `platformio.ini` at `framework/`. Leave the
   `BOARD_HEADER` lines alone: that path is resolved from the framework's
   `hardware/` folder, so it is already correct.
   ```bash
   sed -i.bak '/BOARD_HEADER/!s#\.\./#framework/#g' platformio.ini && rm platformio.ini.bak
   ```
4. Commit, then build. The ESP-IDF build reads the git revision and fails in a
   repository with no commits.
   ```bash
   git add -A && git commit -m "Start from the App Foundation template"
   pio run -e esp32dev
   ```

## Build Configuration

> **Note on Hardware Configuration:** This template uses the framework's stock board headers as working examples. When building your own consumer app, you should **author your own `board.h`** rather than relying on stock boards, which are only reference implementations. Your custom board header is the single source of truth for all your application's pins. See [`docs/developer/custom-boards.md`](../docs/developer/custom-boards.md).

The template ships its own `sdkconfig.defaults` files, passed to ESP-IDF through `board_build.cmake_extra_args`. The framework's `sync_sdkconfig.py` pre-build script keeps the generated `sdkconfig.<env>` files in step with them.

## Next Steps

1. Rename `MyApp` to match your application
2. Remove interfaces you don't need (start with just `ITickable`)
3. Add your domain logic
4. Add interfaces progressively as your module grows

For a complete production example, see [`sample-app/`](../sample-app/) — a Weather Station that demonstrates `IDataSource`, `IDisplayBoard`, Home Assistant integration, and embedded JavaScript web UIs.

For the full developer guide, see [`docs/developer/sample-app-walkthrough.md`](../docs/developer/sample-app-walkthrough.md).
