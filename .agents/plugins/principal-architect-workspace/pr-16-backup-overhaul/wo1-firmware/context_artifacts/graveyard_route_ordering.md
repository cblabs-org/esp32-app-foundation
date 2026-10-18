# Graveyard: Route Ordering Bug in ESPAsyncWebServer

## The Bug

`GET /api/config/backup` and `POST /api/config/restore` were silently never firing. The web portal's backup download appeared to hang or return the namespace-list JSON instead of the full backup.

## Root Cause

ESPAsyncWebServer `on(method, path, handler)` uses **first-match-wins** prefix matching. The original registration order in `ConfigManager::registerRoutes()` was:

```cpp
server.on("/api/config", HTTP_GET, [...]{ /* namespace list */ });   // ← REGISTERED FIRST
server.on("/api/config/backup", HTTP_GET, [...]);                    // ← NEVER REACHED
server.on("/api/config/restore", HTTP_POST, [...]);                  // ← NEVER REACHED
```

Any `GET /api/config/backup` request matched `/api/config` (prefix match) and returned the namespace-list handler. The sub-path handlers were registered but unreachable.

## Fix Applied

Reversed the registration order — specific sub-paths registered before the parent prefix:

```cpp
server.on("/api/config/backup", HTTP_GET, [...]);    // ← registered first
server.on("/api/config/restore", HTTP_POST, [...]);  // ← registered second
server.on("/api/config", HTTP_GET, [...]);            // ← generic fallback last
```

## What NOT to Try

- **URL rewriting middleware:** ESPAsyncWebServer has no built-in middleware chain. Would require forking the library.
- **Regex routes:** Not supported by ESPAsyncWebServer.
- **Single handler with path parsing:** Could work but would conflate unrelated concerns into one handler. Rejected for readability.
- **Query params (`/api/config?action=backup`):** Would work but breaks REST conventions and requires web portal changes. Not acceptable for WO1 scope.

## Verification

After fix: `curl http://192.168.20.186/api/config/backup` returns the full backup JSON with `_meta`. `curl http://192.168.20.186/api/config` returns `{"namespaces":[...],"count":5}`. Both confirmed on device running `4c67b43`.
