# PR-35: Weather Station & Home Assistant Module

This project replaces the framework's baseline configuration with a rich, canonical reference application: the Weather Station. It also introduces a native `esp-mqtt` Home Assistant module to the framework core, strictly adhering to the Hierarchical Object Model (HOM) and the 10ms tick budget.

**Implementation Plan:** [implementation_plan.md](file:///Users/mcneillm/.gemini/antigravity/brain/be9f2248-18c8-4cdf-883a-0b8576e203a9/implementation_plan.md)
**Architecture Doc:** [home_assistant_architecture.md](file:///Users/mcneillm/.gemini/antigravity/brain/be9f2248-18c8-4cdf-883a-0b8576e203a9/home_assistant_architecture.md)
**Pre-Execution Review:** [pr35_review.md](file:///Users/mcneillm/.gemini/antigravity/brain/be9f2248-18c8-4cdf-883a-0b8576e203a9/pr35_review.md)

## Work Orders

- **[x] [wo0-portal-prep](wo0-portal-prep/WORKORDER.md):** Rename `template/` → `sample-app/`, relocate Credits tab to header ℹ️ icon modal, enable `esp-mqtt` in sdkconfig, gate `AppTabContributor`. **[MERGED]**
- **[x] [wo1-home-assistant](wo1-home-assistant/WORKORDER.md):** Build the `HomeAssistantModule` (esp-mqtt wrapper, HOM entities, LWT, reconnect sync, Web Portal tab, MCP tools, unit tests). **[MERGED]**
- **[x] [wo2-weather-module](wo2-weather-module/WORKORDER.md):** Build the `WeatherModule` (OpenWeatherMap JSON polling, KeyManager, HASensor registry, "Weather" portal tab with sub-pills). **[MERGED]**
- **[x] [wo3-widgets-app](wo3-widgets-app/WORKORDER.md):** Build U8G2 widgets (`WeatherWidget`, `WeatherClockBoard`), wire DI and `CarouselEngine` in `main.cpp`. **[MERGED]**
- **[ ] [wo4-ha-entity-parity](wo4-ha-entity-parity/WORKORDER.md):** Expand the entity catalog to full parity with `arduino_ha` (Lights, Climate, Fans, Covers, etc.).
