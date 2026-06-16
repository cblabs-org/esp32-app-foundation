# Display Subsystem Overview

The App Foundation uses a decoupled, three-layer display rendering architecture. By default, it ships with a U8G2 reference implementation for monochromatic OLEDs, but the architecture explicitly supports headless operation or swapping to TFT/e-ink displays.

## Reference Implementation: U8G2

The framework provides a `U8g2DisplayManager` out of the box. This manager owns the U8G2 instance and orchestrates the rendering loop. It handles the I2C/SPI bus setup for the display and manages the screen refresh cycle.

### Two-Phase Bootstrap

Display initialization follows a two-phase bootstrap process to ensure safe operation before the config system is fully online:

1. **Pre-Begin Bootstrap**: The display hardware is initialized extremely early in `Application::begin()`. This allows the framework to show boot progress, OTA status, and critical failure screens even if the file system or config registry fails to load.
2. **Post-Begin Normal Operation**: Once all modules are registered and the system enters its main `tick()` loop, the display transitions to "normal" mode where it renders the application's UI, the carousel, or the screensaver.

## Multi-Interface Pattern

The display subsystem interacts with modules via several specialized interfaces:

- **[`IDisplayBoard`](../../interfaces/display-board.md)**: Allows modules to contribute "boards" (panes/screens) to the `CarouselEngine`.
- **`IStatusContributor`**: Allows modules to push status text or icons to the system status bar (if implemented by the display manager).
- **`IScreensaverAware`**: Allows modules to react when the display goes to sleep or wakes up.

## Application Ownership

The `Application` class retains ultimate ownership of the display hardware instance. The `U8g2DisplayManager` is a framework-level service, but its lifecycle is explicitly bound to the `Application` so that it can render panic screens during `AppFoundation::panic()`.

## Three-Layer Rendering

The display rendering pipeline is strictly separated into three layers to ensure that UI components don't overwrite each other unpredictably:

1. **Background Layer**: Managed by the `ScreensaverController` or static background elements. If the screen is sleeping, rendering stops here.
2. **Content Layer**: This is the primary viewport. The `CarouselEngine` renders the currently active `IDisplayBoard` into this space. The board has full control over the content area.
3. **Overlay Layer**: The `U8g2DisplayManager` renders system overlays on top of the content. This includes:
   - OTA progress bars (highest priority, blocks content)
   - Connection status icons (WiFi, MQTT)
   - System popups or toasts

Modules should never attempt to draw outside their allocated content bounds or assume they have exclusive control of the display buffer unless they are implementing a specialized overlay.
