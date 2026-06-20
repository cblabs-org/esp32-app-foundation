# Work Order 3: UI Widgets & Application Assembly

**Objective:** Build the U8G2 display widgets and tie the application together in `main.cpp`.

**Tasks:**
1. Create `sample-app/src/WeatherWidget.hpp` and `.cpp` to draw the weather icon and temperature.
2. Create `sample-app/src/WeatherClockBoard.hpp` and `.cpp` (implementing `IDisplayBoard`).
3. Modify `sample-app/src/main.cpp`:
   - Statically instantiate `HomeAssistantModule` and `WeatherModule`.
   - Wire DI: `weatherModule.setHomeAssistant(&haModule)`.
   - Register both modules via `app.registerModule()`.
   - Configure the `CarouselEngine` to rotate between `WeatherClockBoard` and the native `DiagnosticsBoard`.
