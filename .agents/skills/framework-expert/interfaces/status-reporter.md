# IStatusReporter

Contract for display modules providing boot-phase status overlays.

## Overview

During the boot sequence, the Application hub needs to display progress and potential fatal errors (like LittleFS mount failures) before the system is fully running. This interface allows the core to report status without tightly coupling to a concrete display implementation.

```cpp
class IStatusReporter {
public:
    virtual ~IStatusReporter() = default;
    virtual void reportStatus(const char* message, float progress = -1.0f) = 0;
    virtual void reportError(const char* errorMessage) = 0;
    virtual void clearStatus() = 0;
};
```

## Methods

- **`reportStatus(message, progress)`**: Display a one-line status string. The progress is a fraction in `[0.0, 1.0]`. If `progress < 0`, the UI should show an indeterminate state (e.g., a spinning loader).
- **`reportError(errorMessage)`**: Display a fatal error string and a halt indicator. Used when the application panics.
- **`clearStatus()`**: Remove the status overlay, transitioning to normal operation.

## NullStatusReporter Fallback

If no display module is registered, or the system is running headless, the Application hub falls back to a `NullStatusReporter` (a no-op implementation). This prevents null pointer dereferences and allows the core logic to remain identical regardless of hardware configuration.
