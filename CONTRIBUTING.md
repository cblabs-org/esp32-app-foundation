# Contributing to the ESP32 App Foundation

Thank you for your interest in contributing! This document provides an overview
of how to contribute to the framework.

> **📖 Looking for the full developer documentation?** The comprehensive guides —
> architecture, module lifecycle, configuration, web portal, security, OTA and more —
> live in **[`docs/developer/`](docs/developer/README.md)**, which opens with a
> recommended reading order. Start there for anything beyond the quick links below.

## Quick Links

| What you want to do | Where to go |
|---|---|
| **Report a bug** or framework defect | [Reporting Framework Issues](docs/developer/issue-management.md) |
| **Contribute a fix** or upstream a change | [Contribution Protocol](.agents/skills/framework-expert/references/contribution-protocol.md) |
| **Propose a new capability** or enhancement | [Contribution Protocol](.agents/skills/framework-expert/references/contribution-protocol.md) § Enhancement / Proposal Process |
| **Write a new module** (authoring guide) | [Creating a Module](docs/developer/creating-a-module.md) — plus [Coding Patterns](docs/developer/coding-patterns.md) |
| **Understand the architecture** | [Architecture Overview](docs/developer/architecture-overview.md) — then the [full developer guide](docs/developer/README.md) |

## Module Authoring

For detailed guidance on building modules that integrate with the framework
(interfaces, configuration, web portal, display, RTOS considerations), see
[Creating a Module](docs/developer/creating-a-module.md) and
[Coding Patterns](docs/developer/coding-patterns.md) in the developer guide.

## Agent Harness Configuration

This repository uses a shared agent configuration in `.agents/`:

- **`AGENTS.md`** — Canonical, harness-neutral bootstrap (read by all harnesses)
- **`CLAUDE.md`** — Claude Code stub (imports `AGENTS.md`)
- **`GEMINI.md`** — Antigravity overrides and rules-load backstop
- **`.agents/rules/`** — Unconditional operational constraints

See each file for details on agent configuration.

