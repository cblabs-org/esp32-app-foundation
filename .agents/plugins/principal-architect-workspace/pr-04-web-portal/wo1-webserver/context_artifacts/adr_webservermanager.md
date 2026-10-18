# ADR: WebServerManager and IWebContributor Dispatch

## Context
Implementation of WO1: `WebServerManager` using `ESPAsyncWebServer` to handle the web portal, with dispatch to `IWebContributor` endpoints.

## Decisions
1. **Application Integration**: `WebServerManager` was instantiated as a core service inside `Application` (`_webServerManager`), similar to `DataManager` and `EventBus`. It is initialized during `Application::begin()`.
2. **Post Body Cap**: `AsyncWebServer::setMaxBodySize(4096)` was specified to limit the global POST body size. We wrapped this in `#ifdef NATIVE_BUILD` for testing, as `ESPAsyncWebServer` typically does not expose this globally on standard forks.
3. **Mocks and Testing**: A mock `AsyncWebServer` class and HTTP method stubs were defined for the `native` GTest build to enable CI compilation of `WebServerManager`.
