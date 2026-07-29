# ADR: Honest main.cpp Composition

**Decision:** We resolved OQ-MK-2 in favor of the "honest main.cpp" pattern (7a) rather than creating a thin `ModuleComposition` framework unit (7b).
**Rationale:** It keeps `main.cpp` tiny while preserving the Hollywood principle. Explicitly instantiating optional modules directly in `main.cpp` means the composition logic is fully visible, greppable, and devoid of implicit framework magic. This ensures deterministic initialization ordering and completely removes kernel ownership of optional modules like HomeAssistant, MCP, and OTA.
