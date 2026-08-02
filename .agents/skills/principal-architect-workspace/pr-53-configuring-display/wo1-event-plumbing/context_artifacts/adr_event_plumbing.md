# ADR: Framework Event Plumbing for CONFIGURING Lifecycle Phase

## Context
When entering the `CONFIGURING` lifecycle state, `Application` publishes an `APP_CONFIGURING` event. U8G2DisplayModule listens to this event to render the base configuring status screen and modal setup wizard. However, display modules require the device hostname (AP SSID) to instruct users how to connect.

## Decision
1. Added `const char* hostname = nullptr;` to `AppConfiguringPayload` struct in `include/events/AppConfiguringPayload.hpp`.
2. Updated `Application::_publishConfiguringEvent()` to set `_configuringPayload.hostname = _wifiManager.getHostname()`.
3. In `U8G2DisplayModule::_onAppConfiguring()`:
   - Added check for `p->gateName != nullptr && strcmp(p->gateName, "WiFi Station") == 0`.
   - Used screen override pattern: prefer `_screens->wifiWizardBoard()`, fallback to `&self->boards.wifiWizard`.
   - Configured `boards.wifiWizard` with `hostname` and default AP IP `"192.168.4.1"`.
   - Pushed wizard modal via `showModal(wizardBoard)`.

## Consequences
- Clean separation between Application event publication and Display rendering.
- Extensibility retained: downstream applications providing custom `IU8G2SystemScreens` retain full control over board instances.
