# adapters/lvgl

## Invariants & contracts

### [docseed-140960cb] **Hardware Isolation**: Display controllers, touch sensors, encoders, and backlight PWM are implemented in hardware adapters (such as the reference [`CrowPanel128Adapter`](../../adapters/lvgl/CrowPanel128Adapter.hpp)).
`invariant · kind: semantic · anchors: adapters/lvgl/CrowPanel128Adapter.hpp · source_doc: docs/developer/using-optional-modules.md · evidence: doc:docs/developer/using-optional-modules.md`

### [docseed-b1d28a8c] **Adapter catalogue location:** Should pre-built adapters live in the framework repo (`adapters/lvgl/`) or in a separate community repository?
`invariant · kind: semantic · anchors: adapters/lvgl · source_doc: docs/archive/lvgl-display-design.md · evidence: doc:docs/archive/lvgl-display-design.md`

### [docseed-cf7af44b] **Reference Implementation:** [`CrowPanel128Adapter`](../adapters/lvgl/CrowPanel128Adapter.hpp) supports the CrowPanel 1.28" circular TFT (GC9A01 display, CST816S touch controller, EC11 rotary encoder).
`invariant · kind: semantic · anchors: adapters/lvgl/CrowPanel128Adapter.hpp · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`
