# sample-app/src

## Invariants & contracts

### [docseed-a431b5da] A throttled check (every 5 seconds) polls gates for regression. If an actionable gate remains unsatisfied beyond its declared `getRegressionToleranceMs()` threshold, the framework regresses the system back to the `CONFIGURING` state.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, sample-app/src/main.cpp, include/AppFoundation.h, modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-addca82f] **No Runtime Cost**: Stubs are compile-time only. Production builds link directly against the true ESP-IDF or Arduino headers.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, sample-app/src/main.cpp, include/AppFoundation.h, modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`

### [docseed-ff7d1a36] **No Inline Guards**: Production code remains clean and free of `#ifdef` preprocessor directives.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, sample-app/src/main.cpp, include/AppFoundation.h, modules/BLEManager/BLEManager.hpp · source_doc: docs/developer/architecture-overview.md · evidence: doc:docs/developer/architecture-overview.md`
