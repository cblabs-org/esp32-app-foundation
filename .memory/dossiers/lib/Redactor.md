# lib/Redactor

## Decisions

### [seed-3ba3a2f8] Added defensive check in `Redactor::redact` for null pointer and zero buffer size to prevent underflow.
`decision · kind: episodic · anchors: lib/Redactor/Redactor.h, lib/Redactor/Redactor.cpp, lib/Redactor/library.json, test/test_unit/test_redactor/test_redactor.cpp`

### [seed-b18e1ee7] Retained Logger shims with identical signatures for backward compatibility across existing call sites.
`decision · kind: episodic · anchors: lib/Redactor/Redactor.h, lib/Redactor/Redactor.cpp, lib/Redactor/library.json, test/test_unit/test_redactor/test_redactor.cpp`
