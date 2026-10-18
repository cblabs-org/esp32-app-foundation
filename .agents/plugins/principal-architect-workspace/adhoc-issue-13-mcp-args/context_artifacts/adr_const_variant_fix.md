# ADR: ArduinoJson 7 Const-Variant Type Check Fix

**Decision date:** 2026-07-18  
**Status:** IMPLEMENTED — `ced3aba` (GitHub PR #14)  
**Issue:** #13

---

## Context

`MCPServer::_handleToolsCall(const JsonDocument& req, Print& out)` receives the JSON-RPC request as a **const reference**. In the arguments-preparation block (Step 3), the original code checked:

```cpp
if (req["params"]["arguments"].is<JsonObject>()) {
```

Subscripting a `const JsonDocument` yields a `JsonVariantConst`. In ArduinoJson 7, `is<T>()` dispatches through `Converter<T>::checkJson(JsonVariantConst)`. For `JsonObject` (a mutable type), this overload is not provided — the template falls back to the generic identity that unconditionally returns `false`. The bug is compile-clean and runtime-100%-reliable: the branch was never taken, `argsDoc` was always constructed as an empty object via `argsDoc.to<JsonObject>()`, and every downstream handler received zero arguments.

## Decision

Change `is<JsonObject>()` → `is<JsonObjectConst>()` at the one affected site. `JsonObjectConst` has a correct `Converter<JsonObjectConst>::checkJson(JsonVariantConst)` overload in ArduinoJson 7.

`argsDoc.set(JsonVariantConst)` already performs a correct deep-copy — the downstream handler receives a fully populated mutable `JsonObject`. No other changes required at the call site.

## Why Not Change the Signature?

An alternative would be to make `_handleToolsCall` accept `JsonDocument&` (non-const). This was rejected because:
- The req document is strictly read-only in this function — making it non-const would be semantically dishonest and invite future mutation bugs.
- The const-variant `is<JsonObjectConst>()` is the correct ArduinoJson 7 API for this pattern and matches documented usage.
- Blast radius of the type-check fix is zero — one token replaced, no collateral changes.

## Scope Audit

Grepped the entire codebase for `is<JsonObject>()` at the time of the fix. All other call sites operate on non-const documents or mutable variants:
- `MCPBuiltinTools.cpp:234` — `params` is `JsonObject&` (mutable) ✅
- `ConfigManager` / `KeyManager` sites — operate on non-const `JsonDocument` locals ✅

Only `MCPServer.cpp:294` was affected.

## Documentation Guard

Added a `[!WARNING]` callout to `docs/developer/mcp-server.md` in the Handler Type section to prevent future contributors from introducing the same bug class. The guard notes:

> **ArduinoJson 7 const-variant pitfall.** Use `is<JsonObjectConst>()` / `is<JsonArrayConst>()` when the source is a `const JsonDocument&` or `JsonVariantConst`. The mutable-type `is<JsonObject>()` silently returns `false` on const variants.

The guard explicitly clarifies that handler code (`params` is `JsonObject&` — mutable) is unaffected; the rule applies only to internal dispatch code operating on the const request document.

## Regression Test

`TEST(McpServerTest, ToolCallArgumentsReachHandler)` in `test/test_unit/test_mcpServer/test_mcpServer.cpp`:
- Registers `arg_echo` tool that reads `params["value"]` and echoes it to `result["received"]`
- Dispatches `{"method":"tools/call","params":{"name":"arg_echo","arguments":{"value":99}}}`
- Asserts `result.content[0].text` (inner JSON) contains `received == 99`
- Returns `received == -1` and `isError == true` if arguments were dropped (pre-fix behaviour)
