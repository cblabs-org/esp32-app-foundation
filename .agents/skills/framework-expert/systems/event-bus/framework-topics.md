# Framework Topics

The App Foundation core services publish standard events to the `EventBus`. Modules can subscribe to these to react to system state changes. 

Here are the 8 framework-published topics, their payload types, and the publisher identity.

## Application Subsystem

### 1. `system.heap` (`SYSTEM_HEAP`)
- **Publisher**: `Application`
- **Payload**: Free heap size (`uint32_t`)

### 2. `app.running` (`APP_RUNNING`)
- **Publisher**: `Application`
- **Payload**: None (`nullptr`)

### 3. `app.configuring` (`APP_CONFIGURING`)
- **Publisher**: `Application`
- **Payload**: `const AppConfiguringPayload*` — struct containing `gateName`, `helpText`, `actionStatus` (GateActionStatus), `isRegression`, `hostname`, and `deviceIP`. Defined in `include/events/AppConfiguringPayload.hpp`.

## OTA Subsystem

### 4. `ota.progress` (`OTA_PROGRESS`)
- **Publisher**: `OTAUpdateManager`
- **Payload**: Progress percentage
- **Note**: Modules should suspend expensive operations when OTA is in progress to ensure the update stream is not interrupted.

## Display Subsystem

### 5. `display.sleep` (`DISPLAY_SLEEP`)
- **Publisher**: `U8G2DisplayModule`
- **Payload**: None (`nullptr`)

### 6. `display.wake` (`DISPLAY_WAKE`)
- **Publisher**: `U8G2DisplayModule`
- **Payload**: None (`nullptr`)

### 7. `display.board.changed` (`DISPLAY_BOARD_CHANGED`)
- **Publisher**: `U8G2DisplayModule`
- **Payload**: Board name string (`const char*`)

### 8. `display.message` (`DISPLAY_MESSAGE`)
- **Publisher**: `U8G2DisplayModule`
- **Payload**: Message string (`const char*`)
