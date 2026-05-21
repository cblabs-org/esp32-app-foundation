# State: WO3 File Manifest and Board-Widget Composition Map

## New Widget Files Created

| File | Extends | Purpose |
|------|---------|---------|
| `modules/u8g2Display/widgets/wifiStatusWidget.hpp/.cpp` | `iGfxWidget` | WiFi RSSI bars (1-4) + blinking disconnect icon |
| `modules/u8g2Display/widgets/systemMessageWidget.hpp/.cpp` | `iGfxWidget` | Centred multi-line overlay (title + 3 detail lines) |
| `modules/u8g2Display/widgets/scrollingMessagePoolWidget.hpp/.cpp` | `scrollingTextWidget` | EventBus `"display.message"` subscriber with circular buffer |

## Board → Widget Composition Map

| Board | Widget Members | Wiring |
|-------|---------------|--------|
| `BootSplashBoard` | `labelWidget _titleLabel{0,12,-1,-1}`, `labelWidget _versionLabel{0,40,-1,-1}` | `activate()` sets fonts + text; `render()` calls both |
| `LoadingBoard` | `progressBarWidget _progressBar{0,20,-1,-1}` | `setStatus()` → `setMessage()` + `setPercent()`; `render()` delegates |
| `WiFiWizardBoard` | `systemMessageWidget _messageDisplay{0,0,-1,-1}` | `activate()` → `setMessage(ssid, ip, instructions)`; `render()` delegates |
| `OtaProgressBoard` | `labelWidget _titleLabel{0,5,-1,-1}`, `progressBarWidget _progressBar{0,25,-1,-1}`, `labelWidget _statusLabel{0,55,-1,-1}` | `setProgress()` → `setPercent()` + `setText()`; `render()` calls all three |
| `ErrorBoard` | `systemMessageWidget _errorDisplay{0,0,-1,-1}` | `showError()` → `setMessage("ERROR", msg, "System halted")`; `render()` delegates |
| `ScreensaverBoard` | `labelWidget _clockLabel{0,0,-1,-1}` | `render()` formats HH:MM, calls `setCoords()` + `setFont()` + `setText()` + `render()` per frame |

## Modified WO2 File

| File | Change |
|------|--------|
| `modules/u8g2Display/widgets/progressBarWidget.hpp` | Added `void setFont(const uint8_t* _font)` inline method |
