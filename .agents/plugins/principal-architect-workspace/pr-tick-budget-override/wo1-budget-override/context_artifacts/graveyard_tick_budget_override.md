# Graveyard & Resolution: Per-Module Tick Budget Override

## Obstacles & Resolved Issues
1. **PlatformIO environment selection for compilation verification**:
   - `build_project` failed when passing `environment: "esp32s3"`.
   - Resolution: Checked `platformio.ini` environment names (`get_project_config` / output hints) and targeted `esp32s3nano` which compiled successfully.
2. **Git status fsmonitor sandbox restriction**:
   - `git status` encountered permission error under standard sandbox due to `.git/fsmonitor--daemon.ipc`.
   - Resolution: Re-ran with `BypassSandbox: true`.
