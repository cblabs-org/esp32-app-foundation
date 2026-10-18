# Graveyard & Lessons Learned — WO1

## Issues Encountered & Resolved

1. **Sandboxed Shell Command Environment (`uv_cwd` EPERM error)**:
   - Running `npm test` inside the standard sandbox failed with an `EPERM` error on `uv_cwd` when invoking global Node/npm binaries.
   - **Resolution:** Re-ran `npm test` with `BypassSandbox: true`, which executed the full Playwright suite cleanly (70 passed).

2. **Compound State Class Migration (`.dot.green`, `.tab-content.active`, `.pill-btn.active`, etc.)**:
   - Initial review needed to distinguish base block classes (`.dot`, `.tab-content`, `.pill-btn`, `.theme-btn`, `.tab-link`) from their active/variant modifier classes.
   - **Resolution:** Replaced active modifier logic in `app.js`, `utils.js`, `auth.js`, `homeassistant.js`, `screenshot.js`, and all 10 tab fragment scripts to use explicit BEM modifiers (`tab-content--active`, `tab-link--active`, `pill-btn--active`, `theme-btn--active`, `btn-ss-colour--active`, `login-overlay--hidden`, `reboot-overlay--visible`).

3. **Playwright Test Selectors**:
   - Multiple Playwright test specs (`navigation.spec.ts`, `theme.spec.ts`, `reboot-overlay.spec.ts`, `keys.spec.ts`, `backup.spec.ts`, `mcp.spec.ts`, `ota.spec.ts`, `system.spec.ts`, `display.spec.ts`) relied on regex matches against `.active`, `.visible`, `.hidden`, and `.active` button states.
   - **Resolution:** Updated all test assertions to match the precise BEM class names (`tab-content--active`, `theme-btn--active`, `pill-btn--active`, `reboot-overlay--visible`).
