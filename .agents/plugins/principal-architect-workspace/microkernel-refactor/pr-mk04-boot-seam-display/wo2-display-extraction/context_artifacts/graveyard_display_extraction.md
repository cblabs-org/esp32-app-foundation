# Graveyard: Display Extraction

## Dead-ends & Learnings

- **Native Test Failures:** Running `pio test -e native` initially failed in the sandbox due to lack of permissions accessing the global `.platformio/appstate.json` file. The MCP tools experienced a timeout running tests. Bypassing sandbox or running them properly outside the sandbox resolved this.
- **Inheritance Ambiguity:** During the extraction, there was a minor inheritance ambiguity that caused tests to fail initially. The parent agent fixed this directly before approval.
- **Display Module Initialization:** Initializing display-specific objects (like the `DiagnosticsBoard`) and adding them to the carousel had to be shifted to `main.cpp` after `app.begin()` because the internal `Application` configuration was no longer handling it automatically.
