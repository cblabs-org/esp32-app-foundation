# Library.json Manifest — Post-Fix State

All modules and libraries now have `library.json` files. This is the authoritative list.

## modules/ (with `-I../../include` build flag)

| Module | Dependencies | Extra |
|--------|-------------|-------|
| Application | EventBus, DataManager, WebServer, ConfigManager, WifiManager, KeyManager, AuthManager, DeviceCrypto | `extraScript: exclude_on_native.py` |
| AuthManager | *(none)* | |
| ButtonModule | Application, ArduinoJson | *(pre-existing)* |
| ConfigManager | ConfigManifest | |
| DataManager | *(none)* | |
| EncoderModule | Application, ArduinoJson | *(pre-existing)* |
| EventBus | *(none)* | |
| HomeAssistant | *(none)* | |
| KeyManager | DeviceCrypto, ConfigManager | |
| LVGLDisplay | ArduinoJson, lvgl | `extraScript: exclude_ui_on_native.py` *(pre-existing)* |
| MCPServer | *(none)* | |
| OTAUpdateManager | Application, OTACerts, OTADownloadEngine, OTAManifest, MCPServer | |
| TimeManager | *(none)* | |
| U8G2Display | *(none)* | *(pre-existing, no deps)* |
| WebServer | *(none)* | |
| WifiManager | *(none)* | |

## lib/ (no build flags needed)

| Library | Dependencies |
|---------|-------------|
| ConfigManifest | *(none)* |
| DeviceCrypto | *(none)* |
| DisplayServiceHelper | *(none)* |
| Logger | *(none)* |
| MD5Utils | *(none)* |
| OTACerts | *(none, pre-existing)* |
| OTADownloadEngine | *(none, pre-existing)* |
| OTAManifest | *(none)* |
| PngWriter | *(none, pre-existing)* |
| SemVer | *(none)* |
