---
name: pr-33-wo3
description: Handoff instructions for Work Order 3 — Playwright test coverage for reboot overlay
complexity: standard
---

# Role Definition

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

# Objective

Add Playwright test coverage for the universal reboot overlay, verifying that it appears with correct context-aware messages across the WiFi, System, and Security flows.

# Strict Boundary Box

- **DO** modify/create files in: `test/web/tests/`
- **DO** modify: `test/web/server.js` (if mock endpoint adjustments are needed)
- **DO NOT** modify any files in `web/portal/`
- **DO NOT** touch any C++ firmware files
- **DO NOT** create branches or make architectural decisions

# Tooling Constraints

- Use the `platformio` MCP server tool `run_tests` if applicable, or use `npx playwright test` in the `test/web/` directory.
- Before writing tests, read the existing test files in `test/web/tests/` to understand the project's testing patterns and mock server setup.

# Pre-Requisite

WO1 and WO2 must be merged first. All reboot paths must be using `window.showRebootOverlay()`.

# Line-Item Instructions

## Task 1: Research existing test patterns

- [ ] Read `test/web/server.js` to understand the mock server setup.
- [ ] Read 2-3 existing test files in `test/web/tests/` to understand the Playwright patterns used (locators, assertions, page navigation, mock API responses).

## Task 2: Create reboot overlay test file

- [ ] Create `test/web/tests/reboot-overlay.spec.js` with the following test cases:

### 2a. Overlay visibility tests
- [ ] **Test: "reboot overlay is hidden by default"** — Navigate to the portal, verify `#reboot-overlay` does not have the `visible` class.
- [ ] **Test: "showRebootOverlay displays with correct message"** — Call `window.showRebootOverlay('Test Message', 'Test sub-message')` via `page.evaluate()`, verify the overlay is visible and contains the expected text.
- [ ] **Test: "hideRebootOverlay removes overlay"** — Show then hide the overlay, verify it's no longer visible.

### 2b. WiFi flow tests
- [ ] **Test: "WiFi Save & Connect shows reboot overlay"** — Mock `/api/wifi/connect` to return success, click the Save & Connect button, verify the overlay appears with the WiFi-specific message.
- [ ] **Test: "WiFi Forget shows reboot overlay"** — Mock `/api/wifi/forget`, arm and confirm the forget button, verify the overlay appears with the AP mode message.

### 2c. System flow tests
- [ ] **Test: "System Restart shows reboot overlay"** — Navigate to the System → Status tab, arm and confirm the Restart button, verify the overlay appears.
- [ ] **Test: "Factory Reset shows reboot overlay"** — Navigate to the System → Status tab, type RESET in the confirmation input, click confirm, verify the overlay appears with the factory reset message.

### 2d. Security flow tests
- [ ] **Test: "TLS toggle shows reboot overlay"** — Navigate to the Security → TLS tab, mock `/api/auth/tls` to return `{ restartRequired: true }`, toggle the TLS switch, verify the overlay appears after the 2s delay.

## Task 3: Verify tests pass

- [ ] Run the full Playwright test suite from the `test/web/` directory.
- [ ] Ensure all new tests pass and no existing tests are broken.
- [ ] Report the total test count (existing + new).

# Verification Requirement

1. All new tests must pass.
2. No existing tests should be broken.
3. Test file follows the same patterns as existing tests (same import style, same locator patterns, same mock setup).

**CRITICAL:** Before distilling or committing, you MUST ask the user if they are happy. Only after approval should you invoke the `@distillery` skill.
