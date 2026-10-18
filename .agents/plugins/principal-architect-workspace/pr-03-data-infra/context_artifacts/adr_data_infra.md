# PR-03 Architecture Decision Records

## EventBus
- Decided to use static function pointers instead of `std::function` for `EventBus` to prevent heap fragmentation.
- Implemented `EventBus` as a purely synchronous pub/sub system. The payloads are only guaranteed to be valid during the callback execution.
- Added topic validation to prevent overflow and malformed events.

## DataManager
- Implemented as a FreeRTOS task with an 8KB stack size to ensure background network and heavy logic operations do not crash the MCU.
- Implemented an `iDataSource` scheduling loop supporting `PriorityTier`.

## Application Wiring
- Wired `EventBus` and `DataManager` natively into the `Application` hub.
- Added `NullStatusReporter` as an inline stub inside `Application` for when no physical display module is present.
- Added `"system.heap"` event publication on the `EventBus` after `_compactAndLock()`.
