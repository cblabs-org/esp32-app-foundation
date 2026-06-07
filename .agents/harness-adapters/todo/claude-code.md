---
name: todo-harness-claude-code
description: "/todo add from conversation — how to resolve a conversation ref and read its transcript under Claude Code (session-search tools / ~/.claude/projects store)."
---

# /todo — Conversation Lookup Adapter — Claude Code

You loaded this adapter because your toolset exposes dedicated session / transcript-search tools. This file covers **only steps 1–2** of `/todo add from conversation`; return to the command for steps 3–6.

Claude Code stores each past conversation as a JSONL transcript and exposes session-management tools to query them. Prefer the tools over raw file reads — they avoid pulling whole transcripts into context.

## 1. Resolve the conversation reference

- **GUID given** → that is the session id (`<session-id>`); use it directly.
- **Title keywords given** → resolve to a session id without reading full transcripts:
  - Use `mcp__ccd_session_mgmt__list_sessions` to enumerate recent sessions for this project and match on title/summary.
  - Or use `mcp__ccd_session_mgmt__search_session_transcripts` with the keywords to find the session whose content matches.
  - (If these tools aren't loaded, fetch their schemas first via `ToolSearch` with `select:mcp__ccd_session_mgmt__list_sessions,mcp__ccd_session_mgmt__search_session_transcripts`.)

## 2. Read the transcript

- Preferred: `mcp__ccd_session_mgmt__search_session_transcripts` scoped to `<session-id>` with the user's note as the query — this returns only the relevant excerpts instead of the whole transcript (the token-efficient path).
- Fallback (raw): the transcript lives on disk at
  ```
  ~/.claude/projects/<project-slug>/<session-id>.jsonl
  ```
  where `<project-slug>` is the absolute project path with `/` replaced by `-`
  (e.g. `-Users-mcneillm-Documents-Projects-esp32-app-foundation`).
  Read the **tail** — deferred observations typically appear there. Each line is a JSON event; the human-authored content is in `message.content` of `type: "user"` / `type: "assistant"` records.

## Conversation reference for the todo item

When you cite the source in the item's `📄 Context:` line, use the **bare `<short-session-id>`** as the ref. Do **not** embed an absolute `~/.claude/...` or `file://` path — it is machine-specific. The session id is stable and lets any future Claude Code agent re-open the conversation via the same session tools.
