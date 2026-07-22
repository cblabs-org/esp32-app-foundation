# WO2 — Modified Files Summary

## Files Changed

### 1. `web/portal/app.js`
- **Auth retry throw** (was line 41, now lines 41–53): Replaced `throw new Error('HTTP ' + retryResponse.status)` with body-preserving error construction attaching `.status` and `.body`.
- **Main error throw** (was line 51, now lines 63–75): Identical pattern applied to the main non-2xx path.
- **Success path** (lines 77–82): Untouched per boundary constraint.
- Net: +24 lines (2 simple throws → 2 body-preserving blocks).

### 2. `.agents/skills/framework-expert/systems/web-portal/shared-utilities.md`
- **Return type**: Changed from `Promise<Response>` to `Promise<object|string>`.
- **Usage example**: Removed erroneous `await response.json()` call. Added both success and error examples.
- **Error shape table**: New documentation for `err.message`, `err.status`, `err.body`.
- Net: +23 lines.

## Commit

`079f5c6` on branch `open-issues-batch/wo2-apifetch-error-body`
Message: `fix(portal): preserve error body in apiFetch on non-2xx (Issue #31)`
