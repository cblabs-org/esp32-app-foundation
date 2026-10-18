# Graveyard: Config Manager

## Dynamic JSON Documents for Persistence
Early implementation used dynamic memory allocation (e.g. `DynamicJsonDocument`) when parsing configs. This caused heap fragmentation and occasional out-of-memory crashes on ESP32, which has a limited heap.
- **Lesson Learned**: Use `StaticJsonDocument` with explicit, pre-calculated capacity sizes (e.g. `StaticJsonDocument<1024>`) in config read/write operations to guarantee memory safety.
