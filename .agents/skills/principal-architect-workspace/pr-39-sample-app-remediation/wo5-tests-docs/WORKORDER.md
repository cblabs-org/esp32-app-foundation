---
name: pr39-wo5
description: Handoff instructions for Work Order 5 — Real unit tests & progressive tutorial README
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Replace the placeholder unit test with real WeatherModule tests, write a progressive tutorial README, rename the stale test directory, and fix outdated lib_deps references. Covers fix items F01, F02, F21, F25.

# Strict Boundary Box

- **ONLY** touch files inside `sample-app/`.
- Do **NOT** modify any framework kernel code.
- Do **NOT** create new branches or merge anything.

# Context

- The native test environment uses GoogleTest.
- Tests run via `pio test -e native` (use MCP `run_tests` tool).
- WeatherModule implements IConfigurable, IConfigGate, IMCPContributor — all testable in native env.
- The native build defines `NATIVE_BUILD` which guards out hardware-dependent code.
- Look at the main project's test files in `/Users/mcneillm/Documents/Projects/esp32-app-foundation/test/test_unit/` for patterns — especially `test_configManager` for config round-trip patterns.

# Tooling Constraints

- Use the `platformio` MCP server tools for builds and tests — not raw `pio` CLI commands.
- The sample-app's PlatformIO project root is `/Users/mcneillm/Documents/Projects/esp32-app-foundation/sample-app`.

# Checklist

- [ ] **F21: Rename test directory** — Rename `sample-app/test/test_unit/test_myapp/` to `sample-app/test/test_unit/test_weather/`. Update any references.

- [ ] **F01: Write real unit tests** — Replace the placeholder `EXPECT_EQ(1+1, 2)` in the test file with real tests:

  **Test 1: Config round-trip** — Create a WeatherModule, call `serializeConfig()` into a JsonObject, then call `applyConfig()` with known values, then `serializeConfig()` again and verify the values match.
  
  **Test 2: Config defaults** — Create a WeatherModule, call `resetConfig()`, then `serializeConfig()` and verify defaults (lat=0, lon=0, interval=600000, units="metric").
  
  **Test 3: IConfigGate — no KeyManager** — Create a WeatherModule without calling `begin()` (so _keyManager is nullptr), verify `isSatisfied()` returns `true` (fail-open when no KeyManager).
  
  **Test 4: Gate name and help text** — Verify `getGateName()` returns `"Weather API Key"` and `getHelpText()` is non-null.
  
  **Test 5: IConfigurable metadata** — Verify `getConfigNamespace()` returns `"weather"` and `getSchemaVersion()` returns `1`.

  **Test 6: IDataSource metadata** — Verify `getSourceName()` returns `"OpenWeatherMap"` and `getPriorityTier()` returns `PriorityTier::Normal` and `getUpdateIntervalMs()` returns default `600000`.

- [ ] **F02: Progressive tutorial README** — Rewrite `sample-app/README.md` with:
  1. Keep the existing "Getting Started" git submodule setup section (but update stale lib_deps — F25)
  2. Add a new "## How It Works" section with a progressive walkthrough:
     - **Static Instances** — explain why modules are declared at file scope (ADR-16)
     - **Dependency Injection** — explain the `begin(Application&)` transient service locator pattern (ADR-39)
     - **Module Registration** — explain `registerModule()` and the compile-time type introspection
     - **Interface Walkthrough** — for each interface WeatherModule implements (ITickable, IConfigurable, IWebContributor, IDataSource, IMCPContributor, IConfigGate), explain what it does and point to the relevant code
     - **Display Board** — explain the IDisplayBoard pattern with WeatherClockBoard
     - **Thread Safety** — briefly explain the atomic + mutex dual strategy
  3. Add a "## Testing" section explaining how to run `pio test -e native`

- [ ] **F25: Fix stale lib_deps** — In the README's lib_deps example, remove `githubClient` (deleted in PR-37). Verify all other entries match current framework modules. Also update any stale references to `wifiManager` → `WifiManager` (PascalCase).

# Verification

After making all changes:
1. Run `pio test -e native` via MCP tools to verify all tests pass.
2. Run `pio run -e esp32dev` via MCP tools to verify hardware build still compiles.
3. Report test results with pass/fail counts.

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only *after* approval should you invoke the `@distillery` skill to package the state and drop control.
