# Work Order 4: Systems Domain Trees

## Context
We are restructuring the `framework-expert` skill. Phase 1 (Core Skeleton) is complete. You are executing Phase 2 (parallel execution) — specifically, **WO4: Systems Domain Trees**. Your job is to create deep-dive reference trees for the four major framework subsystems: Display, Config, Event Bus, and Web Portal.

## ⚠️ Mandatory Pre-Flight Checklist
1. **Read `.agents/rules/` first** — read all files in this directory in filename order.
2. **Load the skill-creator skill** — use `view_file` on `~/.gemini/config/skills/skill-creator/SKILL.md`. You MUST apply its principles of progressive disclosure and token budgeting.
3. **Plan-gate suspension** — The "Plan Gate" rule (`01-plan-before-execute.md`) is SUSPENDED for this Work Order. You have explicit, pre-approved authorization. Do NOT ask for permission to proceed.

## Context Artifacts
Read the Principal Architect's implementation plan for details:
`~/.gemini/antigravity/brain/5ad94103-1cd2-41df-9eec-19be9c51778c/implementation_plan.md` (Read the WO4 section)

## Execution Steps

Work in `.agents/skills/framework-expert/systems/`. You will create directories and files for 4 subsystems.

### 1. `systems/display/` (5 files)
- `overview.md` (~80 lines): U8G2 as reference implementation, two-phase bootstrap, multi-interface pattern, Application ownership, three-layer rendering.
- `carousel.md` (~70 lines): CarouselEngine API, board pool, rotation timing, board registration.
- `custom-boards.md` (~80 lines): IDisplayBoard full lifecycle, creating a telemetry pane, system boards reference.
- `replacing-the-display.md` (~90 lines): ADR-02 rationale (no unified IDisplayDriver), how to swap U8G2 for TFT/e-ink/headless, which interfaces to implement, Application ownership implications.
- `screensaver.md` (~60 lines): ScreensaverController API, inactivity policy, display sleep/wake events, power implications.

### 2. `systems/config/` (3 files)
- `overview.md` (~70 lines): ConfigManager gateway pattern, file ownership, registry lifecycle.
- `persistence.md` (~80 lines): `saveModuleConfig()`, `exportAllConfig()`/`importAllConfig()`, ConfigManifest three-tier backup (Active/Previous/Golden).
- `framework-config.md` (~50 lines): FrameworkConfig struct vs IConfigurable, compile-time secrets, what goes where.

### 3. `systems/event-bus/` (3 files)
- `overview.md` (~70 lines): Pub/sub API signatures, payload lifetime rule (WARNING), static fn-ptr rationale (GRAVE-05).
- `framework-topics.md` (~60 lines): All 8 framework-published topics with payload structs and publisher identity.
- `custom-topics.md` (~50 lines): Declaring module-owned topics, namespace convention, payload struct design.

### 4. `systems/web-portal/` (4 files)
- `overview.md` (~60 lines): SPA architecture, tab registry, `/api/tabs` JSON schema, dynamic fragment loading. Include `AppTabContributor` pattern.
- `routes.md` (~70 lines): PsychicHttp route patterns, GET/POST lambdas, POST safety (ADR-15, StaticJsonDocument, 4096 body cap).
- `html-js-fragments.md` (~60 lines): `tab.html` structure (no wrapping tags), `tab.js` IIFE pattern, DOM injection lifecycle.
- `security.md` (~50 lines): JWT auth middleware, AuthManager, public path bypass (`/api/auth/login`, static assets).

## Acceptance Criteria
- [ ] Display tree frames U8G2 as replaceable reference implementation.
- [ ] `replacing-the-display.md` covers ADR-02 and practical swap guidance.
- [ ] Config tree documents the three-tier ConfigManifest.
- [ ] EventBus tree documents all 8 framework topics with full payload structs.
- [ ] Web portal tree documents POST safety (ADR-15) and JWT middleware.
- [ ] Each file includes cross-links to relevant interface files from WO2 (e.g., `interfaces/display-board.md`). You can use relative links like `../../interfaces/display-board.md`.

When complete, STOP and report back. Do NOT write a distilled summary.
