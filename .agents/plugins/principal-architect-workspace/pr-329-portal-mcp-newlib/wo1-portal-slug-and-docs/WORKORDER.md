---
schema_version: 1
wo_id: WO-329.1
pr_id: PR-329
title: Harden findTabByHash empty-slug guard, pill-btn DOM lookup, Playwright hash
  tests, and portal slug docs
complexity: standard
status: PENDING
allowed_files:
- web/portal/app.js
- test/web/tests/navigation.spec.ts
- .agents/skills/framework-expert/systems/web-portal/html-js-fragments.md
- .agents/skills/framework-expert/systems/web-portal/overview.md
- docs/developer/extending-the-web-portal.md
forbidden_files:
- docs/design_specification.md
- README.md
- modules/U8G2Display/
- modules/U8G2GLDisplay/
- THIRD-PARTY-NOTICES.md
- test/render/
- scripts/deploy_release.py
required_skills: []
verification_command: python3 scripts/doc_lint.py && python3 scripts/run_web_tests.py
max_attempts: 5
max_structural_failures: 2
spec_hash: f3b9344f9f604170e7dc27c0cb425a643d08b20a24b2f6e8adfe91adfcf9d16c
completion: null
---

# WO-329.1 — Harden findTabByHash empty-slug guard, pill-btn DOM lookup, Playwright hash tests, and portal slug docs

## 1. Role & Objective

You are executing `WO-329.1` on stacked branch `pr-329-portal-mcp-newlib/wo1-portal-slug-and-docs` for `PR-329` (`prog-public-launch`). Base commit `c0df554f` already extracted `slugifyTabName(name)` across `web/portal/app.js` and added initial Playwright coverage in `test/web/tests/navigation.spec.ts`. Your objective is to harden `findTabByHash()`, extend Playwright hash navigation coverage, and update the portal slugification documentation:
1. Guard empty `normSlug` in `findTabByHash()` and eliminate CSS selector string interpolation on `candidateId` in `web/portal/app.js`.
2. Extend `test/web/tests/navigation.spec.ts` to cover `/#tab-intervals.icu` and `/#...` fallback behavior.
3. Update the container ID slug derivation formula in `.agents/skills/framework-expert/systems/web-portal/html-js-fragments.md`, `.agents/skills/framework-expert/systems/web-portal/overview.md`, and `docs/developer/extending-the-web-portal.md`.

## 2. Strict Boundary Box & Constraints

- **Allowed files (`allowed_files`):** Modify ONLY:
  - `web/portal/app.js`
  - `test/web/tests/navigation.spec.ts`
  - `.agents/skills/framework-expert/systems/web-portal/html-js-fragments.md`
  - `.agents/skills/framework-expert/systems/web-portal/overview.md`
  - `docs/developer/extending-the-web-portal.md`
  - plus Part II (`status`, `completion`, and `## Completion Report`) of this `WORKORDER.md`.
- **Forbidden files (`forbidden_files`):** Never touch `docs/design_specification.md`, `README.md`, `modules/U8G2Display/`, `modules/U8G2GLDisplay/`, `THIRD-PARTY-NOTICES.md`, `test/render/`, `scripts/deploy_release.py`, or any path outside `allowed_files`.
- **Tooling & Git Discipline:**
  - Read every file inside `.agents/rules/` in filename order before making edits.
  - NEVER use `*` or `**` globs in `run_command`.
  - NEVER mention protected filenames (`docs/design_specification.md` or `README.md`) in any `run_command` argument/string.
  - NEVER run raw `pio`, `platformio`, or `esptool` commands in `run_command`.
  - Do NOT create any git tag, upload firmware, or run `git push`, `git checkout`, `git switch`, `git branch`, `git rebase`, `git merge`, `gh pr`, or `gh issue`.

## 3. Line-Item Instructions

### 3.1 `web/portal/app.js` (`findTabByHash`, lines 581–626)
1. **Guard empty `normSlug` in top-level and child loops:**
   - In Loop 1 (`topLevelTabs`, line 594): change `topSlug === normSlug` to `(normSlug && topSlug === normSlug)` inside the `if (...)` condition.
   - In Loop 2 (`childTabs`, line 607): change `childSlug === normSlug` to `(normSlug && childSlug === normSlug)` inside the `if (...)` condition.
   - This ensures punctuation-only hashes (e.g. `#...`, where `slugifyTabName('...') === ''`) cannot accidentally match any tab whose name slugifies to `""`.
