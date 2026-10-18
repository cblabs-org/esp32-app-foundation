# SPA Frontend Architecture

## Why a Hybrid PROGMEM + LittleFS approach?
The frontend needs to be highly responsive while minimizing memory usage on the ESP32. By embedding the core shell (`index.html`, `app.css`, `app.js`) into PROGMEM, we guarantee that the initial UI load is near-instantaneous and always available, even if the LittleFS partition is corrupted or uninitialized.
The app fragments (`wifi.js`, `system.js`, `ota.js`) are dynamically loaded from LittleFS, allowing for easy updates and minimizing the initial payload size.

## API Request Serialization
The ESP32 web server has a limited socket pool. Concurrent XHR or `fetch` requests can easily exhaust this pool, leading to connection resets and timeouts. 
To prevent this, `app.js` implements a global `ApiQueue` class. All API requests MUST route through `window.apiFetch(url, options)`. This queue ensures that only one HTTP request is inflight at any given time, yielding to the event loop between requests to prevent blocking the UI thread.
