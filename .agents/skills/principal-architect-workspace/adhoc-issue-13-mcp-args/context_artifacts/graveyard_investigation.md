# Graveyard — Issue #13 Investigation Approach

## What We Tried / Encountered

### 1. Direct GitHub CLI access in sandboxed shell
**Attempted:** `gh issue view 13` in the standard sandbox shell.  
**Result:** Failed — the sandbox has no network access by default, so `gh` CLI network calls error out.  
**Workaround:** Used `BypassSandbox: true` on `run_command` to execute `gh` commands with full macOS network access. This is the required pattern for any `gh` CLI invocation in this workspace.

### 2. Confirming the bug was still present post-issue #11/#12
**Concern:** The recent `4317bb5` commit (fixes #11/#12) might have inadvertently fixed #13.  
**Finding:** `4317bb5` only touched `U8G2DisplayModule.cpp` and partition CSVs — `MCPServer.cpp:294` was untouched. Bug confirmed present on `HEAD`.

### 3. Test gap — why the bug went undetected
The 674-line `test_mcpServer.cpp` had no test that asserted argument pass-through. Every `tools/call` test either:
- Called a no-argument tool, or  
- Registered a handler that ignored `params`

An empty-arguments object is indistinguishable from a legitimately no-argument tool call. The bug could only manifest in a real device scenario where a handler reads `params["key"]` and gets a null/missing value instead of the expected value.

### 4. Considered: changing `_handleToolsCall` signature to non-const
**Why rejected:** Semantically dishonest — the function is purely a reader of `req`. The `JsonObjectConst` API is the correct ArduinoJson 7 pattern. See `adr_const_variant_fix.md`.

### 5. Stashed PA skill change during branch work
When creating the fix branch, the working tree had a dirty `.agents/skills/principal-architect/SKILL.md` (stacked-WO model update from a prior session sync that hadn't been committed). This was stashed before branching (`git stash push -m "pa-skill-sync: ..."`) and popped after the PR branch was pushed. It was then committed separately as `b37d945` after PR #14 merged.

**Key takeaway:** Always check `git status --short` before branching — a dirty `.agents/` file carried an unrelated change that could have confused the PR diff.

## What Was NOT a Problem
- `argsDoc.set(JsonVariantConst)` — correctly deep-copies the const variant to the mutable argsDoc. No change needed here.
- All other `is<JsonObject>()` call sites in the codebase — confirmed operating on non-const sources.
- ArduinoJson version — v7.4.3, the current pinned version, has this asymmetry by design. Not a library bug; a usage pattern issue.
