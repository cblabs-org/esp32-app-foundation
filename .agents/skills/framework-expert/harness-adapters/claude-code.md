# Installing framework-expert in Claude Code

Claude Code uses a `.claude/` directory at the project root for its own
conventions. The simplest way to share the canonical `.agents/` layout with
Claude Code is a symlink: `.claude → .agents`. This maps Claude Code's
expected paths onto the harness-neutral `.agents/` tree without duplicating
anything.

## Step 1: Place the skill

```bash
mkdir -p .agents/skills
cp -r <path-to-framework>/framework-expert .agents/skills/
```

## Step 2: Create the `.claude` symlink

```bash
ln -s .agents .claude
```

This gives Claude Code access to:

| Claude Code path | Resolves to |
|------------------|-------------|
| `.claude/skills/framework-expert/` | `.agents/skills/framework-expert/` |
| `.claude/rules/` | `.agents/rules/` |
| `.claude/commands/` | `.agents/workflows/` (if present) |

> **Note:** Claude Code auto-loads all files in `.claude/rules/` into the
> agent's context at session start. Via the symlink, this means everything in
> `.agents/rules/` is auto-loaded automatically — no active-read mandate
> required in the bootstrap file.

## Step 3: Create `.agents/rules/`

Rules are operational constraints. Place any project-specific rules here:

```bash
mkdir -p .agents/rules
```

A minimal rule set for a framework consumer app:

```
.agents/rules/
├── 01-plan-before-execute.md   ← copy from framework repo
└── 02-tmp_workspace.md         ← copy from framework repo
```

You can copy these from the framework repo's `.agents/rules/` directory.

## Step 4: Create `AGENTS.md` at your project root

`AGENTS.md` is the canonical cross-harness bootstrap. Claude Code reads this
natively when present at the project root. Minimum content:

```markdown
# <Your Project> — Agent Bootstrap

## Skills

The `framework-expert` skill is installed at `.agents/skills/framework-expert/`
(available as `.claude/skills/framework-expert/` via the symlink).

Read the skill's `SKILL.md` whenever working on ESP32 App Foundation tasks.
```

## Step 5: Create `CLAUDE.md` to import `AGENTS.md`

`CLAUDE.md` is Claude Code's primary bootstrap file. Use it to import the
shared `AGENTS.md` and add any Claude Code-specific notes:

```markdown
# <Your Project> — Claude Code

@AGENTS.md

## Notes for Claude Code

- Skills live at `.claude/skills/` (→ `.agents/skills/` via symlink).
- Rules in `.claude/rules/` (→ `.agents/rules/`) are auto-loaded at session start.
```

## Step 6: Add `.claude` to `.gitignore`

The `.claude` symlink is a local developer convenience. Add it to `.gitignore`
if you don't want to commit it (or commit it if your whole team uses Claude Code):

```bash
echo ".claude" >> .gitignore
```

## Verification

Open a new Claude Code session in your project directory. The agent should:
1. Load `.claude/rules/` files automatically (confirmed via `AGENTS.md` being read)
2. Have access to the `framework-expert` skill at `.claude/skills/framework-expert/`
