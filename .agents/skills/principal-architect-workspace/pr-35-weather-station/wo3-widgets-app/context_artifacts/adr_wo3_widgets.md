# ADR: WO3 UI Widgets & Application Assembly

## Context
We needed to implement a `WeatherWidget` and `WeatherClockBoard` (implementing `IDisplayBoard`) to display the data fetched by `WeatherModule` and integrate it into the `CarouselEngine`.

## Decision
1. **Decoupled Widget**: `WeatherWidget` is a stateless helper class (`static void draw()`). This decouples the U8G2 layout code from the `IDisplayBoard` interface implementation.
2. **Data Access**: Rather than coupling `WeatherWidget` or `WeatherClockBoard` tightly to `HaSensor` instances, we added simple `const std::string&` getters (`getTemperature`, `getWindSpeed`, `getCondition`) to `WeatherModule`. This aligns with the "pull" model where the board pulls data during `render()`.
3. **Carousel Integration**: `WeatherClockBoard` was statically instantiated in `main.cpp` and added to `app.getDisplay().carousel`. `DiagnosticsBoard` is already internally registered by `Application.cpp`, so adding `WeatherClockBoard` causes them to rotate seamlessly.
