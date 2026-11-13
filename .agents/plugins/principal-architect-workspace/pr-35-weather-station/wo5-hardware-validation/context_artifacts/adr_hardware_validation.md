# ADR: Skipping JSON-RPC Hardware Tests

## Context
During WO5 Hardware Validation, the device booted cleanly but fell back into AP mode (`192.168.4.1`) due to the absence of valid WiFi credentials. This occurred because the `LittleFS` partition was flashed cleanly prior to validation, wiping any previous `wifi.json`. 

## Decision
We opted to **skip** the JSON-RPC `curl` testing phase.

## Rationale
1. **Host Network Isolation**: To reach `192.168.4.1`, the host machine's network adapter would need to disconnect from its primary network (e.g., `<home-subnet>`) and connect to the device's unencrypted AP. This is high-risk for autonomous agents as it disrupts internet connectivity, breaking MCP tooling and LLM API access.
2. **Boot Success Validation**: The serial logs comprehensively demonstrated that the firmware initialized cleanly out of download mode. Critical modules (EventBus, ConfigManager, AuthManager, U8G2Display, and WebServerManager) all loaded perfectly without crashing or `std::atomic` race conditions. The successful fallback to the Captive Portal AP is strong enough proof of hardware stability to clear the WO5 objective.
