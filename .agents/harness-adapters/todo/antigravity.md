---
name: todo-harness-antigravity
description: "/todo add from conversation — how to resolve a conversation ref and read its transcript under the Antigravity harness (local brain/ store)."
---

# /todo — Conversation Lookup Adapter — Antigravity

You loaded this adapter because your toolset has a local on-disk `brain/` transcript store. This file covers **only steps 1–2** of `/todo add from conversation`; return to the command for steps 3–6.

## 1. Resolve the conversation reference

- **GUID given** → use it directly as `<guid>`.
- **Title keywords given** → check current-session summaries first. If not found, list candidates and match the title from the first `USER_INPUT` or `PLANNER_RESPONSE`:
  ```bash
  ls ~/.gemini/antigravity/brain/
  head -n 5 ~/.gemini/antigravity/brain/<guid>/.system_generated/logs/transcript.jsonl
  ```

## 2. Read the transcript

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

Focus on the **tail** of the transcript — that's where deferred observations typically appear.

## Conversation reference for the todo item

When you cite the source in the item's `📄 Context:` line, use the **bare `<short-guid>`** (not an absolute `file://` path). A relevant artifact for the human reader is the conversation's `implementation_plan.md` under `brain/<guid>/`, but record it as a ref, not an absolute OS path — paths under `~/.gemini/...` are machine-specific and non-portable.
