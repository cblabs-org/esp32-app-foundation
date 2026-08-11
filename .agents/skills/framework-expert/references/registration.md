# Registration System Reference

## Universal Module Registration

The framework uses C++ compile-time type introspection (`if constexpr` and `std::is_base_of_v`) to detect which interfaces a class implements. Calling `app.registerModule(&module)` automatically registers the module with all matching subsystems.

```cpp
// In setup()
static MySensorModule sensor;
app.registerModule(&sensor);
```

Behind the scenes, the template expands to check all 9 interfaces:
```cpp
template <typename T>
void registerModule(T* module) {
    if constexpr (std::is_base_of_v<ITickable, T>) {
        _tickables.push_back(module);
    }
    if constexpr (std::is_base_of_v<IConfigurable, T>) {
        _configurables.push_back(module);
    }
    // ... repeats for all interfaces
}
```

## Two-Tier Vector Model & CA-05

Initially, the framework used fixed-size arrays (e.g., `ITickable* tickables[16]`) to avoid heap fragmentation, but this imposed arbitrary limits on app complexity.

**CA-05** replaced fixed arrays with `std::vector`, but strictly limits vector allocation to the boot phase.

### The `_compactAndLock()` Lifecycle

To prevent heap fragmentation during runtime, the framework employs a two-tier lifecycle:

1. **Phase 1: Registration Window** 
   During `setup()`, modules are pushed into vectors. The vectors may reallocate and fragment the heap, but this is acceptable as it happens before long-term objects are created.

2. **Phase 2: Compact and Lock**
   Inside `app.begin()`, the framework calls `_compactAndLock()`. This method:
   - Iterates through all internal vectors.
   - Calls `shrink_to_fit()` on each to release unused capacity back to the heap.
   - Sets a boolean `_isLocked = true`.

3. **Phase 3: Runtime Rejection**
   Any attempt to call `app.registerModule()` after `app.begin()` has been invoked will instantly trigger a fatal panic. Late registration is forbidden because pushing to a locked vector could cause a heap reallocation during steady-state execution.

### Why Does Late Registration Fail?

If an agent attempts to register a module inside `tick()` or in response to an event, the ESP32 will panic and reboot:

```cpp
// ❌ WRONG: Registration after boot
void MyModule::tick(uint32_t currentMillis) {
    if (someCondition && !isRegistered) {
        // FATAL: The registry was locked during app.begin()
        // This will trigger a panic!
        _app->registerModule(&_dynamicModule); 
    }
}
```

**Solution:** Register all modules unconditionally in `setup()`. If a module should be inactive, its `tick()` method should simply return early based on its internal state.
