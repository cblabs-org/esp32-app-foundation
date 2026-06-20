# Work Order 0: Portal Prep & Template Rename

**Objective:** Rename the template directory and relocate the Credits tab to free up a bottom-nav slot for Home Assistant.

**Tasks:**
1. Rename `template/` to `sample-app/`. Update all references in `README.md`, `platformio.ini`, and `.github/workflows`.
2. Remove the "Credits" bottom-nav tab from the portal.
3. Add an ℹ️ info icon button to the top-right of the portal header bar. Clicking it opens a modal overlay rendering the existing `credits.js` card content.
4. Enable `esp-mqtt` in `sdkconfig.defaults` (`CONFIG_MQTT_PROTOCOL_311=y` and related buffer/task settings). Document the RAM cost impact.
5. Gate or remove the internal `AppTabContributor` stub in `Application.hpp` so it does not conflict with application-provided tabs.