2. **Eliminate CSS selector interpolation in Step 3 DOM fallback (lines 614–623):**
   - Replace `var pillBtn = document.querySelector('.pill-btn[data-target="' + candidateId + '"]');` with a loop over `document.querySelectorAll('.pill-btn')` comparing `btn.getAttribute('data-target') === candidateId`:
     ```javascript
     var pillBtn = null;
     var pillBtns = document.querySelectorAll('.pill-btn');
     for (var k = 0; k < pillBtns.length; k++) {
       if (pillBtns[k].getAttribute('data-target') === candidateId) {
         pillBtn = pillBtns[k];
         break;
       }
     }
     ```
   - Preserve the subsequent `if (pillBtn) { ... }` return logic unchanged.

### 3.2 `test/web/tests/navigation.spec.ts` (around lines 359–409)
1. Extend the `'non-alphanumeric tab names (e.g. Intervals.icu) slugify consistently across DOM, pills, events, and URL hashes'` test (or add focused sibling tests in `Tab Navigation`):
   - **Direct navigation via `/#tab-intervals.icu`:** Verify navigating to `/#tab-intervals.icu` activates `#tab-intervals-icu` (`tab-content--active`), `#bottom-nav [data-target="tab-apps"]` (`tab-link--active`), and `#pill-intervals-icu` (`pill-btn--active`).
   - **Punctuation-only hash `/#...` fallback:**
     - Attach a `pageerror` listener and verify navigating to `/#...` throws zero page errors and falls back cleanly to the default STA tab (`#tab-settings` active).
     - Also verify with a tab list containing a punctuation-only tab (e.g., `{ name: '---', id: 'punct', parent: null, order: 60 }` alongside `dottedTabs`) that `/#...` does **not** match the empty-slug tab (`#tab-` / `#nav-`) and instead falls back cleanly to `#tab-settings`.
   - **Quote/metacharacter hash safety:** Verify navigating to a hash with selector metacharacters (e.g. `/#tab-foo"bar`) throws zero `pageerror` exceptions and falls back cleanly to `#tab-settings`.

### 3.3 Portal Slug Derivation Documentation
Update all three documentation files to reflect `slugifyTabName(name)` (`'tab-' + name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '')`, e.g. `'Intervals.icu'` -> `'tab-intervals-icu'`):
1. **`.agents/skills/framework-expert/systems/web-portal/html-js-fragments.md` (line 79):**
   - Replace `'tab-' + name.toLowerCase().replace(/\s+/g, '-')` with `'tab-' + name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '')` (noting e.g. `'Intervals.icu'` becomes `'tab-intervals-icu'`).
2. **`.agents/skills/framework-expert/systems/web-portal/overview.md` (line 77):**
   - Replace `'tab-' + name.toLowerCase().replace(/\s+/g, '-')` with `'tab-' + name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '')` (noting e.g. `'Intervals.icu'` becomes `'tab-intervals-icu'`).
3. **`docs/developer/extending-the-web-portal.md`:**
   - In `### Nav Resolution logic:` (§8, around line 520) and/or §4 (`## 4. The Tab Script Pattern`, around line 333), document clearly how the target container ID (`<div id="tab-...">`), `tab-changed` `e.detail`, and sub-pill ID (`#pill-...`) are derived from `name`: `'tab-' + name.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '')` (for example, `'Intervals.icu'` -> `'tab-intervals-icu'`, replacing non-alphanumeric runs with `-` and trimming leading/trailing hyphens). Ensure all changes pass `python3 scripts/doc_lint.py`.

## 4. Verification, Cryptographic Receipt & Completion Report

1. Run `python3 scripts/doc_lint.py && python3 scripts/run_web_tests.py` and confirm exit `0` with all web tests passing.
2. Run `/Users/mcneillm/.gemini/config/plugins/pa-daemon/bin/pa wo verify WO-329.1` to verify boundary hygiene, re-run `verification_command`, stamp the cryptographic `tree_hash` receipt, update `WORKORDER.md` / `PROJECT.md`, and commit to `pr-329-portal-mcp-newlib/wo1-portal-slug-and-docs` with `PA-WorkOrder` / `PA-Verified-Hash` trailers.
3. Ensure Part II (`status`, `completion`, and `## Completion Report`) is accurate, and report the commit SHA, receipt ID, modified files, and test counts back to the Principal Architect via `send_message`.

## Completion Report
<!-- Written by the Tier 3 Worker at task completion -->

_Pending execution._
