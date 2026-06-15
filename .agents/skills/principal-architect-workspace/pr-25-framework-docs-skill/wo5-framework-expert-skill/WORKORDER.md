---
name: pr-25-wo5
description: Handoff instructions for Work Order 5 — Framework Expert AI skill
complexity: standard
---

# WO5 — Framework Expert Skill

You are an expert senior software engineer and execution-focused worker agent.
You are NOT the Principal Architect. Do not make high-level architectural
decisions, do not branch, and do not plan. Your task is strictly limited to
executing the checklist below.

---

## Step 0 — Mandatory Rules Load (DO THIS FIRST)

**Before doing anything else**, read every file inside `.agents/rules/` in
filename order using file-read tools. These are unconditional operational
constraints. Do not skip this step. Do not assume they were auto-loaded.

---

## Prerequisite Context

WO4 has already landed. The developer docs exist at `docs/developer/`. Before
writing the skill, read ALL of these documentation files:

- `docs/developer/architecture-overview.md`
- `docs/developer/creating-a-module.md`
- `docs/developer/config-system.md`
- `docs/developer/extending-the-web-portal.md`
- `docs/developer/event-bus.md`

Also read an existing skill for structure reference:
- `.agents/skills/embedded-systems/SKILL.md` — example of a well-structured skill

And understand how skills work in this project:
- Skills are directories under `.agents/skills/`
- Each has a `SKILL.md` with YAML frontmatter (`name`, `description`)
- Complex skills have a `references/` subdirectory with detailed reference docs
- The `SKILL.md` is the routing layer — it should be concise (≤200 lines) and
  point agents to the right reference file based on the query
- Reference files are detailed but token-efficient — they are summaries/distillations
  of the full docs, NOT copies of the docs

---

## Objective

Create a self-contained AI skill at `.agents/skills/framework-expert/` that
enables any AI agent to answer developer questions about extending the framework.
The skill uses progressive disclosure: a lightweight routing `SKILL.md` points to
detailed `references/` files based on the query topic.

---

## Strict Boundary Box

- **DO** create: all files under `.agents/skills/framework-expert/`
- **DO NOT** modify any source code files (`.hpp`, `.cpp`, `.h`, `.js`)
- **DO NOT** modify any files in `docs/`, `modules/`, `template/`, or `CONTRIBUTING.md`
- **DO NOT** modify any existing skills in `.agents/skills/`
- **DO NOT** create branches or make git commits — the PA handles branching.

---

## Line-Item Checklist

### Part A — Create `SKILL.md`

- [ ] Create `.agents/skills/framework-expert/SKILL.md`
- [ ] The file MUST begin with YAML frontmatter:
  ```yaml
  ---
  name: framework-expert
  description: >
    Answers questions about building applications on the ESP32 App Foundation.
    Trigger on: "how do I create a module", "register a module", "registerModule",
    "ITickable", "IConfigurable", "IWebContributor", "IMCPContributor",
    "IConfigGate", "IDataSource", "EventBus", "ConfigManager", "web portal tab",
    "config system", "extension point", "framework architecture", "AppFoundation.h",
    "tick function", "boot sequence", "LittleFS layout", "FrameworkConfig".
    Also trigger when an agent is working on code that extends the framework
    and needs guidance on interface contracts or registration patterns.
  ---
  ```

- [ ] Below the frontmatter, write the skill body. It must contain:

  **1. Overview section** (5-10 lines): What the framework is, the single include
  (`AppFoundation.h`), and the primary registration API (`registerModule()`).

  **2. Quick answers** for the most common questions (inline, not in references):
  - "How do I register a module?" → `app.registerModule(&myModule);`
  - "What interfaces are available?" → table of 8 interfaces with one-line descriptions
  - "What include do I use?" → `#include "AppFoundation.h"`

  **3. Reference routing table** — directs the agent to the right reference file:
  ```markdown
  ## Detailed References

  | Topic | Read |
  |---|---|
  | Framework architecture, boot lifecycle, 3-tier model | `references/architecture.md` |
  | Creating a new module, interface selection, registration | `references/creating-a-module.md` |
  | IConfigurable, schema versioning, config persistence | `references/config-system.md` |
  | IWebContributor, REST routes, web portal JS fragments | `references/web-portal.md` |
  | EventBus, topics, subscribe/publish patterns | `references/event-bus.md` |
  ```

  **4. "When to read the full docs"** — point agents to `docs/developer/` for
  complete tutorials when the references aren't sufficient.

