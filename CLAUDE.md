# ESP32 App Foundation — Claude Code

@AGENTS.md

The canonical agent bootstrap is imported above from `AGENTS.md` (shared across
harnesses). This file holds only Claude Code-specific notes.

## Notes for Claude Code

- Project config resolves through the `.claude → .agents` symlink: skills live at
  `.claude/skills/`, slash commands at `.claude/commands/` (→ `.agents/workflows/`),
  and `.claude/rules/` auto-loads `.agents/rules/*.md` into context. Even so, the
  `AGENTS.md` mandate to actively read `.agents/rules/` first still applies.
- `/todo` conversation lookups: use the Claude Code adapter at
  `.agents/harness-adapters/todo/claude-code.md` (session-search MCP tools).
