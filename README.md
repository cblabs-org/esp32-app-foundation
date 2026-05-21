# ESP32 App Foundation

A reusable, dual-framework (Arduino + ESP-IDF) application foundation for the ESP32 ecosystem.

## Framework Overview

This repository provides a standardized composition root and application lifecycle for ESP32 projects. Built for both robust enterprise use and rapid prototyping, it utilizes the ESP-IDF framework with Arduino core as a component (`framework = arduino, espidf`).

### Key Features:
- **Hollywood Principle Entry Point:** A clean `Application::begin()` and `Application::tick()` lifecycle managed by the foundation, keeping `main.cpp` completely clean and structural.
- **Two-Tier Module Registration:** Clearly separates Boot phase registration from Lock phase runtime, using `std::vector` combined with `shrink_to_fit()` for memory efficiency.
- **Interface Contracts:** Standardized `ITickable`, `iConfigurable`, `IWebContributor`, `IMcpContributor`, etc., ensuring all components implement necessary lifecycle hooks.
- **Built-in Services:** Built in `EventBus`, `DataManager`, `WebServerManager`, and `McpServer` to standardize inter-module communication and external interfaces.

## Submodule Quick-Start

To use the ESP32 App Foundation in your project, we recommend adding it as a Git submodule:

1. **Add the submodule** to your project's `lib` directory:
   ```bash
   git submodule add https://github.com/matthewmcneill/esp32-app-foundation.git lib/esp32-app-foundation
   ```

2. **Update your `platformio.ini`**:
   Ensure your environment is set up for dual-framework execution and points to the newly added library.
   ```ini
   [env:my_esp32_project]
   platform = espressif32
   framework = arduino, espidf
   lib_deps =
       esp32-app-foundation
   ```

3. **Initialize Your Application**:
   In your `main.cpp`, instantiate the `Application` and register your modules via `registerTickable()`, `registerConfigurable()`, etc., before calling `begin()`.

## Directory Structure

- `include/`: Global interface contracts (`ITickable.hpp`, `IWebContributor.hpp`, etc.).
- `modules/`: The core `Application` composition root and background services.
- `template/`: Boilerplate code for stamping out new downstream projects.
- `lib/`: First-party reusable components.
