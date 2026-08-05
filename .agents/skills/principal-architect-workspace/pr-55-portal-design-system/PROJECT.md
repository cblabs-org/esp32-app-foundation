# PROJECT: pr-55-portal-design-system

**Issue:** [#67 — WebPortal RFC: Shared CSS component library and JS utility polish](https://github.com/matthewmcneill/esp32-app-foundation/issues/67)
**Base branch:** `main`
**Parent program:** standalone (not part of microkernel-refactor)

---

## Objective

Implement the shared CSS component library and JS utility enhancements proposed in RFC #67, restructured per the three-reviewer consensus into a phased approach that:

1. Settles the CSS naming convention (BEM going forward, legacy migrated)
2. Repairs existing debt (formalize `.card` and `.alert`, fix doc lies)
3. Extends the ApiQueue with priority lanes (harmonized task queue)
4. Adds genuinely universal new component vocabulary
5. Consolidates overlapping patterns (cards, rows, badges, dots)
6. Promotes the signal bars SVG renderer from wifi.js to shared utils

---

## Architectural Decisions (from RFC #67 thread)

| Decision | Rationale |
|---|---|
| **BEM `block--modifier` going forward** | `app.css` has 3 inconsistent conventions across 76 classes. Adopt BEM and migrate all legacy classes. |
| **Migrate legacy, don't grandfather** | User directive: "Let's pay down the debt now." |
| **`.card` as first-class CSS class** | Currently styled on bare `<article>`. Docs reference `.card` at `extending-the-web-portal.md:127`. Formalize it. |
| **`.alert` unification** | No `.alert` exists. Feedback fragmented across `.ap-banner`, `.form-error`, `.login-error`, `.toast-*`. Unify inline variants. |
| **ApiQueue priority lanes** | U1 task queue must share the single serialization point with `apiFetch()`. Two queues would defeat socket-pool protection. HIGH (UI) pre-empts LOW (background). |
| **`window.*` namespace** | All 10 existing utils use `window.*`. No `AppUtils` exists. Stay consistent. |
| **SVG signal bars (promote, not rewrite)** | Current `signalBarsIcon()` in wifi.js is SVG-based, theme-aware, resolution-independent. Promote to utils.js with `aria-label`. |
| **U1 absorbed into ApiQueue** | Not a separate utility — becomes `apiFetch.enqueueTask()` in app.js. |
| **A11y contracts mandatory** | Every new component ships with accessibility contract defined. |

---

## Status Log

| Date | Event |
|---|---|
| 2026-08-04 | RFC #67 filed by departures-board migration audit |
| 2026-08-04 | 3-reviewer analysis complete (owner + agent + design architect) |
| 2026-08-05 | PA decomposition drafted — 4 WOs |
| — | **Awaiting human approval of decomposition** |

---

## Work Order Ledger

| WO | Topic | Complexity | Dependencies | Status |
|---|---|---|---|---|
| **[ ] [wo1-naming-convention](wo1-naming-convention/WORKORDER.md)** | BEM ADR + legacy class migration | standard | none (bases on `main`) | 📋 Planned |
| **[ ] [wo2-debt-repair](wo2-debt-repair/WORKORDER.md)** | Formalize `.card`, `.alert`, ApiQueue priority lanes | architect | wo1 | 📋 Planned |
| **[ ] [wo3-new-primitives](wo3-new-primitives/WORKORDER.md)** | Core new components (#1, #8, #9+U3, #12, #14, #16, #17, #18) | standard | wo2 | 📋 Planned |
| **[ ] [wo4-consolidation](wo4-consolidation/WORKORDER.md)** | Card grid, dot, badge, list-item, signal bars (#2–7, #11, #15+U2) | standard | wo3 | 📋 Planned |

### Stacking topology

```
main
 └── pr-55-portal-design-system/wo1-naming-convention
      └── pr-55-portal-design-system/wo2-debt-repair
           └── pr-55-portal-design-system/wo3-new-primitives
                └── pr-55-portal-design-system/wo4-consolidation
```

---

## File Conflict Analysis

All 4 WOs touch `web/portal/app.css` — they **must be serialized** (stacked, not parallel).

| File | WO1 | WO2 | WO3 | WO4 |
|---|---|---|---|---|
| `web/portal/app.css` | ✏️ rename classes | ✏️ add `.card`, `.alert` | ✏️ add 8 components | ✏️ add card-grid, consolidate |
| `web/portal/app.js` | ✏️ update class refs | ✏️ ApiQueue priority lanes | — | — |
| `web/portal/utils.js` | — | — | ✏️ `renderTerminalLog` | ✏️ `renderSignalBars` |
| `web/portal/wifi.js` | — | — | — | ✏️ remove local `signalBarsIcon`, use shared |
| `web/portal/*.js` (all tabs) | ✏️ update class refs | ✏️ update `.ap-banner`→`.alert` refs | — | — |
| `web/portal/index.html` | ✏️ update class refs | ✏️ update markup | — | — |
| `docs/developer/extending-the-web-portal.md` | ✏️ update class refs | ✏️ document `.card`, `.alert` | ✏️ document new components | ✏️ document card-grid |
| `.agents/skills/.../shared-utilities.md` | ✏️ update class refs | ✏️ document ApiQueue | ✏️ document new utils | ✏️ document signal bars |
| `test/web/` (Playwright) | ✏️ update selectors | — | — | — |

> [!WARNING]
> **WO1 (legacy migration) is the riskiest WO** — it touches every portal JS file that references renamed classes and every Playwright test with class-based selectors. Must be verified with full Playwright suite and visual spot-check.

---

## Detailed WO Summaries

### WO1 — Naming Convention ADR + Legacy Class Migration

**Scope:** Adopt BEM convention. Migrate all existing classes that use inconsistent patterns. Write ADR. Update all references.

**CSS audit baseline (76 classes across 5 conventions):**

| Convention | Count | Examples | Migration |
|---|---|---|---|
| Hyphen-suffix variants | 13 | `.badge-ok`, `.toast-success`, `.rag-green` | → `.badge--ok`, `.toast--success`, `.rag--green` |
| Compound/state classes | 11 | `.dot.green`, `.tab-link.active`, `.pill-btn.active` | → `.dot--green`, `.tab-link--active`, `.pill-btn--active` |
| Prefixed components | 42 | `.status-badge`, `.info-row`, `.compact-row` | Already BEM-compatible (block names) — keep as-is |
| Element selectors | 9 | `article`, `dialog`, `button` | Keep (element reset, not class convention) |
| Standalone/base | 8 | `.badge`, `.dot`, `.toast`, `.container` | Keep (BEM block names) |

**~24 classes need renaming.** The 42 prefixed components and 8 base classes are already BEM-compatible block names.

### WO2 — Debt Repair

**Scope:** Formalize `.card` and `.alert` (classes docs/consumers assume exist), extend ApiQueue with priority lanes.

- `.card` — extract current `article` styling, keep `article` as alias
- `.alert` with `--success`, `--error`, `--warning`, `--info` modifiers — unify `.form-error` and `.ap-banner` inline variants
- ApiQueue `highQueue` / `lowQueue` with pre-emption

### WO3 — Core New Primitives

**Scope:** Uncontested new CSS components and JS utilities (RFC items #1, #8, #9+U3, #12, #14, #16, #17, #18). Each with a11y contract defined.

### WO4 — Consolidation + Signal Bars

**Scope:** Design-sensitive items needing reconciliation (RFC items #2–7, #11, #15+U2). Card grid system, dot custom property, badge modifier, list-item reconciliation, signal bars promotion.

---

## Open Questions

> [!IMPORTANT]
> **Q1:** Should we add Playwright snapshot tests for visual regression during WO1, or is the existing test suite sufficient?

> [!IMPORTANT]
> **Q2:** For `.alert` unification — should `.ap-banner` migrate to `.alert--warning`, or does its unique AP-mode positioning (shown/hidden by `body.ap-mode`) warrant keeping it separate?

> [!IMPORTANT]
> **Q3:** The `.list-item` / `.info-row` / `.compact-row` reconciliation in WO4:
> - **A)** Keep all three, document their semantic differences
> - **B)** Consolidate to `.list-row` with `--compact` and `--info` modifiers
> - **C)** Keep `.info-row` and `.compact-row` as-is, don't add `.list-item`
