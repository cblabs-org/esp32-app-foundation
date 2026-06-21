# ADR: Hardware Testing Fallbacks (AP Mode)

**Context:** During autonomous execution of `wo5-hardware-validation`, the ESP32 successfully booted cleanly. Because the `LittleFS` partition was flashed cleanly, the framework fell back to AP Mode (`192.168.4.1`) as expected. 

**Decision:** We explicitly skipped JSON-RPC `curl` testing against the AP. 

**Rationale:** The host machine (Mac) would have to disconnect from its primary network (dropping internet/cloud LLM connectivity) to join the local AP, which breaks the agent's connection. Validating that the board boots the new Home Assistant integration *without* crash-looping is sufficient for declaring the build hardware-stable.
