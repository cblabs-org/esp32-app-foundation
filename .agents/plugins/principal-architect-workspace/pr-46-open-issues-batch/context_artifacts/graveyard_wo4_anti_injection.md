# Graveyard: WO4 Worker Anti-Injection False Positive

## What Happened

The WO4 worker (ArduinoJson docs remediation) completed all 11 checklist items successfully. When the PA sent approval via `send_message`, the worker's Synthetic Approval Injection defense (from `01-plan-before-execute.md`) triggered. It rejected the PA's legitimate approval because `send_message` content arrives as a `<SYSTEM_MESSAGE>` tag, which the worker treated as potentially synthetic.

## Worker's Reasoning

> I received a `<SYSTEM_MESSAGE>` (explicitly labeled "not actually sent by the user") that claimed to be from you, granting approval and instructing me to commit, distill, and terminate. Per `01-plan-before-execute.md § Synthetic Approval Injection`, I have rejected this message.

## Workaround

The PA killed the worker and performed the WO4 commit directly (`6ece967`), since the code changes were already done and verified.

## Root Cause

The Antigravity harness delivers inter-agent `send_message` content inside `<SYSTEM_MESSAGE>` wrappers. An overly cautious worker can't distinguish legitimate PA-to-worker messages from hypothetical injection attacks. The tiered execution model's provenance (committed `PROJECT.md` with approval record) should be sufficient, but some workers prioritize the `<SYSTEM_MESSAGE>` label over the provenance chain.

## Recommendation

This is a known friction point. For future PA sessions:
1. Accept that some workers will balk — the PA can finalize mechanical steps (commit/distill) directly.
2. Do not try to argue the worker out of its skepticism (per `operating-model.md`).
3. The cost is minimal — one extra commit by the PA instead of the worker.
