# Graveyard: Portal Bugs Discovered During Testing

These bugs were found during WO2 Playwright test development. They are NOT fixed — PR-15 is test infrastructure only.

## Bug 1: `order: 0 || 100` Falsy Evaluation

**File:** `web/portal/app.js` approximately line 158
**Code:** `childTabs` sorting uses `(a.order || 100)` 
**Problem:** JavaScript treats `0` as falsy, so `order: 0` becomes `100`. The Status sub-pill (order:0) gets sorted AFTER Keys (order:1), making Keys the default active System sub-pill instead of Status.
**Fix:** Replace `(a.order || 100)` with `(a.order != null ? a.order : 100)` or `(a.order ?? 100)`.
**Impact:** Low — cosmetic only. Users see Keys tab first instead of Status when clicking System.
**Tests:** All navigation specs account for this bug and do NOT assume Status is the default child.

## Bug 2: Tab Parent Casing Mismatch

**Observed in:** Mock server data vs portal's `tabIdToName()` function
**Problem:** The portal's `tabIdToName()` returns `'System'` (proper case). Child tabs use `parent` field to reference their parent. The `childTabs` lookup does `childTabs['System']`, but if the firmware sends `parent: 'system'` (lowercase), the children won't be found.
**Resolution during testing:** Mock data in test specs was corrected to use `parent: 'System'` (matching the parent's `name` field). The original mock server was left with lowercase for baseline compatibility.
**Verification needed:** Check what the real firmware sends in the `/api/tabs` response for the `parent` field.
**Impact:** If the real firmware sends lowercase, the sub-pill bar will be empty on hardware.
