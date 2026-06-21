# Graveyard: Subtopics Routing

## Attempt: Adding `getCommandSubtopics()` to `IHaEntity`
Initially, we considered allowing `IHaEntity` to return an explicit list of command subtopics so `HomeAssistantModule` could precisely subscribe to them.
- **Why it failed:** This required passing dynamic topic arrays across interfaces, fragmenting topic handling and breaking `HaSwitch` signature backward compatibility without adding tangible benefits. Using wildcard `#` at the base entity topic handles this universally at the MQTT layer and simplifies the class definitions significantly.
