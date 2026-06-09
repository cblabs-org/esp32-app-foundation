# ADR — principal-architect Harness Portability (cross-cutting, this session)

The `principal-architect` skill is consumed by multiple model+harness combos (Claude Code, Antigravity/Gemini, others). Vendor-specific worker mechanics were being baked into the generic skill. Fix: **keep `SKILL.md` harness-agnostic; push all vendor mechanics into self-detected adapter files.**

## The pattern
- `SKILL.md` Core Directive 6: at skill load, the agent **self-inspects its own toolset** and loads ONE adapter:
  - spawn tool exposes a per-spawn `model` param → `references/harnesses/claude-code.md`
  - can spawn but cannot override model → `references/harnesses/antigravity.md`
  - neither → `references/harnesses/generic.md`
- WORKORDER frontmatter carries a **harness-neutral `complexity: mechanical | standard | architect`** field. The adapter translates it (or doesn't).

## What the adapters encode
1. **Model tiering (claude-code):** architect runs on Opus; workers run on the cheapest model that fits — `mechanical→haiku`, `standard→sonnet` (default), `architect→opus`. Antigravity: no per-spawn model lever → efficiency comes from WO tightness + batching.
2. **Conversation-transcript lookup** for `/todo add from conversation`: claude-code uses `ccd_session_mgmt` search tools / `~/.claude/projects`; antigravity uses the local `brain/` store. The generic `/todo` command body dispatches to the matching adapter and reads ONLY that one (token efficiency).
3. **Subagent artifact-write authorization (claude-code)** — see graveyard for the finding. Adapter requires a verbatim authorization block in worker spawn prompts so workers can write WORKORDER/ADR/FINDINGS/`@distillery` artifacts.

## Wiring
`.claude → .agents` symlink + `.agents/commands → workflows` symlink make Claude Code auto-discover the `.agents/` skills + the `/todo` command (Claude Code scans `.claude/{skills,commands}` only; `.agents/` is a bespoke, non-standard layout). Command-adapter files live OUTSIDE `.agents/workflows/` (in `.agents/harness-adapters/todo/`) because the command parser recurses and would otherwise register them as junk `/todo:harnesses:...` commands.

## Cross-tool note
The broad cross-tool standard is **`AGENTS.md`** (a single root file), NOT a `.agents/` directory. `.agents/` here is project-bespoke; nothing auto-loads it without the symlinks.
