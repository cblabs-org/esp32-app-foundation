# lib/ConfigManifest

## Invariants & contracts

### [docseed-370bd372] **Active:** The schema version currently running on the device.
`contract · kind: semantic · anchors: lib/ConfigManifest · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-3c601b94] **Golden:** The last-known-good schema version, snapshotted when the firmware passes validation and is blessed via `snapshotActiveToGolden()`.
`contract · kind: semantic · anchors: lib/ConfigManifest · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-649e0037] **Previous:** The schema version that was active prior to the last OTA update.
`contract · kind: semantic · anchors: lib/ConfigManifest · source_doc: docs/design_specification.md · evidence: doc:docs/design_specification.md`

### [docseed-fa5d9bee] **× config-lifecycle generations.** The `ConfigManifest` three-tier lifecycle (Active / Previous / Golden) exists for rollback, but *today stores schema version **numbers**, not full config **data*** ([ConfigManifest.hpp:56-58](../lib/ConfigManifest/ConfigManifest.hpp)), so today there is no data multiplier. Under full-data retention:
`contract · kind: semantic · anchors: lib/ConfigManifest/ConfigManifest.hpp · source_doc: docs/archive/microkernel-architecture-design.md · evidence: doc:docs/archive/microkernel-architecture-design.md`
