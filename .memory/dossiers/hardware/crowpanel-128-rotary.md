# hardware/crowpanel-128-rotary

## Invariants & contracts

### [docseed-1c973724] **Framework stays clean**: Consumer projects that don't use LVGL, or that use a different display panel, carry no CrowPanel-specific code in the library they pull.
`invariant · kind: semantic · anchors: include/interfaces, modules/LVGLDisplay, adapters/lvgl, hardware/crowpanel-128-rotary/board.h · source_doc: docs/developer/adr/adr-adapters-placement.md · evidence: doc:docs/developer/adr/adr-adapters-placement.md`

### [docseed-b1f69fd1] **Consumer guidance**: The `adapters/` directory serves as a reference for consumer projects writing their own adapters. The README documents the interface contract, authoring instructions, and wiring examples.
`contract · kind: semantic · anchors: include/interfaces, modules/LVGLDisplay, adapters/lvgl, hardware/crowpanel-128-rotary/board.h · source_doc: docs/developer/adr/adr-adapters-placement.md · evidence: doc:docs/developer/adr/adr-adapters-placement.md`

### [docseed-c2139d0a] `hardware/crowpanel-128-rotary/partitions.csv` — Elecrow 1.28" Rotary Display (16MB flash, Octal PSRAM)
`invariant · kind: semantic · anchors: hardware/crowpanel-128-rotary/partitions.csv · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-f9bcc326] **Extensible pattern**: Future pluggable interfaces (e.g. audio, sensor bus) can follow the same convention (`adapters/audio/`, `adapters/sensor/`) without muddying the module or hardware trees.
`contract · kind: semantic · anchors: include/interfaces, modules/LVGLDisplay, adapters/lvgl, hardware/crowpanel-128-rotary/board.h · source_doc: docs/developer/adr/adr-adapters-placement.md · evidence: doc:docs/developer/adr/adr-adapters-placement.md`
