# FrameworkConfig vs IConfigurable

Configuration in the App Foundation is split into two distinct categories: compile-time framework configuration and runtime module configuration.

## FrameworkConfig Struct

The `FrameworkConfig` is a static C++ struct passed into the `Application` constructor. It defines the immutable, compile-time properties of the device.

```cpp
struct FrameworkConfig {
    const char* deviceName;
    const char* wifiApPassword;
    uint16_t webServerPort;
    // ...
};
```

**What goes here:**
- Hardcoded device identities (e.g., "Living Room Sensor").
- Fallback AP passwords for provisioning.
- Hardware pin definitions (if not managed dynamically).
- Compile-time secrets (e.g., API keys baked into the firmware).

These values cannot be changed without recompiling and flashing the device.

## IConfigurable Interface

The [`IConfigurable`](../../interfaces/configurable.md) interface is implemented by individual modules. It defines the dynamic, runtime state that users can modify via the Web Portal or MCP Server.

**What goes here:**
- User-provided Wi-Fi credentials.
- MQTT broker addresses.
- Module-specific tuning parameters (e.g., polling intervals, threshold values).
- API keys entered by the user post-flashing.

## Which Should I Use?

When building a new module, ask yourself: *Should the end-user be able to change this without compiling code?*

- If **Yes**, implement `IConfigurable` and expose the setting via the Web Portal.
- If **No**, add the setting to a compile-time `#define` or pass it in via the module's constructor. 

*Do not* add module-specific settings to the global `FrameworkConfig` struct. That struct is reserved for core framework services. Modules should manage their own runtime configuration.
