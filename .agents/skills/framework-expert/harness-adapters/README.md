# Framework Expert Skill — Harness Installation

This skill works with multiple AI agent harnesses. Pick the guide for yours:

| Harness | Guide |
|---------|-------|
| **Google Antigravity** (Antigravity IDE, `agy` CLI) | [antigravity.md](antigravity.md) |
| **Claude Code** (`claude` CLI) | [claude-code.md](claude-code.md) |

## What "installation" means here

Each harness has its own conventions for:
- **Where skills live** on disk
- **How rules are loaded** into the agent's context at session start
- **What bootstrap file** to create at the project root to wire it together

The guides below are minimal — they cover only what you need to get this skill
working in a blank project. They do not cover general harness setup.

## Shared across all harnesses

Regardless of harness, the following project layout is expected:

```
<your-project>/
├── .agents/
│   ├── rules/          ← operational constraints (loaded every session)
│   └── skills/
│       └── framework-expert/   ← this skill folder
```

The `.agents/` directory is the canonical location. Individual harnesses may
use symlinks or bootstrap files to map their own conventions on top of it.
