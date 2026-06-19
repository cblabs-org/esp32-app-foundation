# Work Order 3: UI Widgets & Application Assembly

**Objective:** Build the U8G2 display widgets and tie the application together.

**Tasks:**
1. Rename the `template/` directory to `sample-app/`. Update all references in `README.md`, `platformio.ini`, and `.github/workflows`.
2. Create `sample-app/src/WeatherWidget.hpp` and `.cpp` to draw the weather icon.
3. Create `sample-app/src/WeatherClockBoard.hpp` and `.cpp` (implementing `IDisplayBoard`).
4. Modify `sample-app/src/main.cpp`:
   - Register `HomeAssistantModule` and `WeatherModule`.
   - Configure the `CarouselEngine` to rotate between `WeatherClockBoard` and the native `DiagnosticsBoard`.
   - Remove the generic "App" tab from the portal and replace it with the "Weather" tab.
