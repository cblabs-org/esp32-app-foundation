# Graveyard: JSON Parsing

**Failed attempts / Rejected paths:**
- **ArduinoJson static or dynamic documents:** Rejected explicitly because we want to avoid large allocations and memory fragmentation.
- **Copying previous Gadec UK parsing code:** Rejected due to "clean room provenance" requirement. We implemented the listener from scratch.
- **Blocking the main tick loop:** Rejected. By inheriting `IDataSource` instead of trying to run `HTTPClient` synchronously in `ITickable::tick()`, we ensured that the 10ms budget isn't violated. The `DataManager` executes our `fetchUpdate()` in its own background FreeRTOS task.
