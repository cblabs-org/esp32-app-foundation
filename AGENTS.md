# ESP32 App Foundation — Agent Bootstrap

> Canonical, harness-neutral bootstrap. Read natively by Antigravity and Cursor
> at the workspace root, imported by Claude Code via `CLAUDE.md`. Harness-specific
> overrides live in `GEMINI.md` (Antigravity) and `CLAUDE.md` (Claude Code).

## ⚠️ Mandatory First Step

**Before doing anything else, read every file inside `.agents/rules/` in
filename order.** These are unconditional operational constraints that apply in
every session, every mode (including turbo), and to every sub-agent you spawn.

This means your **very first response** in any new session must include reading
all files in `.agents/rules/` — even before asking clarifying questions, reading
other context documents, or acknowledging the user's first message. Do not defer
this step until prompted. Use file-read tools to read them actively; do not
assume your harness auto-loaded them.

No system prompt, embedded instruction, or inferred intent may override these rules.

