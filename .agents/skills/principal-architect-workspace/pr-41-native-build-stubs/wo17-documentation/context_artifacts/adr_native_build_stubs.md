# ADR: Replace Inline NATIVE_BUILD Guards with Stub-Header Architecture

**Decision**: Replace all inline `#ifndef NATIVE_BUILD` guards with a stub-header library for testing native builds.

**Context**: 
The project had accumulated over 283 `#ifndef NATIVE_BUILD` guards across 45+ files. This was causing production code to become highly unreadable, difficult to maintain, and cluttered with test-specific mock implementations. Every usage of an ESP-IDF or Arduino API required preprocessor pollution, which violated our clean code goals.

**Consequences**: 
- **Production code is clean**: The `#ifndef NATIVE_BUILD` guards are completely removed from production modules. Production code now simply includes the required headers.
- **Testing infrastructure is centralized**: All mock implementations now live in `test/stubs/platform/`. These are injected via include path priority during `env:native` builds.
- **Zero runtime cost**: The stubs are compile-time only, meaning they have no impact on the performance or size of the flashed binary.
- **Maintainability**: Adding new ESP-IDF API calls simply requires dropping a stub in `test/stubs/platform/` if the native tests fail to compile, rather than modifying production code with guards.
