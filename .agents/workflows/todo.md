---
description: Workflow for managing the project backlog in .agents/todo_list.md. Supports adding items from the current session or synthesising them from another conversation, updating item status, and reviewing the list for PA handoff.
---

# /todo — Project Backlog Workflow

Manages the canonical project backlog at `.agents/todo_list.md`.

This file is repo-committed and persists across all sessions and agents. It is the single source of truth for outstanding work. Agent brain artifacts (`backlog.md` etc.) are ephemeral session views only.

---

## Invocation Modes

### `/todo add "[note]"`

Add a new item distilled from the **current session**.

**Steps:**

1. Read `.agents/todo_list.md` to understand existing sections and avoid duplicates.
2. Scan the current conversation context for the observation, bug, or deferred work the user is referencing.
3. Identify the appropriate section (see **Section Taxonomy** below). Create a new section if none fits.
4. Draft the item using the **Item Format** below. Infer any dependencies from the conversation.
5. Present the draft to the user for confirmation — do NOT write yet.
6. On approval, append the item to the correct section in `todo_list.md`.

---

### `/todo add from conversation [ref] "[note]"`

Add an item synthesised from **another conversation**. Use when you noticed something mid-session but didn't want to interrupt the agent.

**`[ref]`** can be:
- A GUID (e.g. `34de947b`) — unambiguous, always preferred
- Title keywords (e.g. `"Monitoring ESP32 Devices"`) — fuzzy-matched

**`[note]`** is your intent/framing. It doesn't need to be precise — the agent reads the conversation for the technical detail.

**Steps:**

1. **Resolve the conversation reference:**
   - If a GUID is given, use it directly.
   - If title keywords are given:
     a. Check the conversation summaries in the current session context first.
     b. If not found there, list `~/.gemini/antigravity/brain/` to get candidate GUIDs, then read each `transcript.jsonl` head to match the title from the first USER_INPUT or PLANNER_RESPONSE.
     ```bash
     ls ~/.gemini/antigravity/brain/
     head -n 5 ~/.gemini/antigravity/brain/<guid>/.system_generated/logs/transcript.jsonl
     ```

2. **Read the transcript** for context relevant to the user's note:
   ```bash
   cat ~/.gemini/antigravity/brain/<guid>/.system_generated/logs/transcript.jsonl | python3 -c "
   import sys, json
   for line in sys.stdin:
       try:
           step = json.loads(line)
           src = step.get('source','')
           t = step.get('type','')
           content = step.get('content','')
           if content and len(content) > 20 and t in ('USER_INPUT','PLANNER_RESPONSE'):
               print(f'--- [{src}] [{t}] ---')
               print(content[:2000])
               print()
       except: pass
   "
   ```
   Focus on the tail of the transcript — that's where deferred observations typically appear.

3. **Synthesise** a context-rich todo item from the transcript evidence + the user's note. Include:
   - What was observed / what broke
   - Why it was deferred (what it depends on)
   - A link to the source conversation

4. **Read `.agents/todo_list.md`** to check for duplicates and pick the right section.

5. **Present the draft** to the user for confirmation. Do NOT write yet.

6. On approval, append the item to `todo_list.md`.

---

### `/todo update`

Sync status changes — mark items done, in-progress, or blocked.

**Steps:**

1. Read `.agents/todo_list.md`.
2. Apply the requested status changes (user specifies which items).
3. Show a diff of what will change.
4. On approval, write the updated file.

---

### `/todo review`

Summarise the open backlog for `/principal-architect` handoff planning.

**Steps:**

1. Read `.agents/todo_list.md`.
2. List all open items (`[ ]`) grouped by section.
3. Identify dependency chains and suggest PR groupings.
4. Output a summary table suitable for pasting into a `/principal-architect` bridge prompt.

---

## Item Format

All items written to `todo_list.md` must follow this format:

```markdown
- [ ] **Short imperative title** — One or two sentence description of what needs doing and why.
  Optional lines (only include if relevant):
  > ⚠️ Depends on: [item title or PR label]
  > 📄 Context: conversation [`<short-guid>`](file:///path/to/brain/<guid>/implementation_plan.md) — one-line summary of what's there
```

**Status markers:**
- `[ ]` — open / not started
- `[/]` — in progress / delegated to an agent
- `[x]` — done
- `[~]` — blocked

**Title style:** imperative verb phrase. *"Add daily OTA scheduler"* not *"Daily OTA scheduler needed"*.

---

## Section Taxonomy

Use existing sections where they fit. Add new sections if genuinely needed.

| Section | Scope |
|---------|-------|
| **🔧 Build System** | PlatformIO, CMake, sdkconfig, scripts, build pipeline |
| **🏗️ Architecture** | Module coupling, FrameworkConfig, OOP patterns, abstractions |
| **🔄 OTA** | OTA update logic, scheduling, verification, release tooling |
| **🌐 Web Portal — UI Consistency** | Styling, component behaviour, state binding |
| **🗂️ Web Portal — Navigation** | Tab structure, routing, `IWebContributor` registration |
| **🚀 Release Process** | Versioning, deploy scripts, manual release steps |
| **🐛 Bugs** | Confirmed defects deferred for later investigation |
| **📱 Sample App** | The bundled example application (clock, weather, carousel, demo features) |
| **📚 Documentation & DX** | Developer docs, framework guides, progressive discovery skill |

---

## Canonical File Location

```
.agents/todo_list.md
```

This file is tracked by git. Commit it when items are added or completed — it is part of the project's working memory.

---

## Notes for Agents

- **Never** write to `todo_list.md` without user confirmation of the drafted item.
- **Always** check for duplicates before drafting a new item.
- When a conversation reference is provided, **read the transcript** — don't guess at context from the user's note alone.
- The user's note is the *intent signal*; the transcript is the *evidence*. Both are needed.
- If a conversation GUID cannot be resolved, ask the user to confirm it before proceeding.
