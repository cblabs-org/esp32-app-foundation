# ADR: WO3 — Core New Primitives Architecture and Implementation

## Context
Work Order 3 of the `pr-55-portal-design-system` program (RFC #67 implementation) adds 8 new uncontested CSS component patterns to `web/portal/app.css` and 1 shared JS utility (`renderTerminalLog`) to `web/portal/utils.js`.

## Architectural & Design Choices

1. **Loading Button State (`button[aria-busy="true"]`)**:
   - Uses `pointer-events: none; opacity: 0.7` to prevent interaction and visually communicate loading state.
   - Inject a CSS-only spinner via `::after` pseudo-element utilizing `@keyframes spin`.
   - Uses `currentColor` for outline/secondary buttons and white for primary buttons to maintain high contrast across themes.

2. **Instruction Callout (`.callout`)**:
   - Styled with `border-left: 4px solid var(--primary)` and `background: var(--primary-alpha)`.
   - Automatically respects light/dark theme contrast.
   - Recommended markup uses `<aside>` or `role="note"`.

3. **Terminal Log Viewer (`.terminal-log` & `window.renderTerminalLog`)**:
   - Visual: `#1a1a1a` background, `#4ade80` text, monospace font (`Courier New`).
   - Function: `window.renderTerminalLog(containerId)` attaches `role="log"` and `aria-live="polite"`.
   - Returns `{ append(line), clear(), scrollToBottom() }`.
   - Safe rendering: Uses `window.escapeHtml` on appended lines to prevent XSS vulnerabilities.

4. **Dialog Close Button (`.dialog-close-btn`)**:
   - `<dialog>` containers given `position: relative`.
   - Close button positioned `absolute; top: 0.75rem; right: 0.75rem;` with 24x24px sizing and `aria-label="Close"`.

5. **Password Mask Placeholder (`.password-mask::placeholder`)**:
   - Applied `letter-spacing: 4px` for clean spacing on bullet placeholders (`••••••••`).

6. **Field Validation Feedback (`.field-feedback`)**:
   - Standardized feedback elements with `.field-feedback--error` and `.field-feedback--success` modifiers.
   - Associated with inputs via `aria-describedby`.

7. **Disabled Section State (`article[aria-disabled="true"]`, `.card[aria-disabled="true"]`)**:
   - Applies `opacity: 0.5; pointer-events: none; filter: grayscale(30%);` to visually and interactively disable card sections gated by `IConfigGate`.

8. **Compact Row (`.compact-row`)**:
   - Preserved existing `.compact-row` flexbox styling in `app.css` and documented it thoroughly in developer guides.
