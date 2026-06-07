---
description: Workflow for managing the project backlog in .agents/todo/todo.md (todo_list branch). Supports adding items from the current session or synthesising them from another conversation, updating item status, and reviewing the list for PA handoff.
argument-hint: [add|update|review|add from conversation] "<note>"
---

# /todo — Project Backlog Workflow

Manages the canonical project backlog at `.agents/todo/todo.md` on the `todo_list` orphan branch, accessed via a permanent git worktree.

This file is **branch-isolated** on the `todo_list` orphan branch — completely independent of `main` and all feature branches. It can never be overwritten by a PR merge. Agent brain artifacts (`backlog.md` etc.) are ephemeral session views only.

---

## Setup (run once per clone)

The worktree must exist before reading or writing the todo list:

```bash
# Check if worktree exists
ls .agents/todo/todo.md 2>/dev/null || git worktree add .agents/todo todo_list
```

The worktree directory (`.agents/todo/`) is gitignored on `main` — it will never appear in a PR or be accidentally committed to a feature branch.

`.agents/todo.md` (on `main`) is a navigation stub only — **never edit it**.

---

## Invocation Modes

### Worktree Guard

Before **any** write operation (add, update), verify the worktree exists:

```bash
ls .agents/todo/todo.md 2>/dev/null || git worktree add .agents/todo todo_list
```

If this fails (e.g. remote branch missing), stop and report to the user.

---

### `/todo add "[note]"`

Add a new item distilled from the **current session**.

**Steps:**

1. Run the **Worktree Guard** above.
2. Read `.agents/todo/todo.md` to understand existing sections and avoid duplicates.
3. Scan the current conversation context for the observation, bug, or deferred work the user is referencing.
4. Identify the appropriate section (see **Section Taxonomy** below). Create a new section if none fits.
5. Draft the item using the **Item Format** below. Infer any dependencies from the conversation.
6. Present the draft to the user for confirmation — do NOT write yet.
7. On approval, append the item to `.agents/todo/todo.md` and commit from within the worktree:
```bash
cd .agents/todo
git add todo.md
git commit -m "todo: add [item title]"
git push origin todo_list
cd -
```

---

### `/todo add from conversation [ref] "[note]"`

Add an item synthesised from **another conversation**. Use when you noticed something mid-session but didn't want to interrupt the agent.

**`[ref]`** can be:
- A GUID (e.g. `34de947b`) — unambiguous, always preferred
- Title keywords (e.g. `"Monitoring ESP32 Devices"`) — fuzzy-matched

**`[note]`** is your intent/framing. It doesn't need to be precise — the agent reads the conversation for the technical detail.

**Steps:**

> [!IMPORTANT]
> **Harness adapter — read only yours.** Resolving a `[ref]` to a conversation and reading its transcript is harness-specific. Determine your harness by **self-inspection of your own toolset**, then read **only** the one matching adapter (do not read the others — that wastes context):
> | If your toolset has… | Read this adapter |
> |---|---|
> | dedicated session / transcript-search tools | `.agents/harness-adapters/todo/claude-code.md` |
> | a local on-disk `brain/` transcript store | `.agents/harness-adapters/todo/antigravity.md` |
> | neither | *(no adapter)* — ask the user to paste the relevant transcript excerpt or a link, then skip to step 3 |
>
> The adapter defines, for steps 1–2 only, how to (a) resolve the reference and (b) read the transcript. Everything below it is harness-neutral.

1. **Resolve the conversation reference** to a specific conversation, per your adapter:
   - If a GUID is given, use it directly.
   - If title keywords are given, check the current-session conversation summaries first, then fall back to your adapter's lookup.

2. **Read that conversation's transcript** per your adapter, focusing on the **tail** — deferred observations typically appear there.

3. **Synthesise** a context-rich todo item from the transcript evidence + the user's note. Include:
   - What was observed / what broke
   - Why it was deferred (what it depends on)
   - A reference to the source conversation (see **Item Format** — use a repo-relative path or the bare conversation ref, **never** an absolute `file://` path)

4. **Read `.agents/todo/todo.md`** to check for duplicates and pick the right section.

5. **Present the draft** to the user for confirmation. Do NOT write yet.

6. On approval, append the item to `.agents/todo/todo.md` and commit/push from within the worktree (see the **Worktree Guard** and `/todo add` commit block).

---

### `/todo update`

Sync status changes — mark items done, in-progress, or blocked.

**Steps:**

1. Run the **Worktree Guard** above.
2. Read `.agents/todo/todo.md`.
3. Apply the requested status changes (user specifies which items).
4. Show a diff of what will change.
5. On approval, write the updated file and commit from within the worktree:
```bash
cd .agents/todo
git add todo.md
git commit -m "todo: [description of status changes]"
git push origin todo_list
cd -
```

---

### `/todo review`

Summarise the open backlog for `/principal-architect` handoff planning.

**Steps:**

1. Read `.agents/todo/todo.md` (no write needed, worktree guard not required — but set up worktree if missing for consistency).
2. List all open items (`[ ]`) grouped by section.
3. Identify dependency chains and suggest PR groupings.
4. Output a summary table suitable for pasting into a `/principal-architect` bridge prompt.

---

## Item Format

All items written to `.agents/todo/todo.md` must follow this format:

```markdown
- [ ] **Short imperative title** — One or two sentence description of what needs doing and why.
  Optional lines (only include if relevant):
  > ⚠️ Depends on: [item title or PR label]
  > 📄 Context: conversation `<short-ref>` — one-line summary of what's there
```

> **Conversation references must be portable.** Use the bare conversation ref (e.g. a short GUID) or a repo-relative path. **Never** embed an absolute `file://` OS path — it breaks on every other machine and harness. If your harness exposes a stable in-repo or tool-addressable link, the adapter you loaded will say so.

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
.agents/todo/todo.md        ← live backlog (todo_list orphan branch, via worktree)
.agents/todo.md             ← navigation stub on main (do not edit)
```

All reads and writes go to `.agents/todo/todo.md`. Commits must be made from within the `.agents/todo/` worktree directory and pushed to `origin todo_list`.

The `todo_list` branch is an **orphan** — it shares no history with `main` or any feature branch. PRs cannot touch it. It will never be overwritten by a merge.

---

## Notes for Agents

- **Never** write to `todo/todo.md` without user confirmation of the drafted item.
- **Always** run the worktree guard before any write operation.
- **Always** commit and push to `origin todo_list` from within `.agents/todo/` after writing.
- **Always** check for duplicates before drafting a new item.
- **Never** edit `.agents/todo.md` (the stub on `main`) — it is a pointer only.
- When a conversation reference is provided, **read the transcript** — don't guess at context from the user's note alone.
- The user's note is the *intent signal*; the transcript is the *evidence*. Both are needed.
- If a conversation GUID cannot be resolved, ask the user to confirm it before proceeding.
