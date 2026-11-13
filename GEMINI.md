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

<!-- repo-memory:begin v2 — managed by repo-memory; edits inside are overwritten -->
@[memory atlas](.memory/ATLAS.md)

## Repository Memory (`repo-memory`)
- **Pushed context (`[repo-memory ...]`):** Hooks automatically inject verified invariants, gotchas, failure signatures, and ruled-out approaches when you start a task or touch anchored files. Treat them as repository notes verified against the code; do not repeat or echo them as new discoveries.
- **Pull for depth (MCP):** Use `memory_brief` (module dossier), `memory_why` (entry/file/line rationale), `memory_history` (evolution), and `memory_search` (FTS5 search) when exploring unfamiliar subsystems.
- **Distillation:** End-of-turn capture happens automatically via the `Stop` hook on milestone turns (no action required). Use `memory_note` mid-turn only when you want to record a critical discovery or dead end immediately.
<!-- repo-memory:end -->
