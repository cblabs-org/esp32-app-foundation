---
name: framework-setup
description: >
  Quick-start bootstrapper for new ESP32 App Foundation consumer projects.
  Use this skill to set up a blank folder into a fully-wired development
  environment with the framework cloned, agent skills installed, and harness
  configuration (Antigravity, Claude Code) in place. Trigger on: "new project",
  "blank folder", "set up the framework", "bootstrap ESP32", "I want to build
  an ESP32 device", "get started with App Foundation", "clone the framework",
  "set up my development environment", "start a new IoT project", "install
  framework skills", "how do I get started", "scaffold a new app". Also trigger
  when the user opens a blank or near-empty directory and mentions ESP32, IoT,
  firmware, or embedded device — even if they don't say "App Foundation"
  explicitly.
---

# Framework Setup

Bootstraps a blank folder into a ready-to-develop ESP32 App Foundation
consumer project. This skill handles **environment wiring only** — cloning the
framework, installing agent skills, and configuring harnesses. It then hands
off to `framework-expert` for the actual firmware design and scaffolding.

## What is the ESP32 App Foundation?

The ESP32 App Foundation is an open-source, contract-based C++ framework for
ESP32 devices. Instead of writing monolithic firmware, you implement small
modules against pure virtual interfaces (`ITickable`, `IConfigurable`,
`IWebContributor`, etc.) and register them with the framework. It handles WiFi,
configuration persistence, OTA updates, a web portal, a display carousel, and
an on-device MCP server out of the box — you focus on your application logic.

**Repository:** `https://github.com/cblabs-org/esp32-app-foundation`

---

## Step 1 — Detect Environment

Before doing anything, assess the current workspace:

```
Check for:
  ✓ framework/          → framework already cloned?
  ✓ .agents/skills/framework-expert/  → skills already installed?
  ✓ AGENTS.md           → harness bootstrap already configured?
  ✓ src/main.cpp        → project already scaffolded?
```

**If all are present:** Tell the user the environment is already set up and
suggest they use `framework-expert` directly. Stop here.

**If `framework/` exists but skills are missing:** Skip to Step 3.

**If nothing exists:** Inform the user:

> "I can see you're starting from a blank folder. I'll clone the ESP32 App
> Foundation, install the agent skills, and wire up your development
> environment. This takes about a minute."

**Wait for user confirmation before proceeding.**

---

## Step 2 — Add the Framework as a Submodule

```bash
git submodule add https://github.com/cblabs-org/esp32-app-foundation.git framework
git submodule update --init
```

This tracks the framework as a git submodule inside `framework/`. When the
framework releases updates, the user runs `git submodule update --remote` to
pull them — and because Step 3 uses symlinks, the skills update automatically.

> **If the project isn't a git repo yet**, initialise one first:
> `git init && git submodule add ...`

---

## Step 3 — Symlink Agent Skills

Create symlinks from `.agents/skills/` into the framework submodule. This
ensures skills always reflect the framework version currently checked out —
no manual copy/refresh needed.

```bash
mkdir -p .agents/skills
ln -s ../../framework/.agents/skills/framework-expert .agents/skills/framework-expert
ln -s ../../framework/.agents/skills/framework-portal-design .agents/skills/framework-portal-design
```

These two skills provide:
- **framework-expert** — the main development skill covering interfaces,
  patterns, security, design specs, and project scaffolding.
- **framework-portal-design** — UI kit, brand tokens, and portal styling
  guidelines.

> **Note:** Additional skills (embedded-systems,
> lvgl-pro-expert, etc.) are available in the framework submodule at
> `framework/.agents/skills/`. Symlink more as needed:
> `ln -s ../../framework/.agents/skills/<name> .agents/skills/<name>`

---

## Step 4 — Wire Agent Harnesses

### 4a — Create baseline rules

Copy the minimum operational rules for a consumer app:

```bash
mkdir -p .agents/rules
cp framework/.agents/rules/01-plan-before-execute.md .agents/rules/
cp framework/.agents/rules/02-tmp_workspace.md .agents/rules/
```

### 4b — Create `AGENTS.md` (cross-harness bootstrap)

Write the following to `AGENTS.md` at the project root:

```markdown
# <ProjectName> — Agent Bootstrap

## ⚠️ Mandatory First Step

Before doing anything else, read every file inside `.agents/rules/` in
filename order. These are unconditional operational constraints.

## Skills

The following skills are installed at `.agents/skills/`:
- `framework-expert` — ESP32 App Foundation development guidance
- `framework-portal-design` — portal UI and branding

Read a skill's `SKILL.md` when it triggers.
```

Replace `<ProjectName>` with the name of the user's project.

### 4c — Create `GEMINI.md` (Antigravity bootstrap)

Write the following to `GEMINI.md` at the project root:

```markdown
# <ProjectName> — Antigravity Overrides

The canonical agent bootstrap lives in `AGENTS.md`. Read it first.

## Mandatory rules — reliability backstop

As your **very first action** in any session, actively read every file in
`.agents/rules/` in filename order using file-read tools — do not assume
they were auto-loaded.
```

### 4d — Create `.claude` symlink (Claude Code support)

```bash
ln -s .agents .claude
```

Add to `.gitignore` if not already present:

```bash
grep -qxF '.claude' .gitignore 2>/dev/null || echo '.claude' >> .gitignore
```

This gives Claude Code access to skills and rules via its expected `.claude/`
path without duplicating anything.

---

## Step 5 — Verify

Confirm the setup is correct by checking all expected paths exist:

```
.agents/
├── rules/
│   ├── 01-plan-before-execute.md
│   └── 02-tmp_workspace.md
└── skills/
    ├── framework-expert/
    │   └── SKILL.md
    └── framework-portal-design/
        └── SKILL.md
AGENTS.md
GEMINI.md
.claude → .agents  (symlink)
framework/          (cloned repo)
```

Report any missing items to the user.

---

## Step 6 — Next Steps

Tell the user the environment is ready, then advise them on what to do next:

> **Your development environment is set up!** Here's how to get started:
>
> 1. **Design your app first.** Ask me to help you design your application —
>    I'll walk you through an interview to understand what you're building and
>    generate a design specification document. Just say something like:
>    *"Help me design my app"* or *"I want to build a weather station."*
>
> 2. **Or jump straight to scaffolding.** If you already know what you want,
>    say *"Bootstrap my project"* and I'll generate `platformio.ini`,
>    `src/main.cpp`, and your application module with the right interfaces
>    wired up.
>
> The `framework-expert` skill is now available and will activate automatically
> when you ask about building, designing, or extending your ESP32 application.
