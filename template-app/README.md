# App Template — Minimal Starting Point

> ESP32 App Foundation (c) 2026 CB Labs

A minimal, buildable skeleton for creating a new application on the ESP32 App Foundation. The `MyApp` module demonstrates all five primary extension points with clear, stub logic.

## What's Included

| File | Purpose |
|------|---------|
| `src/main.cpp` | Entry point — registers `MyApp` and wires framework services |
| `src/myApp.hpp` | Skeleton module header — implements 5 interfaces |
| `src/myApp.cpp` | Skeleton module implementation with stub logic |
| `platformio.ini` | Build configuration referencing the framework |

## Interfaces Demonstrated

| Interface | What It Does |
|-----------|-------------|
| `ITickable` | Periodic main-loop logic (interval-based timing) |
| `IConfigurable` | Persistent settings via ConfigManager |
| `IWebContributor` | REST API endpoints + web portal tab |
| `IMCPContributor` | MCP tool registration for AI agents |
| `IConfigGate` | Boot-time readiness gating |

## Getting Started

### Option A: Inside the Framework Repo

The template works as-is when located inside the framework repository (paths reference `../modules`, `../include`, etc.).

```bash
cd template-app
pio run -e esp32dev
```

### Option B: Standalone Project (Recommended)

1. Create a new repository for your project
2. Add the framework as a Git submodule:
   ```bash
   git submodule add https://github.com/matthewmcneill/esp32-app-foundation.git lib/esp32-app-foundation
   ```
3. Copy the contents of this directory to your project root
4. Update `platformio.ini` paths — replace `../` with `lib/esp32-app-foundation/`:
   ```ini
   build_flags =
       -I lib/esp32-app-foundation/include
   lib_extra_dirs =
       lib/esp32-app-foundation/modules
       lib/esp32-app-foundation/lib
   ```

## Next Steps

1. Rename `MyApp` to match your application
2. Remove interfaces you don't need (start with just `ITickable`)
3. Add your domain logic
4. Add interfaces progressively as your module grows

For a complete production example, see [`sample-app/`](../sample-app/) — a Weather Station that demonstrates `IDataSource`, `IDisplayBoard`, Home Assistant integration, and embedded JavaScript web UIs.

For the full developer guide, see [`docs/developer/sample-app-walkthrough.md`](../docs/developer/sample-app-walkthrough.md).
