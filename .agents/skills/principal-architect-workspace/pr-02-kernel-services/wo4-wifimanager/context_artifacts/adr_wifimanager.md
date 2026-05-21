# Architecture Decision Record: WiFi Manager

## Captive Portal fallback
When the device cannot connect to a configured WiFi Access Point after a timeout period, it spins up its own local Wi-Fi Access Point (AP) and launches a Captive Portal server. This allows users to configure the device credentials via a browser interface.

## AppState Lifecycle Coordination
The `WiFiManager` transitions the application's state from `WIFI_SETUP` to `APP_SETUP` once a successful network connection is established, or once AP mode is fully configured. This ensures downstream network-reliant components (like time synchronization) wait until connectivity is available.
