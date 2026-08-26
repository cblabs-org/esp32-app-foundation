# Framework Topics

The App Foundation core services publish standard events to the `EventBus`. Modules can subscribe to these to react to system state changes. 

Here are the 10 framework-published topics, their payload types, and the publisher identity.

## Application Subsystem

### 1. `system.heap` (`SYSTEM_HEAP`)
- **Publisher**: `Application`
- **Payload**: Free heap size (`uint32_t`)

### 2. `app.running` (`APP_RUNNING`)
- **Publisher**: `Application`
- **Payload**: None (`nullptr`)

### 3. `app.configuring` (`APP_CONFIGURING`)
- **Publisher**: `Application`
- **Payload**: `const AppConfiguringPayload*` — struct containing `preconditionName`, `helpText`, `cause` (Cause), `severity` (Severity), `isRegression`, `hostname`, and `deviceIP`. Defined in `include/events/AppConfiguringPayload.hpp`.

### 4. `precondition.health` (`PRECONDITION_HEALTH`)
- **Publisher**: `PreconditionEngine`
- **Payload**: `const PreconditionHealthPayload*` — struct containing `name`, `cause` (Cause), `severity` (Severity), `everMet`, and `role` (PreconditionRole). Published per-precondition when state changes. Defined in `include/events/PreconditionHealthPayload.hpp`.

### 5. `data.source.health` (`DATA_SOURCE_HEALTH`)
- **Publisher**: `DataManager` (worker task)
- **Payload**: `const DataSourceHealthPayload*` — struct containing `name` (`const char*`) and `state` (`SourceHealthState`). Published when a data source transitions health state. Defined in `include/events/DataSourceHealthPayload.hpp`.

## OTA Subsystem

### 6. `ota.progress` (`OTA_PROGRESS`)
- **Publisher**: `OTAUpdateManager`
- **Payload**: Progress percentage
- **Note**: Modules should suspend expensive operations when OTA is in progress to ensure the update stream is not interrupted.

## Display Subsystem

### 7. `display.sleep` (`DISPLAY_SLEEP`)
- **Publisher**: `U8G2DisplayModule`
- **Payload**: None (`nullptr`)

### 8. `display.wake` (`DISPLAY_WAKE`)
- **Publisher**: `U8G2DisplayModule`
- **Payload**: None (`nullptr`)

### 9. `display.board.changed` (`DISPLAY_BOARD_CHANGED`)
- **Publisher**: `U8G2DisplayModule`
- **Payload**: Board name string (`const char*`)

### 10. `display.message` (`DISPLAY_MESSAGE`)
- **Publisher**: `U8G2DisplayModule`
- **Payload**: Message string (`const char*`)
