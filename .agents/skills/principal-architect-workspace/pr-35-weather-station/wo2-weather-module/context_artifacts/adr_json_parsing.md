# ADR: JSON Parsing for OpenWeatherMap

**Context:** The OpenWeatherMap JSON response contains arrays and nested objects. Using ArduinoJson to parse the full document would require allocating a massive JSON DOM in memory, which is risky on embedded devices with tight RAM constraints like the ESP32.

**Decision:** We elected to use `JsonStreamingParser` with a custom `JsonListener` implementation. We parse the incoming bytes one-by-one inside the `IDataSource::fetchUpdate()` worker thread. This allows us to extract `main.temp`, `wind.speed`, and the first `weather[0].main` value efficiently without building a full tree in memory. 

**Consequences:** The `WeatherModule` maintains a small state machine (`_currentKey`, `_weatherArrayDepth`) during the parsing cycle, keeping memory usage constant and bounded.
