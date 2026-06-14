# ESP32 App Foundation — Antigravity Overrides

The canonical agent bootstrap lives in **`AGENTS.md`** (read natively by
Antigravity at the workspace root, and shared with Claude Code / Cursor / Gemini
CLI). **Read it first.**

This file holds only Antigravity-specific overrides. Per Antigravity's precedence
(System → GEMINI.md → AGENTS.md → rules dir), anything here wins over `AGENTS.md`.

## Mandatory rules — reliability backstop

Do not rely on Antigravity's workspace rules-directory auto-load. As your **very
first action** in any session, actively read every file in `.agents/rules/` in
filename order using file-read tools — do not assume they were auto-loaded.
These are unconditional operational constraints. `AGENTS.md` states this mandate
too; it is repeated here because `GEMINI.md` is the most reliably loaded channel.

## Antigravity-specific notes

- `/todo` conversation lookups: use the Antigravity adapter at
  `.agents/harness-adapters/todo/antigravity.md` (local `~/.gemini/antigravity/brain/`
  transcript store).
