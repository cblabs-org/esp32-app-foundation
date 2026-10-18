# Graveyard: Antigravity Rules Engine Bug

## Summary
During the implementation of the App tab, the agent failed to respect a global rule defined at `~/.gemini/config/rules/01- plan-before-execute.md` which instructed the agent to never auto-implement without an approved plan.

## Investigation
- **Global Rules (`~/.gemini/config/rules/`)** and **Workspace Rules (`.agents/rules/`)** are officially documented features of the Antigravity framework.
- They support activation modes like "Always On", "Model Decision", etc.
- **Dead-end:** The rules engine in the current version of Antigravity is **completely non-functional**. 
- The framework fails to read or inject the rules files into the agent's system prompt context. 
- The bug was verified to occur regardless of the underlying LLM selected (tested on both Claude Opus 4.6 Thinking and Gemini 3.1 Pro High). Neither model receives a `<rules>` context block.

## Resolution
A critical bug report was written and saved as an artifact (`bug_report_rules_not_loaded.md`). The user should send this report to the Antigravity developers. Until this is fixed, agents will have total amnesia regarding any rules defined in the `rules` directories. A temporary workaround would be using `SKILL.md` files with broad triggers to enforce behaviours, as skills *are* correctly injected.
