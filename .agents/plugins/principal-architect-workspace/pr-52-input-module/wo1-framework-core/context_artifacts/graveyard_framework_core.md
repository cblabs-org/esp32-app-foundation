# Graveyard — WO1 Framework Core

## 1. Native Build Platform Stubs for `iot_button` & FreeRTOS Queue
- **Problem**: Building native tests (`env:native`) failed because `iot_button.h` and `freertos/queue.h` were not present in the native host toolchain.
- **Resolution**: Created minimal, portable stubs in `test/stubs/platform/`:
  - `test/stubs/platform/iot_button.h`: Stubs `iot_button_create`, `iot_button_new_gpio_device`, `iot_button_register_cb`, `iot_button_delete` with `ESP_OK` returns.
  - `test/stubs/platform/freertos/queue.h`: Provides a fully functional native C++ queue implementation backed by `std::queue<std::vector<uint8_t>>`, guaranteeing queue behavior works natively for unit tests without FreeRTOS binaries.

## 2. Interface Signature Mismatch on `IMCPContributor`
- **Problem**: Initial draft work order referenced `contributeMCPTools()`.
- **Resolution**: Verified exact interface signature in `include/interfaces/IMCPContributor.hpp` which defines `virtual void registerMCPTools(MCPToolRegistry& registry) = 0;`. Implementation strictly matches this header contract.
