# PR-212 Remediation: Logging Subsystem Unification (V2)

## Goal
Address critical defects found in PR #212 (allocation in critical section, cross-task race condition in log hook) and implement the new verbose HTTP payload dump architecture.

## Work Orders

| Phase | Work Order | Folder | Status |
|---|---|---|---|
| 1 | WO1 - Redactor Core | `wo1-redactor-core` | ⏳ IN PROGRESS |
| 1 | WO2 - LogPipeline Correctness | `wo2-logpipeline-correctness` | ⏳ IN PROGRESS |
| 2 | WO3 - Test Infrastructure | `wo3-test-infrastructure` | ⏸️ BLOCKED (Waiting on WO2) |
| 3 | WO4 - `dumpPayload()` API & Structural Scrubber | `wo4-dump-payload` | ⏸️ BLOCKED (Waiting on WO1-WO3) |
| 4 | WO5 - Callers and Docs | `wo5-callers-docs` | ⏸️ BLOCKED (Waiting on WO4) |
