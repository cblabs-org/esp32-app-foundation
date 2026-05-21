# PR-03 Graveyard

- We initially considered `std::function` for `EventBus` callbacks to support lambdas, but abandoned it because the overhead and heap fragmentation was too high for ESP32. We strictly rely on `void (*)(const char*, const void*, void*)` plus a context pointer.
- `Arduino.h` `#define LOW 0x0` conflicted with our `PriorityTier::LOW` enum inside `dataManager.cpp`. We resolved this by explicitly `#undef LOW` in the implementation file.
