# Graveyard: Application Hub

## Fixed-Array Registration (ADR-05)
Early design ideas proposed using fixed-size C-style arrays (e.g. `ITickable* _tickables[MAX_TICKABLES]`) to avoid dynamic memory usage. However, this forced developers to guess max capacities, leading to configuration overhead and brittle compile-time parameters.
- **Lesson Learned**: The two-tier vector model (`std::vector` + post-registration `shrink_to_fit()` and lock) is safer and cleaner, offering dynamic flexibility during init while maintaining static-like safety at runtime.

## Singleton Anti-pattern
We avoided the Singleton pattern (such as `Application::getInstance()`). The `Application` instance is instead declared as a file-scope static in `src/main.cpp`, protecting it from accidental stack-allocation and avoiding global-state coupling issues.