- [ ] The SKILL.md must be ≤200 lines. It is a routing layer, not a reference.

### Part B — Create Reference Files

Each reference file is a **distillation** of the corresponding developer doc.
It is NOT a copy. It should be 50-70% the length of the source doc, focused on
the facts an AI agent needs to generate correct code, with zero filler prose.

- [ ] Create `.agents/skills/framework-expert/references/architecture.md`
  - Source: `docs/developer/architecture-overview.md`
  - Must include: 3-tier model, `AppFoundation.h` facade, `registerModule()` API,
    interface table, boot lifecycle phases, LittleFS layout
  - Omit: tutorial prose, extended explanations, motivation paragraphs

- [ ] Create `.agents/skills/framework-expert/references/creating-a-module.md`
  - Source: `docs/developer/creating-a-module.md`
  - Must include: interface selection guide, class declaration pattern,
    `registerModule()` usage, `tick(uint32_t currentMillis)` signature and
    contract, module checklist, common mistakes
  - Omit: full walkthrough narrative, step-by-step tutorial flow

- [ ] Create `.agents/skills/framework-expert/references/config-system.md`
  - Source: `docs/developer/config-system.md`
  - Must include: the five IConfigurable methods with exact signatures, config
    file path pattern, schema versioning contract, ArduinoJson patterns,
    FrameworkConfig clarification, `saveModuleConfig()` API
  - Omit: extended ArduinoJson tutorial, migration worked examples

- [ ] Create `.agents/skills/framework-expert/references/web-portal.md`
  - Source: `docs/developer/extending-the-web-portal.md`
  - Must include: IWebContributor method signatures, route registration pattern,
    tab fragment structure, JS `fetch()` pattern, `/api/tabs` contract
  - Omit: full HTML/JS templates (point to template/ directory instead)

- [ ] Create `.agents/skills/framework-expert/references/event-bus.md`
  - Source: `docs/developer/event-bus.md`
  - Must include: API signatures, module-owned topic pattern, payload lifetime
    rule, framework-published topics table, custom topic declaration pattern
  - Omit: extended examples, common patterns section

### Part C — Create Keeping-Current Guide

- [ ] Create `.agents/skills/framework-expert/references/KEEPING-CURRENT.md`
  - This file tells future agents how to update the skill when the framework changes.
  - Content:
    ```markdown
    # Keeping This Skill Current

    ## When to Update

    Update this skill's references when:
    - A new interface is added to `include/interfaces/`
    - An existing interface signature changes
    - The registration API (`registerModule()`) changes
    - New EventBus topics are added by framework modules
    - The boot lifecycle phases change
    - The web portal SPA shell contract changes

    ## How to Update

    1. Read the changed source files
    2. Update the corresponding `references/*.md` file
    3. If a new topic area was added, create a new reference and add it
       to the routing table in `SKILL.md`
    4. Update the `skill_manifest.json` timestamp

    ## Source of Truth

    - Interface contracts: `include/interfaces/*.hpp`
    - Registration API: `modules/Application/Application.hpp`
    - Boot sequence: `modules/Application/Application.cpp` → `begin()`
    - Full developer docs: `docs/developer/`
    ```

### Part D — Create Skill Manifest

- [ ] Create `.agents/skills/framework-expert/skill_manifest.json`:
  ```json
  {
    "name": "framework-expert",
    "version": "1.0.0",
    "created": "2026-06-15",
    "lastUpdated": "2026-06-15",
    "sourceDocsVersion": "PR-25",
    "references": [
      "references/architecture.md",
      "references/creating-a-module.md",
      "references/config-system.md",
      "references/web-portal.md",
      "references/event-bus.md",
      "references/KEEPING-CURRENT.md"
    ]
  }
  ```

---

## Verification Requirement

After completing all checklist items, verify:
1. All 9 files exist:
   - `.agents/skills/framework-expert/SKILL.md`
   - `.agents/skills/framework-expert/skill_manifest.json`
   - 5 reference files in `references/`
   - `references/KEEPING-CURRENT.md`
2. `SKILL.md` is ≤200 lines
3. All reference files are distillations (not copies) of the source docs
4. The routing table in `SKILL.md` points to all reference files
5. No source code or docs were modified

**STOP** after verification and send a message back to the Principal Architect
reporting your results. Do not distill or commit until the PA approves.
