# WO5: Callers & Documentation (Completed)

## Tasks Executed
1. **Caller Audit**: Grepped the codebase for unsafe `ESP_LOGD` usages dumping payloads/bodies via `%s` and `c_str()`. Found no remaining instances (the codebase is already clean of these anti-patterns following recent cleanups).
2. **Design Documentation**: Created `docs/design/logging.md` to formally specify the Logging & Redaction System. The documentation details the `LogPipeline` static buffer vs heap fallback strategy, the thread-safety mechanisms, the `Redactor` bounds limitations, the `StructuralScrubber` heuristics, and the usage of the new `dumpPayload()` API.

All WOs in the PR #212 remediation plan are now fully executed.
