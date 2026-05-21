# STATE: WebServerManager

- `modules/webServer/webServer.hpp/cpp`: Manages `AsyncWebServer`.
- Exposes `getServer()` for modules needing the bare server.
- Registers standard `/` `/app.css` `/app.js` endpoints statically.
- Iterates over `IWebContributor*` registered in `Application` and calls `registerRoutes()`.
