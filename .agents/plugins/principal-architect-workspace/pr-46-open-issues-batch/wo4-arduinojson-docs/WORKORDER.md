---
name: open-issues-batch-wo4
description: "Handoff instructions for Work Order 4: ArduinoJson v7 documentation remediation (Issue #29)"
complexity: mechanical
---

# WO4 — Issue #29: ArduinoJson v7 Documentation Remediation

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Remove all references to the deprecated `StaticJsonDocument` and `DynamicJsonDocument` types across the framework's skill docs and developer docs. These types were removed in ArduinoJson 7.0, which is the version the framework pins. Replace with correct ArduinoJson 7 guidance using `JsonDocument`, and reframe the allocation-bounding rationale around the body-size middleware (`getMaxPostBody()`) rather than the document type.

## Parent Project

`pr-46-open-issues-batch` — user approved the decomposition on 2026-07-22. See `PROJECT.md` in the parent directory.

## Strict Boundary Box

- You MUST ONLY modify documentation files (.md) — NO C++ source code, NO JS, NO build files.
- You MUST NOT change any code in `modules/`, `src/`, `web/`, or `test/`.
- The archive file `docs/archive/comparative_analysis_report.md` MAY be left as-is (it's a historical artifact) or updated with a note — your choice, but do not delete it.
- Surface any out-of-scope findings to the PA in your completion message.

## Tooling Constraints

- No build tools needed. This is a docs-only work order.
- After all edits, run a final grep to confirm zero remaining occurrences:
  ```bash
  grep -rn "StaticJsonDocument\|DynamicJsonDocument" .agents/skills/framework-expert/ docs/
  ```

## Target Files & Required Changes

The following files contain `StaticJsonDocument` and/or `DynamicJsonDocument` references that must be updated:

### Skill Docs (`.agents/skills/framework-expert/`)

- [ ] **1. `embedded-safety/memory-rules.md`** (lines 29, 32, 33):
  - Rewrite section "§4. StaticJsonDocument for POSTs (ADR-15)" to use `JsonDocument`.
  - Explain that ArduinoJson 7 removed `StaticJsonDocument`/`DynamicJsonDocument` and replaced both with a single elastic `JsonDocument`.
  - Reframe the allocation-bounding rationale: the primary mechanism is now the body-size middleware (`getMaxPostBody()`) which caps the incoming body before parsing. A `JsonDocument` fed a body that is already bounded by the middleware cannot over-allocate.
  - Optionally mention `deserializeJson(doc, body, maxLen)` and `DeserializationOption::NestingLimit` as defense-in-depth.

- [ ] **2. `interfaces/web-contributor.md`** (line 30):
  - Replace `StaticJsonDocument<256> doc; // Or JsonDocument in v7` with just `JsonDocument doc;` and remove the v7 comment (v7 is the only version).

- [ ] **3. `performance/heap-management.md`** (lines 27, 29, 31):
  - Replace all `StaticJsonDocument` references with `JsonDocument`.
  - Update the "Safe in tick()" / "Avoid in tick()" guidance — `JsonDocument` is the correct type. The heap concern is bounded by input size (middleware) not document type.
  - Remove `DynamicJsonDocument` from the "Avoid" list — it no longer exists.

- [ ] **4. `systems/web-portal/routes.md`** (lines 18, 48, 58):
  - **Rule 2 rewrite**: Replace the mandate for `StaticJsonDocument` with `JsonDocument`. State plainly that `StaticJsonDocument`/`DynamicJsonDocument` were removed in ArduinoJson 7.
  - Connect the allocation-bounding story to `getMaxPostBody()` (rule 1): a handler whose contributor declares `getMaxPostBody()` cannot be handed a body larger than that limit, so the parse is bounded by the declared limit rather than by the document type.
  - Update the canonical code examples to use `JsonDocument doc;` (no template parameter).

### Developer Docs (`docs/`)

- [ ] **5. `docs/developer/embedded-safety.md`** (lines 36, 43, 104, 111, 128):
  - Rewrite "Rule 4: `StaticJsonDocument` for POST Body Parsing (ADR-15)" using the same approach as the skill doc version above.
  - Update all code examples to `JsonDocument doc;`.
  - Add a note that ArduinoJson 7 replaced v6 types.

- [ ] **6. `docs/developer/extending-the-web-portal.md`** (line 301):
  - Already has a partial v7 note — verify it's complete and remove any remaining `StaticJsonDocument` reference in the surrounding context.

- [ ] **7. `docs/developer/performance-guide.md`** (line 79):
  - Replace `StaticJsonDocument<512> _doc;` with `JsonDocument _doc;` in the code example.

- [ ] **8. `docs/developer/sample-app-walkthrough.md`** (line 805):
  - Update the audit table row to reflect the fix (change status from "🟡 Stale API version" to "✅ Fixed — uses `JsonDocument`" or similar).

- [ ] **9. `docs/design_specification.md`** (lines 993, 996, 1005, 2661, 2788):
  - Lines 993–1005: Update the embedded-safety cross-reference and code example.
  - Line 2661: Update the config checklist item.
  - Line 2788: Update the D33 decision table row — it already describes the v7 change, verify it's consistent.

- [ ] **10. `docs/archive/comparative_analysis_report.md`** (line 152):
  - This is an archived historical document. Add a `[Note: ArduinoJson 7 — see D33]` annotation or leave as-is. Do not delete the archive.

### Final Verification

- [ ] **11. Run the final grep** to confirm zero remaining occurrences in active docs (excluding archives if you chose to leave the archive):
```bash
grep -rn "StaticJsonDocument\|DynamicJsonDocument" .agents/skills/framework-expert/ docs/developer/ docs/design_specification.md
```
Expected output: no matches.

## Verification Requirement

After completing all checklist items:
1. Confirm the final grep returns zero matches (excluding `docs/archive/` if left as-is).
2. **STOP and ask the user** if they are happy with the changes.
3. Only after user approval: commit your changes with message `docs: replace StaticJsonDocument/DynamicJsonDocument with ArduinoJson 7 JsonDocument (Issue #29)`, then invoke the `@distillery` skill to package the state, passing `.agents/plugins/principal-architect-workspace/pr-46-open-issues-batch/wo4-arduinojson-docs/` as the target directory.
4. Append a status update to this WORKORDER.md and send a completion message to the PA.
