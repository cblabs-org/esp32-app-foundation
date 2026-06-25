---
name: docs-excellence-wo7
description: Handoff instructions for Work Order 7 — Final compliance audit and verification
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Perform a final compliance audit of ALL files modified in WO1–WO6. Every file must score PASS on all 5 house-style criteria. Build all three codebases (main firmware, sample-app, template-app) and run all tests. Produce a compliance scorecard artifact.

# Strict Boundary Box

- You are primarily **reading and auditing** — only modify files if you find remaining compliance gaps
- If you find issues, fix them in place (documentation/comments only)
- Do NOT modify any logic or behavior
- Do NOT create git branches or make commits

# Tooling Constraints

- Use `platformio` MCP tools for ALL builds and tests
- Build commands: `build_project` with appropriate project paths
- Test commands: `run_tests` with appropriate project paths

# House-Style Reference

Read `.agents/skills/house-style-docs/references/cpp-embedded.md` to refresh the 5 audit criteria.

# Line-Item Instructions

## 1. Audit — Template-App (WO1)

Read each file and score against the 5 criteria. Fix any remaining gaps.

- [ ] `template-app/src/MyApp.hpp` (renamed from myApp.hpp)
- [ ] `template-app/src/MyApp.cpp` (renamed from myApp.cpp)
- [ ] `template-app/src/main.cpp`
- [ ] `template-app/README.md`

Verify:
- All legacy `registerMcpTools(MCPServer&)` references removed
- Interface-grouped export list present and complete
- Doxygen on every public method
- TODO markers at all customization points
- File naming is PascalCase

## 2. Audit — Sample-App (WO2 + WO3)

Read each file and score against the 5 criteria. Fix any remaining gaps.

- [ ] `sample-app/src/main.cpp`
- [ ] `sample-app/src/WeatherModule.hpp`
- [ ] `sample-app/src/WeatherModule.cpp`
- [ ] `sample-app/src/WeatherClockBoard.hpp`
- [ ] `sample-app/src/WeatherClockBoard.cpp`
- [ ] `sample-app/src/WeatherWidget.hpp`
- [ ] `sample-app/src/WeatherWidget.cpp`
- [ ] `sample-app/README.md`
- [ ] `sample-app/platformio.ini`

Verify:
- All headers have full attribution (CB Labs, URL, license, year)
- Interface-grouped export lists on WeatherModule and WeatherClockBoard
- Doxygen on every public method
- `// --- Step N ---` flow comments in fetchUpdate() and endDocument()
- IConfigGate registration verified
- WeatherWidget uses configurable units
- Streaming timeout in fetchUpdate()

## 3. Audit — Framework HomeAssistant + publicKey (WO4)

- [ ] `modules/HomeAssistant/HomeAssistantModule.hpp`
- [ ] `modules/HomeAssistant/HomeAssistantModule.cpp`
- [ ] `modules/HomeAssistant/entities/IHaEntity.hpp`
- [ ] `modules/HomeAssistant/entities/HaBinarySensor.hpp`
- [ ] `modules/HomeAssistant/entities/HaButton.hpp`
- [ ] `modules/HomeAssistant/entities/HaClimate.hpp`
- [ ] `modules/HomeAssistant/entities/HaCover.hpp`
- [ ] `modules/HomeAssistant/entities/HaDeviceTrigger.hpp`
- [ ] `modules/HomeAssistant/entities/HaFan.hpp`
- [ ] `modules/HomeAssistant/entities/HaLight.hpp`
- [ ] `modules/HomeAssistant/entities/HaNumber.hpp`
- [ ] `modules/HomeAssistant/entities/HaSelect.hpp`
- [ ] `modules/HomeAssistant/entities/HaSensor.hpp`
- [ ] `modules/HomeAssistant/entities/HaSwitch.hpp`
- [ ] `include/publicKey.hpp`

## 4. Audit — Framework Lib + Path Fixes (WO5)

- [ ] `lib/Logger/Logger.hpp`
- [ ] `lib/DeviceCrypto/DeviceCrypto.hpp`
- [ ] `lib/MD5Utils/MD5Utils.hpp`
- [ ] `modules/MCPServer/MCPServer.cpp`
- [ ] `modules/MCPServer/MCPToolRegistry.cpp`
- [ ] `modules/MCPServer/MCPBuiltinTools.cpp`
- [ ] `modules/KeyManager/KeyManager.cpp`
- [ ] `modules/DataManager/DataManager.cpp`

## 5. Audit — Stub Consolidation + OTA (WO6)

- [ ] `include/platform/NativeBuildStubs.hpp` (new file)
- [ ] `modules/MCPServer/MCPServer.hpp` — String stub removed
- [ ] `modules/WifiManager/WifiManager.hpp` — String stub removed
- [ ] `modules/OTAUpdateManager/OTAUpdateManager.hpp` — String stub removed, `_learnedManifestUrl` renamed
- [ ] `modules/OTAUpdateManager/OTAUpdateManager.cpp` — references updated
- [ ] `lib/Logger/Logger.hpp` — String stub removed

Verify:
- `grep -rn "STRING_STUB_DEFINED" --include="*.hpp" .` → only in NativeBuildStubs.hpp
- `grep -rn "learnedManifestUrl_" modules/OTAUpdateManager/` → zero results

## 6. Build Verification

- [ ] Build main firmware: `pio run -e esp32dev` in project root — MUST PASS
- [ ] Build sample-app: `pio run -e esp32dev` in `sample-app/` — MUST PASS
- [ ] Build template-app: `pio run -e esp32dev` in `template-app/` — MUST PASS
- [ ] Run native tests: `pio test -e native` in project root — MUST PASS
- [ ] Run sample-app native tests if they exist

## 7. Produce Compliance Scorecard

Create an artifact at the PA workspace directory with the final scorecard. Format:

```markdown
# PR-40 Final Compliance Scorecard

## Template-App
| File | Header | Methods | Flow | Vars | Naming | Result |
|------|:------:|:-------:|:----:|:----:|:------:|:------:|
| MyApp.hpp | ✅ | ✅ | ✅ | ✅ | ✅ | PASS |
...

## Sample-App
...

## Framework (Modified Files)
...

## Build Results
| Target | Result |
|--------|--------|
| Main firmware (esp32dev) | ✅ PASS |
...

## Summary
Total files audited: X
Files passing all criteria: X/X (100%)
```

- [ ] **STOP and send the scorecard to the PA. Ask the user if they are happy before distilling.**
