# modules/HomeAssistant

## Decisions

### [seed-0ff2f0d8] **Stale Identifier Elimination in §6.1**: Rather than mentioning historical camelCase `i` prefixes in a transitional note, the text unconditionally asserts that all framework interfaces use the PascalCase `I` prefix. This satisfied the strict zero-occurrence lint criteria for `iConfigurable` and `iDataSource`.
`decision · kind: episodic · anchors: include/types/FrameworkConfig.hpp, platformio.ini, include/interfaces/IOTAVerifier.hpp, modules/U8G2Display/IDisplayBoard.hpp, modules/LVGLDisplay/ILVGLHardwareAdapter.hpp, modules/HomeAssistant/entities/IHaEntity.hpp`

### [seed-547329c8] **Module-Local Extension Points**: Located and documented all 3 module-local extension points (`IDisplayBoard`, `ILVGLHardwareAdapter`, and `IHaEntity`) in dedicated subsections under §6.16 to preserve the clean boundary between kernel-level and subsystem-specific extension points.
`decision · kind: episodic · anchors: include/types/FrameworkConfig.hpp, platformio.ini, include/interfaces/IOTAVerifier.hpp, modules/U8G2Display/IDisplayBoard.hpp, modules/LVGLDisplay/ILVGLHardwareAdapter.hpp, modules/HomeAssistant/entities/IHaEntity.hpp`

### [seed-8b7ee3e9] **Light Touches in §2**: Updated lingering stale references in §2.5, §2.7, and §2.8 to maintain conceptual consistency across the front matter of the specification.
`decision · kind: episodic · anchors: include/types/FrameworkConfig.hpp, platformio.ini, include/interfaces/IOTAVerifier.hpp, modules/U8G2Display/IDisplayBoard.hpp, modules/LVGLDisplay/ILVGLHardwareAdapter.hpp, modules/HomeAssistant/entities/IHaEntity.hpp`

## Open issues

### [seed-5feb66a7] Sections §7 through §17 of `docs/design_specification.md` still contain legacy identifiers (`reapplyConfig`, `AsyncWebServer`, `NATIVE_BUILD`, `isScrollFinished`, `reportProgress`, `reportMessage`, etc.). These are out of scope for WO3 and are scheduled for remediation in WO4 and WO5.
`open-issue · kind: semantic · anchors: include/types/FrameworkConfig.hpp, platformio.ini, include/interfaces/IOTAVerifier.hpp, modules/U8G2Display/IDisplayBoard.hpp, modules/LVGLDisplay/ILVGLHardwareAdapter.hpp, modules/HomeAssistant/entities/IHaEntity.hpp`
