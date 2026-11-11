# template-app/src

## Invariants & contracts

### [docseed-3e5cdbd3] [main.cpp](../../template-app/src/main.cpp)
`invariant · kind: semantic · anchors: template-app/src/main.cpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-70e15374] Don't call both `registerModule()` and individual registration methods — `registerModule()` does all the work.
`invariant · kind: semantic · anchors: template-app/src/MyApp.hpp, platformio.ini, template-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-76542df8] [MyApp.hpp](../../template-app/src/MyApp.hpp)
`invariant · kind: semantic · anchors: template-app/src/MyApp.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-89ea17af] **Unsigned subtraction** (`currentMillis - _lastActionMs`) — correctly handles `millis()` overflow without special-casing.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, template-app/src/MyApp.hpp, platformio.ini, template-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-ce05d6a2] **Forward Declarations:** Use forward declarations (e.g. `class PsychicHttpServer;`) in your header file instead of full `#include` statements to reduce compile times and prevent circular dependency loops.
`invariant · kind: semantic · anchors: template-app/src/MyApp.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-ce9b080f] **Interfaces Inheritance:** Standard multiple inheritance is used to implement multiple interfaces.
`contract · kind: semantic · anchors: template-app/src/MyApp.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-de57456f] [MyApp.cpp](../../template-app/src/MyApp.cpp)
`invariant · kind: semantic · anchors: template-app/src/MyApp.cpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`

### [docseed-efe8077c] **Never use `delay()`** — it blocks the entire thread, halting WiFi, display, web server, and all other modules.
`invariant · kind: semantic · anchors: include/interfaces/ITickable.hpp, template-app/src/MyApp.hpp, platformio.ini, template-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

### [docseed-f525c754] All modules at global/static scope (ADR-16).
`invariant · kind: semantic · anchors: template-app/src/MyApp.hpp, platformio.ini, template-app · source_doc: docs/developer/sample-app-walkthrough.md · evidence: doc:docs/developer/sample-app-walkthrough.md`

## Ruled out, failure signatures and gotchas

### [docseed-5d302593] **`override` Keyword:** Always mark inherited virtual functions with `override` to allow the compiler to detect signature mismatches.
`failure-signature · kind: procedural · anchors: template-app/src/MyApp.hpp · source_doc: docs/developer/creating-a-module.md · evidence: doc:docs/developer/creating-a-module.md`
