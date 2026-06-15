---
title: PR-24 Web Portal + MCP Server Authentication + TLS
distilled_at: 2026-06-15T17:09:00Z
original_plan_id: standalone
artifacts:
  - context_artifacts/adr_pr24_auth.md
  - context_artifacts/graveyard_pr24_auth.md
---

## Executive Summary
Successfully implemented session-based authentication and self-signed TLS (via PsychicHttp and mbedTLS) to secure the web portal and MCP HTTP endpoints, including a proactive Auth Guard UI and dynamic connection-storm mitigation.

## Next Steps
- Verify any final portal testing results from the user.
- Await the next feature request or bug report.

## Deep Context Menu

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_pr24_auth.md` - Details on PsychicHttp migration, dual-server TLS model, auth mechanisms, and SAN cert generation.
- `context_artifacts/graveyard_pr24_auth.md` - Explains how to avoid ApiQueue deadlocks and ESP32 DRAM connection storms (`0x7780`).
