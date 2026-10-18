# Graveyard: Test Pitfalls Encountered During WO1

## Pitfall 1: `mutable` Required for `serializeConfig()` Test Mocks

**Error**: `error: cannot assign to non-static data member within const member function 'serializeConfig'`

**Root Cause**: `serializeConfig(JsonObject&) const` is declared `const` in the `iConfigurable` interface. Test mocks that track invocation via `bool serializeConfigCalled = false` cannot assign to that flag inside a `const` method.

**Fix**: Declare the tracking flag as `mutable`:
```cpp
mutable bool serializeConfigCalled = false;
```

**Affected Files**: `test_application.cpp`, `test_configManager.cpp`

---

## Pitfall 2: Shared Namespace Causes Test Interference

**Error**: `ApplicationBeginTest.Begin_CallsApplyConfigOnRegisteredModules` failed — `confB.resetConfigCalled` was `false` when expected `true`.

**Root Cause**: Both `MockConfigurable` instances returned `"mock"` from `getConfigNamespace()`. When `beginAll()` ran:
1. Module A (ns="mock") → no file found → `resetConfig()` called → file `mod_mock.json` created
2. Module B (ns="mock") → file `mod_mock.json` exists → `applyConfig()` called (not `resetConfig()`)

**Fix**: Made `MockConfigurable` accept a namespace via constructor parameter. Test uses `MockConfigurable("mockA")` and `MockConfigurable("mockB")` with file cleanup in setUp/tearDown.

---

## Pitfall 3: Stray `mod_mock.json` Committed to Git

**Root Cause**: Running native tests creates `mod_mock.json` in the working directory. `git add -A` picked it up.

**Fix**: Removed via `git rm mod_mock.json && git commit --amend --no-edit`. Should add `mod_*.json` to `.gitignore` in a future cleanup.
