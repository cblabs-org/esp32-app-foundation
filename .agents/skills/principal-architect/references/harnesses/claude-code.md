---
name: harness-claude-code
description: Worker-spawning adapter for the Claude Code harness (Anthropic CLI / Claude Agent SDK). Selected when your spawn tool can override the worker model per spawn.
---

# Harness Adapter — Claude Code

You are running under **Claude Code** (Anthropic's CLI / Claude Agent SDK). You confirmed this by self-inspection (Core Directive 6): your sub-agent spawn tool exposes a per-spawn **`model`** parameter. This adapter governs how you spawn and right-size workers.

## Spawn Mechanics

Spawn workers with the **`Agent`** tool:
- `subagent_type: general-purpose` for execution workers.
- `model:` — **always set this explicitly.** Never let a worker silently inherit the Architect's Opus context; that is the single most expensive mistake in this workflow.
- `run_in_background: true` for autonomous execution — the worker runs unattended and auto-wakes you on completion (the default for well-specified Work Orders).
- `prompt:` — the bridge prompt from Step 4 of the skill.

### Autonomous vs. interactive (choose per Work Order)
There are two spawn patterns; pick per WO rather than committing to one default:
- **Autonomous `Agent` worker** — `run_in_background: true`, reports back only to you (the Architect), auto-wakes you. Best for mechanical, well-specified orders.
- **Interactive `spawn_task` worker** — surfaces a chip that opens a separate session + its own worktree for the user to drive directly through a few iterations. Best when hands-on iteration is valuable. Does **not** auto-report back to you.

If the WO benefits from human-in-the-loop iteration, briefly surface this choice to the user before spawning.

## Complexity → Model Mapping (The Efficiency Lever)

You, the Architect, run on **Opus** because you hold the macro context and make the judgment calls. Workers execute a strict, pre-specified checklist, so they should run on the **cheapest model that does the job without quality loss**. Model tier is *coupled to work-order specificity* — the tighter your `WORKORDER.md`, the cheaper the model that can execute it. This is the primary cost/latency lever in the whole workflow.

Map the WORKORDER's `complexity` field directly to the `model` parameter:

| `complexity` | `model` | Rationale |
|---|---|---|
| `mechanical` | `haiku` | Rote, deterministic work — fastest, cheapest tier. |
| `standard`   | `sonnet` | **Default worker tier.** Strong at code, fast, far cheaper than Opus. |
| `architect`  | `opus` | Reserve for WOs that genuinely need architect-grade reasoning. If you reach for this, first ask whether the ambiguity belongs back in *your* court before handoff. |

The `model` you pass MUST match the `complexity` recorded in the WORKORDER frontmatter, so the choice stays auditable in the durable artifact.
