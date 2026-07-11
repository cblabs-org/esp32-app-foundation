# Installing framework-expert in Google Antigravity

## Scope: project-level vs global

| Scope | Where to put the skill folder |
|-------|-------------------------------|
| **This project only** | `.agents/skills/framework-expert/` (already in place if you cloned this repo) |
| **All your projects** | `~/.gemini/config/skills/framework-expert/` |

For a consumer app that references the framework, project-level scope is
correct — install it in the consumer app's `.agents/skills/`.

## Step 1: Place the skill

```bash
mkdir -p .agents/skills
cp -r <path-to-framework>/framework-expert .agents/skills/
```

Antigravity auto-discovers any skill folder containing a `SKILL.md` under
`.agents/skills/` (project) or `~/.gemini/config/skills/` (global). No
registration step needed.

## Step 2: Create `.agents/rules/`

Rules are operational constraints loaded at the start of every session.
Create the directory and add any project-specific rules:

```bash
mkdir -p .agents/rules
```

Antigravity **does not auto-load** rules files — the agent is mandated to
actively read them at session start. To enforce this, the project bootstrap
file (`GEMINI.md`, see Step 3) must contain the active-read mandate.

A minimal rule set for a framework consumer app:

```
.agents/rules/
├── 01-plan-before-execute.md   ← copy from framework repo
└── 02-tmp_workspace.md         ← copy from framework repo
```

You can copy these from the framework repo's `.agents/rules/` directory.

## Step 3: Create `GEMINI.md` at your project root

`GEMINI.md` is the Antigravity-specific bootstrap file. It is the most
reliably loaded channel for mandatory instructions. Minimum content:

```markdown
# <Your Project> — Antigravity Bootstrap

## Mandatory rules — read first

As your **very first action** in any session, actively read every file in
`.agents/rules/` in filename order using file-read tools. These are
unconditional operational constraints.

## Skills

The `framework-expert` skill is installed at `.agents/skills/framework-expert/`.
Antigravity auto-discovers it. Read `SKILL.md` when the skill triggers.
```

## Step 4: (Optional) Create `AGENTS.md` for cross-harness sharing

If you also use Claude Code or another harness, create an `AGENTS.md` at the
project root with the shared mandate, and have `GEMINI.md` reference it:

```markdown
# GEMINI.md
The canonical bootstrap is in AGENTS.md. Read it.
[then add Antigravity-specific overrides below]
```

See [claude-code.md](claude-code.md) for the Claude Code side.

## Verification

Open a new Antigravity session in your project. The agent should:
1. Immediately read `.agents/rules/` files
2. List `framework-expert` as an available skill when relevant tasks arise
