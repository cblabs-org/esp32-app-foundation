# Work Order 2: WeatherModule Logic

**Objective:** Build the `WeatherModule` for the sample app that fetches OpenWeatherMap data and pushes it to Home Assistant.

**Strict Constraints:**
- **Non-blocking Fetch:** Use the `IDataSource` pattern or an asynchronous HTTP client to ensure the JSON fetch does not block the 10ms tick budget.
- **Clean-room Provenance:** Write the parsing logic completely from scratch using `JsonStreamingParser` (do not copy Gadec UK code).

**Tasks:**
1. Create `sample-app/src/WeatherModule.hpp` and `.cpp`.
2. Implement `ITickable`, `IConfigurable`.
3. Instantiate `HaSensor` objects for Temperature, Wind Speed, and Weather Condition.
4. Register the sensors with the `HomeAssistantModule` instance from the framework hub.
5. Fetch JSON data from OpenWeatherMap using the API key securely retrieved from `KeyManager`.
6. Publish states to the `HaSensor` objects to sync with Home Assistant.
7. Broadcast a `weather.updated` event to the `EventBus` for local displays.
