# State — Display Module Target API

**Canonical reference**: `docs/design_specification.md §7.8`
**Session**: 0735b228-f5fb-4a7a-b7e7-4e2807bebae0

> Read `docs/design_specification.md §7.8` for the authoritative spec. This artifact
> captures the API contract in a compact form for fast-loading by worker agents.

---

## Public Sub-Object Structure

```
U8G2DisplayModule (root)
 ├── CarouselEngine    carousel           // manages rotating board pool
 ├── BoardCatalog      boards             // named concrete board references
 │    ├── BootBoard&            boot
 │    ├── WiFiWizardBoard&      wifiWizard
 │    ├── OTAProgressBoard&     otaProgress
 │    └── ErrorBoard&           error
 └── ScreensaverController screensaver    // display-intrinsic idle policy
```

## Root Module API

```cpp
// Presentation commands (IDisplayBoard* — polymorphic boundary):
void showModal(IDisplayBoard* board);
void clearModal();
void showBase(IDisplayBoard* board);
void resetActivity();   // delegates to screensaver

// Layer render priority (highest → lowest):
// MODAL → BASE/CAROUSEL → SCREENSAVER
```

## CarouselEngine API

```cpp
void add(IDisplayBoard* board);
void show(IDisplayBoard* board);     // force a specific board
void suspend();
void resume();
void setInterval(uint32_t ms);
void tick(uint32_t currentMillis);
IDisplayBoard* currentBoard() const;
```

## BoardCatalog API

```cpp
// Concrete typed references — callers get full domain API:
BootBoard&              boot;
WiFiWizardBoard&        wifiWizard;
OTAProgressBoard&       otaProgress;
ErrorBoard&             error;
```

## ScreensaverController API

```cpp
enum class ScreensaverPolicy { DimOnly, FullSleep };

void setDimTimeout(uint32_t ms);
void setBlankTimeout(uint32_t ms);
void setDimLevel(uint8_t brightness);        // 0–255
void setPolicy(ScreensaverPolicy policy);
void setBoard(IDisplayBoard* board);         // optional: what renders during screensaver
void resetActivity();
bool isActive() const;
bool isDimmed() const;
bool isBlanked() const;
void tick(uint32_t currentMillis, U8G2& display);
```

## Canonical Call Patterns

```cpp
// Configure a board (caller uses concrete type):
display.boards.otaProgress.setProgress(0.5f, "Downloading...");
display.boards.wifiWizard.configure(hostname);

// Surface a board (root module, IDisplayBoard* boundary):
display.showModal(&display.boards.otaProgress);
display.clearModal();
display.showBase(&display.boards.configuringStatus);

// Carousel:
display.carousel.add(&myAppBoard);
display.carousel.suspend();
display.carousel.resume();

// Screensaver:
display.screensaver.setDimTimeout(60000);
display.screensaver.setPolicy(ScreensaverPolicy::FullSleep);
display.screensaver.resetActivity();  // call on user interaction
```

## Application Call Sites (onEnter* pattern)

```cpp
void Application::onEnterConfiguring() {
    _display.carousel.suspend();
    _display.showBase(&_display.boards.configuringStatus);
}
void Application::onWifiAPStarted(const char* hostname) {
    _display.boards.wifiWizard.configure(hostname);
    _display.showModal(&_display.boards.wifiWizard);
}
void Application::onWifiConnected() { _display.clearModal(); }
void Application::onEnterRunning()  { _display.carousel.resume(); }
```

## Simplified tick()

```cpp
void U8G2DisplayModule::tick(uint32_t ms) {
    _screensaver.tick(ms, _u8g2);   // display-intrinsic: inactivity policy
    _carousel.tick(ms);              // display-intrinsic: rotation timing
    _renderCurrentBoard(ms);         // display-intrinsic: SPI/U8G2 output
    // No AppState. No WifiManager. No ConfigGates.
}
```
