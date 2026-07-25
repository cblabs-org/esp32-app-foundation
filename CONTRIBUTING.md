# Contributing to the ESP32 App Foundation

Thank you for your interest in contributing! This document provides an overview
of how to contribute to the framework.

## Quick Links

| What you want to do | Where to go |
|---|---|
| **Report a bug** or framework defect | [Issue Management Protocol](.agents/skills/framework-expert/references/issue-management.md) |
| **Contribute a fix** or upstream a change | [Contribution Protocol](.agents/skills/framework-expert/references/contribution-protocol.md) |
| **Propose a new capability** or enhancement | [Contribution Protocol](.agents/skills/framework-expert/references/contribution-protocol.md) § Enhancement / Proposal Process |
| **Write a new module** (authoring guide) | [Module Developer Guide](.agents/skills/framework-expert/guides/module-developer-guide.md) |
| **Understand the architecture** | [Architecture Overview](.agents/skills/framework-expert/references/architecture.md) |

## Module Authoring

For detailed guidance on building modules that integrate with the framework
(interfaces, configuration, web portal, display, RTOS considerations), see the
[Module Developer Guide](.agents/skills/framework-expert/guides/module-developer-guide.md).

## Agent Harness Configuration

This repository uses a shared agent configuration in `.agents/`:

- **`AGENTS.md`** — Canonical, harness-neutral bootstrap (read by all harnesses)
- **`CLAUDE.md`** — Claude Code stub (imports `AGENTS.md`)
- **`GEMINI.md`** — Antigravity overrides and rules-load backstop
- **`.agents/rules/`** — Unconditional operational constraints

See each file for details on agent configuration.

