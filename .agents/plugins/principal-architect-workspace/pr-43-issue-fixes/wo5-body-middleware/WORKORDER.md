---
name: pr-43-wo5
description: Provider-declared body limits via IWebContributor + global middleware (ADR-15)
complexity: complex
---

# WO5 — Provider-Declared Body Limits + Global Middleware

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Implement a unified body-size enforcement mechanism where each `IWebContributor` declares its own POST/PUT body limit via `getMaxPostBody()` and API path prefix via `getApiPrefix()`. The global middleware in `WebServerManager` enforces these limits, replacing the patchwork of per-route guards. Update `routes.md` docs to describe the real mechanism. Closes GitHub Issue #22. Also absorbs WO4's former Part C (MCP body cap raise).

## Strict Boundary Box

- **DO NOT** modify the auth middleware or its bypass list.
- **DO NOT** change any module's core business logic.
- **DO NOT** modify any files outside the checklist below.

## Critical Design Constraints

1. **Provider-declared limits.** Each module declares its own body limit — no hardcoded exclusion lists in the middleware. This follows the same pattern as `getTabName()`, `getTabScript()`, etc.
2. **`getApiPrefix()` for path matching.** The middleware runs before route dispatch and only knows the URI path. Each contributor declares its API path prefix so the middleware can look up the correct limit.
3. **Default 4096 (ADR-15).** Contributors that don't override get the standard 4096-byte limit.
4. **`ESP_OK` after sending 413.** Returning `ESP_FAIL` drops the socket before the client receives the response. Match the auth middleware pattern (line 540).
5. **Existing per-route guards become redundant** once the middleware exists. Leave them in place for now as documentation-as-code (belt-and-braces). They can be stripped in a follow-up cleanup.
6. **PsychicHttp buffering caveat.** The `contentLength()` check protects against well-behaved clients. A malicious client lying about Content-Length could still exhaust heap before the middleware runs. Document this as a known limitation — it's the same constraint the existing per-route guards have.

## Checklist

### Interface Changes

- [ ] **`include/interfaces/IWebContributor.hpp`** — After `getTabScript()`, add two new virtual methods:

  ```cpp
  /**
   * @brief Return the API path prefix for this contributor's routes.
   *
   * Used by the global body-size middleware to match incoming requests
   * to the correct body limit. Return nullptr if this contributor does
   * not register any POST/PUT routes (default).
   *
   * The prefix is matched using startsWith() — e.g. returning "/api/mcp"
   * matches "/api/mcp", "/api/mcp/config", etc.
   *
   * @return API path prefix string (e.g. "/api/mcp"), or nullptr.
   */
  virtual const char* getApiPrefix() const { return nullptr; }

  /**
   * @brief Maximum POST/PUT body size this contributor's routes accept.
   *
   * The global body-size middleware in WebServerManager enforces this
   * limit for all routes matching this contributor's getApiPrefix().
   * Override to allow larger or smaller payloads than the ADR-15 default.
   *
   * @return Maximum body size in bytes. Default: 4096 (ADR-15).
   */
  virtual size_t getMaxPostBody() const { return 4096; }
  ```

- [ ] **`include/interfaces/IWebContributor.hpp` header comment** — Add `getApiPrefix()` and `getMaxPostBody()` to the exported interface documentation list.

### Module Overrides

- [ ] **`modules/MCPServer/MCPServer.hpp` (or `.cpp`)** — Add overrides:
  ```cpp
  const char* getApiPrefix() const override { return "/api/mcp"; }
  size_t getMaxPostBody() const override { return 8192; }
  ```

- [ ] **`modules/ConfigManager/ConfigManager.hpp` (or `.cpp`)** — Add overrides:
  ```cpp
  const char* getApiPrefix() const override { return "/api/config"; }
  size_t getMaxPostBody() const override { return 8192; }
  ```

- [ ] **`modules/WebServer/WebServerManager.cpp`** — For the WebServerManager's own routes (WiFi, System, etc.), these use the 4096 default which is correct. No override needed. But check: does `WifiManager` implement `IWebContributor`? If so, add:
  ```cpp
  const char* getApiPrefix() const override { return "/api/wifi"; }
  size_t getMaxPostBody() const override { return 1024; }
  ```

- [ ] **Any other `IWebContributor` implementors** — Search for all classes that override `registerRoutes()`. For each, determine if they need a non-default body limit and add `getApiPrefix()` + `getMaxPostBody()` overrides accordingly.

### Middleware Implementation

- [ ] **`modules/WebServer/WebServerManager.cpp`** — Add named constant near the top:
  ```cpp
  /// ADR-15: Default maximum POST/PUT body size for routes that do not
  /// declare a custom limit via IWebContributor::getMaxPostBody().
  static constexpr size_t DEFAULT_MAX_POST_BODY = 4096;
  ```

