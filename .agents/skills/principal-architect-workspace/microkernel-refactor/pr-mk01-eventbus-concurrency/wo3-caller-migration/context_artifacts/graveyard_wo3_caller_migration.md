# Graveyard & Troubleshooting Log: WO3 Caller Migration

## 1. Stack-Allocated Payload Pointer Lifetime Hazard
- **Attempt**: Leaving `OTAProgressEvent event` and `AppConfiguringPayload payload` as local stack variables inside `publishProgress()` and `_publishConfiguringEvent()`.
- **Why it Failed**: In host GoogleTest builds, `EventBus::publish()` executes subscribers synchronously, masking stack pointer lifetime bugs. On target, `esp_event` posts asynchronously to the event queue. By the time `esp_event` task executes `event_trampoline`, the stack frame has exited, resulting in memory corruption/garbage payload data.
- **Resolution**: Promoted all EventBus pointer payloads to persistent class member variables.

## 2. PlatformIO MCP Server Concurrent Lock Conflicts
- **Symptom**: `build_project` and `run_tests` returned "A build is already actively running for this project" or "Hardware is currently tied up".
- **Root Cause**: Host OS background process `pio test` (PID 48764 / PID 50653) held file locks on the platformio workspace from prior executions outside MCP.
- **Resolution**: Cleared process lock via `reset_server_state` and killed stuck host `pio` processes, then dispatched tasks asynchronously using `background: true` parameter per `pio-manager` skill rules.
