# State: Work Order 0 - Portal Prep & Template Rename

- **Template Rename**: The `template/` directory has been renamed to `sample-app/`. References in `README.md`, `platformio.ini`, `main.cpp`, `myApp.cpp`, and `myApp.hpp` were updated.
- **Credits Modal**: The credits tab was removed from the bottom-nav array in `web/portal/index.html`. An info icon was added to the header, launching a `<dialog>` modal which hosts the `tab-credits` div. `credits.js` dynamically populates this modal.
- **SDKConfig**: `CONFIG_MQTT_PROTOCOL_311=y` added to `sdkconfig.defaults`, along with buffer and stack size defaults.
- **AppTabContributor**: Gated via `_enableDefaultAppTab` in `Application.hpp` and `.cpp`.