- [ ] **`modules/WebServer/WebServerManager.cpp` — In `begin()`, after all contributors register routes (~line 290):** Build the body limit registry:
  ```cpp
  // Build body-limit lookup from contributor-declared limits.
  // Static so the middleware lambda can capture a pointer to it.
  struct BodyLimitEntry {
      const char* prefix;
      size_t      maxBody;
  };
  static std::vector<BodyLimitEntry> s_bodyLimits;
  s_bodyLimits.clear();
  for (auto* contributor : contributors) {
      if (contributor == nullptr) continue;
      const char* prefix = contributor->getApiPrefix();
      if (prefix != nullptr) {
          s_bodyLimits.push_back({prefix, contributor->getMaxPostBody()});
      }
  }
  // Sort by prefix length descending for longest-prefix-first matching
  std::sort(s_bodyLimits.begin(), s_bodyLimits.end(),
      [](const BodyLimitEntry& a, const BodyLimitEntry& b) {
          return strlen(a.prefix) > strlen(b.prefix);
      });
  ```

- [ ] **`modules/WebServer/WebServerManager.cpp` — After the cache-control middleware:** Add the body-size middleware:
  ```cpp
  // -----------------------------------------------------------------------
  // Global POST/PUT Body-Size Middleware — ADR-15 enforcement
  // -----------------------------------------------------------------------
  // Each IWebContributor declares its own max body size via getMaxPostBody()
  // and its API path prefix via getApiPrefix(). The middleware matches the
  // incoming URI against the registry (longest prefix first) and enforces
  // the contributor's declared limit. Unmatched paths get DEFAULT_MAX_POST_BODY.
  //
  // NOTE: PsychicHttp may buffer the body before this middleware runs.
  // The contentLength() check protects against well-behaved clients but
  // cannot prevent OOM from a malicious client lying about Content-Length.
  // This is the same constraint existing per-route guards have.
  // -----------------------------------------------------------------------
  activeServer->addMiddleware([](PsychicRequest* request,
                                 PsychicResponse* response,
                                 PsychicMiddlewareNext next) -> esp_err_t {
      if (request->method() == HTTP_POST || request->method() == HTTP_PUT) {
          String path = request->uri();
          size_t limit = DEFAULT_MAX_POST_BODY;

          // Longest-prefix-first lookup
          for (const auto& entry : s_bodyLimits) {
              if (path.startsWith(entry.prefix)) {
                  limit = entry.maxBody;
                  break;
              }
          }

          if (request->contentLength() > limit) {
              // Build JSON response with the specific limit for debuggability
              char body[64];
              snprintf(body, sizeof(body),
                  "{\"error\":\"Payload too large\",\"max\":%u}",
                  (unsigned)limit);
              response->send(413, "application/json", body);
              return ESP_OK;  // ESP_OK so PsychicHttp flushes the response
          }
      }
      return next();
  });
  ESP_LOGI(TAG, "Global body-size middleware registered (default=%u, %u overrides)",
           DEFAULT_MAX_POST_BODY, s_bodyLimits.size());
  ```

### Docs Update

- [ ] **`.agents/skills/framework-expert/systems/web-portal/routes.md:34`** — Rewrite ADR-15 rule 1:
  ```
  1. **Provider-Declared Body Limits**: The framework enforces POST/PUT body
     size limits via global middleware in `WebServerManager`. Each
     `IWebContributor` declares its own limit by overriding `getMaxPostBody()`
     (default: 4096 bytes, the ADR-15 standard). The middleware matches
     incoming requests to the contributor's `getApiPrefix()` and enforces
     the declared limit, rejecting oversized bodies with HTTP 413 before
     the handler is invoked.

     To allow larger payloads, override both methods in your contributor:
     ```cpp
     const char* getApiPrefix() const override { return "/api/mymodule"; }
     size_t getMaxPostBody() const override { return 8192; }
     ```
  ```

- [ ] **`.agents/skills/framework-expert/systems/web-portal/routes.md:37-57`** — Update the canonical POST example to show the new pattern. Remove the per-route body guard from the example (the middleware handles it). Keep the `StaticJsonDocument` rule (ADR-15 rule 2).

## Verification

- [ ] PlatformIO build passes (IWebContributor.hpp, WebServerManager.cpp, MCPServer, ConfigManager all compile).
- [ ] Code review: confirm middleware returns `ESP_OK` after sending 413 (not `ESP_FAIL`).
- [ ] Code review: confirm longest-prefix-first matching — `/api/mcp/config` matches `/api/mcp` (not `/api/config`).
- [ ] Code review: confirm the 413 response includes the specific limit value for the matched route.
- [ ] Code review: confirm all `IWebContributor` implementors have correct `getApiPrefix()` and `getMaxPostBody()` overrides.
- [ ] Grep for all existing per-route `contentLength` checks — confirm they are left in place (belt-and-braces, not removed).
