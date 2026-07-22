---
name: open-issues-batch-wo1
description: "Handoff instructions for Work Order 1: Fix child tab content div creation (Issue #37)"
complexity: mechanical
---

# WO1 — Issue #37: Fix Child Tab Content Div Creation

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix the bug where app-contributed **child tabs** (sub-pills) render blank content panes because `buildBottomNav()` in `app.js` only creates `<div class="tab-content">` containers for top-level tabs, not for children.

## Parent Project

`pr-46-open-issues-batch` — user approved the decomposition on 2026-07-22. See `PROJECT.md` in the parent directory.

## Strict Boundary Box

- You MUST ONLY modify files listed in the checklist below.
- You MUST NOT touch any C++ source files, build system files, or documentation outside the portal.
- You MUST NOT modify the `activateSubPill()` or `showContentDiv()` functions — the fix goes in `buildBottomNav()`.
- You MUST NOT create new files beyond what's listed.
- Surface any out-of-scope findings to the PA in your completion message.

## Tooling Constraints

- Use the `platformio` MCP tools for any build verification (not raw `pio` CLI).
- For web tests, run: `cd test/web && npx playwright test`

## Line-Item Instructions

- [ ] **1. Read the current code**: Open `web/portal/app.js` and read the `buildBottomNav()` function (approximately lines 217–280) to understand the existing top-level tab content-div creation loop.

- [ ] **2. Add child tab content-div creation**: After the `topLevelTabs` loop in `buildBottomNav()` (after the closing of the `for (var i = 0; i < topLevelTabs.length; i++)` block which creates content divs and nav links), add a new loop that creates content divs for all child tabs:

```js
// --- Create content containers for child (sub-pill) tabs ---
// Without these, child tab fragments have no target div and render blank.
// (Fixes GitHub Issue #37)
for (var p in childTabs) {
  for (var j = 0; j < childTabs[p].length; j++) {
    var child = childTabs[p][j];
    var childId = 'tab-' + child.name.toLowerCase().replace(/\s+/g, '-');
    if (!document.getElementById(childId)) {
      var cdiv = document.createElement('div');
      cdiv.id = childId;
      cdiv.className = 'tab-content';
      document.querySelector('.tab-content-container').appendChild(cdiv);
    }
  }
}
```

Place this **after** the main top-level loop and **before** the function's closing brace. The `childId` derivation (`tab-` + lowercase name with spaces replaced by hyphens) MUST match the existing pattern used in `activateSubPill()`.

- [ ] **3. Run the Playwright web tests** to verify no regressions:
```bash
cd test/web && npx playwright test
```
All existing tests must pass.

- [ ] **4. Verify the fix manually**: Confirm by reading the code that `showContentDiv(childId)` in `activateSubPill()` will now find the div that was created by your new loop.

## Verification Requirement

After completing all checklist items:
1. Confirm all Playwright tests pass.
2. **STOP and ask the user** if they are happy with the changes.
3. Only after user approval: commit your changes with message `fix(portal): create content divs for child tabs (Issue #37)`, then invoke the `@distillery` skill to package the state, passing `.agents/skills/principal-architect-workspace/pr-46-open-issues-batch/wo1-child-tabs/` as the target directory.
4. Append a status update to this WORKORDER.md and send a completion message to the PA.

## Status Log

| Date | Event |
|---|---|
| 2026-07-22 | Worker agent read WORKORDER, verified provenance via PROJECT.md |
| 2026-07-22 | Implemented child-tab content-div loop in `buildBottomNav()` (16 lines added to `web/portal/app.js`) |
| 2026-07-22 | Playwright tests: **69/69 passed** — no regressions |
| 2026-07-22 | Manual verification: confirmed `childId` derivation matches `buildPillBar()`, `activateTopTab()`, `activateSubPill()` |
| 2026-07-22 | PA approved — committed as `1e9f855` on `open-issues-batch/wo1-child-tabs` |
| 2026-07-22 | Distillery packaged: `context_bridge.md`, `sessions.md`, `context_artifacts/adr_child_tab_divs.md` |
| 2026-07-22 | **WO1 COMPLETE** ✅ |
