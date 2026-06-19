# PR-35: Weather Station & Home Assistant Module

This project replaces the framework's baseline configuration with a rich, canonical reference application: the Weather Station. It also introduces a native `esp-mqtt` Home Assistant module to the framework core, strictly adhering to the Hierarchical Object Model (HOM) and the 10ms tick budget.

**Implementation Plan:** [implementation_plan.md](../../../../../../../.gemini/antigravity/brain/be9f2248-18c8-4cdf-883a-0b8576e203a9/implementation_plan.md)
**Architecture Doc:** [home_assistant_architecture.md](../../../../../../../.gemini/antigravity/brain/be9f2248-18c8-4cdf-883a-0b8576e203a9/home_assistant_architecture.md)

## Work Orders

- **[ ] [wo1-home-assistant](wo1-home-assistant/WORKORDER.md):** Build the `HomeAssistantModule` (esp-mqtt wrapper, HOM entities, Web Portal tab, TLS).
- **[ ] [wo2-weather-module](wo2-weather-module/WORKORDER.md):** Build the `WeatherModule` (OpenWeatherMap JSON polling, KeyManager, HASensor registry).
- **[ ] [wo3-widgets-app](wo3-widgets-app/WORKORDER.md):** Build the U8G2 widgets, configure `CarouselEngine` in `main.cpp`, rename `template/` to `sample-app/`.
- **[ ] [wo4-ha-entity-parity](wo4-ha-entity-parity/WORKORDER.md):** Expand the entity catalog to full parity with `arduino_ha` (Lights, Climate, Fans, Covers, etc.).
