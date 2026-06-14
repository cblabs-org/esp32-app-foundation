# ADR: PsychicHttp Migration (WO9)

## Context
The ESP32 App Foundation was relying on `ESPAsyncWebServer` for HTTP serving. However, `ESPAsyncWebServer` has been unmaintained, lacks HTTP/2 support, and struggles with large payloads. The transition to `PsychicHttp` brings enhanced stability and modernized architecture (building on ESP-IDF's core HTTP daemon).

## Decisions
1. **Dependency Upgrade**: Included `PsychicHttp` via PlatformIO (`lib_deps = hoeken/PsychicHttp @ 3.1.1`).
2. **Global Server Access**: Refactored `WebServerManager` to return `PsychicHttpServer&`.
3. **Route Signatures**: All lambda and class method route handlers have been migrated to conform to PsychicHttp's signatures (expecting `esp_err_t` return types and pointers to `PsychicRequest` and `PsychicResponse`).
4. **Static File Serving**: Moved away from in-memory strings for JSON payloads and static assets where possible. For instance, `timezones.json` is now served directly from `LittleFS` using `PsychicFileResponse` to reduce heap usage.
5. **Security**: AuthManager methods (`checkMcpAuth`, `checkWebAuth`) were updated to use `PsychicRequest` to evaluate credentials.
6. **Uploads**: Handlers for multipart uploads (like OTAUpdateManager) have been rewritten using `PsychicUploadHandler`.
7. **SDK Configuration**: Enabled `CONFIG_LWIP_IPV6` and `CONFIG_ESP_HTTPS_SERVER_ENABLE` in `sdkconfig.defaults` to satisfy `PsychicHttp`'s underlying dependencies.
