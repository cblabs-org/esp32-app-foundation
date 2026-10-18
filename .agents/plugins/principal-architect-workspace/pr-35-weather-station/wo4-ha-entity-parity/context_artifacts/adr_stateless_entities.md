# Architecture Decision Record: Stateless Entities & Subtopics

## Context
Home Assistant entities like `HaButton` and `HaDeviceTrigger` are entirely stateless—they have no "state" to publish upon reconnect, and the discovery schema strictly avoids `state_topic` for them. In contrast, complex entities like `HaClimate` and `HaFan` utilize templates for state JSON but require multiple command subtopics (e.g., `.../set/mode`, `.../set/temp`).

## Decision
1. **Stateless Support:** Added `virtual bool hasState() const { return true; }` to `IHaEntity`. Overridden to return `false` in `HaButton` and `HaDeviceTrigger`. The `HomeAssistantModule` now checks this before injecting a `state_topic` into discovery JSON, and skips these entities in `publishAllStates()`. This elegantly solves both problems without creating complex separate interfaces.
2. **Subtopic Routing:** Instead of limiting command subscriptions to an exact topic match (`.../set`), `HomeAssistantModule` now subscribes to both `.../set` and `.../set/#`. The `handleCommand` signature was modified to `(const std::string& topic, const std::string& payload)`, enabling individual entities to parse out their suffixes directly and route appropriately (e.g., `HaFan` inspecting `topic` for `/percentage` vs `/oscillation`).

## Consequences
- Perfect 1-to-1 parity with Home Assistant's required schemas.
- Lightweight coupling; `HomeAssistantModule` stays generic and pushes routing specifics to the subclasses.
