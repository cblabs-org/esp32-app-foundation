# ESP32 App Foundation Template

This directory contains a complete, functional template project to help you get started building a new application on top of the ESP32 App Foundation framework.

## Getting Started

To use this template for your own project, we recommend treating the ESP32 App Foundation as a Git submodule.

1. **Create your project repository:**
   ```bash
   mkdir my-esp32-app
   cd my-esp32-app
   git init
   ```

2. **Add the foundation as a submodule:**
   ```bash
   git submodule add https://github.com/matthewmcneill/esp32-app-foundation.git lib/esp32-app-foundation
   git submodule update --init --recursive
   ```

3. **Copy the template files into your project root:**
   ```bash
   cp -r lib/esp32-app-foundation/template/* ./
   cp -r lib/esp32-app-foundation/template/.* ./ 2>/dev/null || true
   ```

4. **Update `platformio.ini`:**
   In your new project's `platformio.ini`, you must configure PlatformIO to compile the foundation's modules.
   Remove the `../` references and replace them with the submodule paths:
   ```ini
   build_flags =
       -DAPP_FOUNDATION_VERSION=\"0.1.0\"
       -I lib/esp32-app-foundation/include
   lib_extra_dirs = 
       lib/esp32-app-foundation/modules
       lib/esp32-app-foundation/lib
   lib_deps =
       application
       configManager
       dataManager
       eventBus
       keyManager
       mcpServer
       otaUpdateManager
       timeManager
       webServer
       wifiManager
       deviceCrypto
       githubClient
       logger
       md5Utils
   ```
   You must also update `board_build.partitions` and `custom_sdkconfig` to point to `lib/esp32-app-foundation/partitions...` instead of `../`.

## Project Structure

- `src/`: Your application source code.
- `test/`: Unit tests and integration tests for your application.
- `platformio.ini`: The project configuration file, pre-configured with the foundation's recommended environments (`esp32dev` and `esp32s3nano`).

## Building the Project

Use PlatformIO to build your project:

```bash
pio run
```

To build for a specific environment:

```bash
pio run -e esp32dev
```
