---
name: pr-29-wo10
description: Handoff instructions for Work Order 10 — Fix MCP bearer key UI when password set
complexity: mechanical
---

# Work Order 10 — Fix MCP Bearer Key UI When Password Set

## Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Fix two bugs in the Security tab's MCP bearer token section:
1. When a password is set, clicking the reveal button fails to display the token
2. Icons in the MCP token section are non-standard/inconsistent

## Versioning

Read `.agents/rules/03-versioning.md`. This is a UI bug fix — a dev tag increment
is warranted when building (web assets are part of the firmware).

## Strict Boundary Box

- You **MAY** modify `web/portal/auth.js`
- You **MAY** modify `web/portal/app.css`
- Do **NOT** modify any C++ source files
- Do **NOT** create any branches or make git commits

## Background

### MCP Token UI Architecture

The MCP bearer token is displayed in `web/portal/auth.js` (lines 268-281):

```html
<article>
  <header>MCP Bearer Token</header>
  <p>Use this token in your AI agent config to authenticate MCP requests...</p>
  <div class="token-display">
    <code id="mcp-token-value" class="token-masked">••••••••••••••••</code>
    <button id="reveal-token-btn" class="btn-icon" title="Show/hide token">👁</button>
    <button id="copy-token-btn" class="btn-icon" title="Copy to clipboard">📋</button>
  </div>
</article>
```

### Token Reveal Flow (lines 626-694)

1. User clicks `#reveal-token-btn`
2. `toggleTokenReveal()` is called
3. If token not yet fetched, calls `GET /api/auth/mcp-token` (session-gated)
4. Server returns `{token: "<hash>"}`
5. Token is cached in `fetchedToken` variable
6. `#mcp-token-value` textContent is swapped and `token-masked` class is toggled

### The Bug

When a password is set, the `GET /api/auth/mcp-token` endpoint requires an
authenticated session (session cookie). The likely failure scenarios:

1. **Session cookie not being sent**: The `apiFetch()` call may not include
   credentials, or the cookie may not be set correctly
2. **API returns 401**: The token reveal JS may not handle the auth failure
   gracefully — it may silently fail without showing an error
3. **Response parsing**: The `data.token` extraction may fail if the response
   format differs when auth is enabled

### Icon Issues

The reveal button uses `👁` (emoji eye) and copy button uses `📋` (emoji clipboard).
The rest of the portal uses SVG icons or a consistent icon set. These should be
standardised.

## Line-Item Instructions

### Step 1: Debug the token reveal flow

- [ ] **1a.** Read `web/portal/auth.js` — find the `toggleTokenReveal()` function
  (lines ~626-694).

- [ ] **1b.** Check if `apiFetch()` or the fetch call for `/api/auth/mcp-token`
  includes `credentials: 'include'` or `credentials: 'same-origin'`. If not,
  the session cookie won't be sent and the server will return 401.

- [ ] **1c.** Check the error handling in the token reveal:
  - What happens if the fetch returns a non-200 status?
  - Is there a `try/catch` around the JSON parsing?
  - Does the UI show an error message, or does it silently fail?

- [ ] **1d.** Fix the authentication issue:
  - Ensure the fetch includes credentials
  - Add proper error handling for 401 responses
  - Show an error toast/message if the token fetch fails

### Step 2: Fix error handling

- [ ] **2a.** Add proper error handling to `toggleTokenReveal()`:
  ```javascript
  try {
      const response = await apiFetch('/api/auth/mcp-token');
      if (!response.ok) {
          showStatus('Failed to retrieve MCP token', 'error');
          return;
      }
      const data = await response.json();
      fetchedToken = data.token;
  } catch (e) {
      showStatus('Error fetching MCP token: ' + e.message, 'error');
      return;
  }
  ```

### Step 3: Standardise icons

- [ ] **3a.** Survey the rest of the portal for the icon pattern used. Check
  `web/portal/index.html` and other JS files for how icons are rendered (SVG
  inline, icon class, or unicode).

- [ ] **3b.** Replace the emoji icons with the portal's standard icon approach:
  - Replace `👁` (reveal) with an appropriate SVG eye icon
  - Replace `📋` (copy) with an appropriate SVG clipboard icon
  - Ensure the icons match the visual style of other portal buttons

### Step 4: Build and verify

- [ ] **4a.** Build for esp32s3nano (this compiles web assets):
  ```bash
  pio run -e esp32s3nano
  ```

- [ ] **4b.** Build for esp32dev:
  ```bash
  pio run -e esp32dev
  ```

- [ ] **4c.** Run native tests:
  ```bash
  pio test -e native
  ```

### Step 5: Test on device

- [ ] **5a.** Flash and test:
  1. Set a password in the Security tab
  2. Login with the password
  3. Click the reveal button on the MCP bearer token
  4. Confirm the token is displayed
  5. Click copy and confirm clipboard works
  6. Verify icons match the rest of the portal

## Verification Requirement

After completing all checklist items:
1. Confirm both boards build cleanly
2. Confirm native tests pass
3. Confirm token reveal works when password is set
4. Confirm icons are consistent with the rest of the portal
5. **STOP and send a message back to the Principal Architect** reporting completion.
   Do NOT commit, do NOT distill, and do NOT make any further changes until explicitly approved.
