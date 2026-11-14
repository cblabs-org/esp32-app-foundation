# AI-Native Development

> ESP32 App Foundation (c) 2026 CB Labs
>
> How the framework is built with AI coding agents, and what the agent configuration, planning records and memory files in this repository are for.

---

The ESP32 App Foundation is developed with AI coding agents working under human
direction. The configuration that steers those agents, the plans they worked
from and the notes they keep are all committed alongside the code, on purpose:
they are part of how the project is maintained, and they are useful to anyone
who builds on it with an agent of their own.

You do not need any of this to use the framework. Everything here is optional
for a human developer, and the firmware builds without it.

**Prerequisites:** none. For the framework itself, start with the
[Architecture Overview](architecture-overview.md).

---

## 1. What lives where

| Path | What it is | Who reads it |
|---|---|---|
| `AGENTS.md` | Harness-neutral bootstrap: tells any agent to read the rules first | Every agent session |
| `CLAUDE.md`, `GEMINI.md` | Harness-specific notes for Claude Code and Antigravity, importing `AGENTS.md` | That harness |
| `.claude` → `.agents` | Symlink, so Claude Code finds the shared configuration | Claude Code |
| `.agents/rules/` | Operating rules, read in filename order at the start of every session | Every agent session |
| `.agents/skills/` | Skills: packaged knowledge an agent loads when a task needs it | Agents, on demand |
| `.agents/hooks.json`, `.agents/scripts/` | Tool-call hooks that enforce some rules mechanically (for example the flash tag gate) | The harness |
| `.agents/plugins/principal-architect-workspace/` | Planning records for each piece of work: `PROJECT.md` and work orders | Agents planning or reviewing work; humans auditing history |
| `.agents/context_archives/` | Distilled summaries of long sessions | Agents resuming related work |
| `.memory/` | The repository's long-term memory: per-area dossiers of facts and decisions | Agents, before working in an area |

---

## 2. Rules

The files in `.agents/rules/` are constraints every agent follows, whatever the
task:

| Rule | What it enforces |
|---|---|
| `00-operating-model.md` | Who may authorise what in the tiered model (section 4) |
| `01-plan-before-execute.md` | No code changes until a human approves the plan; never build on someone else's uncommitted work |
| `02-tmp_workspace.md` | Scratch files go in the repo's ignored `tmp/` folder |
| `03-versioning.md` | Tag before flashing; agents may create `dev` tags only; beta and stable tags and releases are human-only |
| `04-mcp-tooling.md` | Builds, tests and uploads go through the PlatformIO MCP server and its hardware lock |
| `github-reads.md` | With the maintainers' bug-fixer plugin, GitHub issues are read through a sanitising script |
| `bug-fixer-AGENTS.md` | Safety rules for the autonomous bug-fixing loop (never merge, never edit the spec, one issue per PR) |

The rules are written for agents but read well as a description of how the
project is run.

To give your own agent the PlatformIO MCP server the rules expect, copy
`.mcp.example.json` to `.mcp.json` (it is gitignored).

---

## 3. Skills

Skills are folders with a `SKILL.md` that an agent loads when a task matches
its description.

| Skill | Purpose |
|---|---|
| `framework-expert` | The framework's contracts, systems, patterns, security model and review checklists, organised for agents. It is the agent-facing counterpart to this developer guide. |
| `framework-setup` | Turns an empty folder into a consumer project with the framework as a submodule and the skills linked in |
| `framework-portal-design` | Design tokens, components and UI kit for the web portal |
| `lvgl-pro-expert`, `squareline-studio-expert` | Generating LVGL Pro and SquareLine Studio UI projects |
| `embedded-systems` | General embedded-systems guidance (a git submodule) |

Consumer projects normally link `framework-expert` and
`framework-portal-design` from the framework submodule, so the skills always
match the framework version they build against. The `framework-setup` skill
does this for you; see the [README](../../README.md#c-let-an-ai-coding-agent-set-it-up).

When a change alters a public interface, the matching page under
`.agents/skills/framework-expert/interfaces/` is updated in the same pull
request. Drift between the skill and the code is treated as a bug.

---

## 4. The tiered operating model

Larger pieces of work run in three tiers:

1. **Chief Architect (tier 1)** and the human owner agree the direction for a
   programme of work.
2. A **Principal Architect (tier 2)** breaks a pull request into work orders,
   each with a strict boundary of files and behaviour, and records the plan in
   `PROJECT.md`. The human approves that decomposition explicitly, and the
   approval is recorded in the project's status log.
3. **Workers (tier 3)** implement one work order each, inside its boundary,
   and report back. The Principal Architect verifies, integrates and opens the
   pull request; the human merges.

The approval gate sits between the human and the Principal Architect. A worker
checks the committed record of approval rather than trusting a claim relayed by
another agent, which keeps the chain auditable.

The planning tool itself is a plugin on the maintainer's machine and is not
part of this repository. Its output is: each folder in
`.agents/plugins/principal-architect-workspace/` is one project, with its
`PROJECT.md` (objective, architecture context, decomposition, status log) and
one subfolder per work order. Read them to see why something was built the way
it was.

---

## 5. Repository memory

`.memory/` holds what agents have learned about the codebase between sessions:

- `dossiers/` — one file per area of the code (`hardware/`, `lib/DeviceCrypto/`
  and so on) with facts, decisions and known pitfalls
- `ATLAS.md` — the index of dossiers
- `QUESTIONS.md` — open questions waiting for a human answer
- `ops/` — dated logs of memory maintenance

Agents read the relevant dossier before working in an area and add to it when
they learn something the code and docs don't already say. Dossiers can lag
behind the code; when they disagree, the code and the
[documentation authority map](../README.md) win.

---

## 6. Using agents on your own app

For a consumer application you need very little of this:

1. Run the `framework-setup` skill, or link `framework-expert` and
   `framework-portal-design` from the framework submodule into your own
   `.agents/skills/` by hand.
2. Add an `AGENTS.md` that tells your agent to read your rules and use those
   skills.
3. Copy the rules you want. `01-plan-before-execute.md` and
   `02-tmp_workspace.md` are a good minimum.

Ask `framework-expert` to help write a design spec for your device, scaffold a
module, or review your code against the framework's contracts with its
compliance and release audit guides.
