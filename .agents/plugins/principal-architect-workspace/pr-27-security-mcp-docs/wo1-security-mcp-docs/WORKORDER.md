---
name: pr-27-wo1
description: Create security.md and mcp-server.md human developer docs, update README.md index
complexity: standard
---

# Work Order: Security & MCP Server Human Documentation

You are an expert senior software engineer and technical writer. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

## Objective

Create two new human developer documentation guides and update the docs index.

## Strict Boundary Box

- **DO** create/modify files ONLY in `docs/developer/`
- **DO NOT** modify any source code, skill files, or agent workspace files
- **DO NOT** create branches or make git commits
- **DO** verify all API signatures against actual header files in `include/interfaces/` and module headers before writing them

## Pre-Flight

1. Read `.agents/rules/` files in order (mandatory)
2. Read the existing docs template style by viewing the first 40 lines of `docs/developer/embedded-safety.md` — match its format exactly (copyright header, blockquote intro, horizontal rules, section headings, code blocks)

## Checklist

### Task 1: Create `docs/developer/security.md`

Source material — read ALL of these skill reference files:
- `.agents/skills/framework-expert/security/overview.md`
- `.agents/skills/framework-expert/security/tls-tradeoffs.md`
- `.agents/skills/framework-expert/security/credential-storage.md`
- `.agents/skills/framework-expert/security/ota-security.md`
- `.agents/skills/framework-expert/security/web-portal-auth.md`

Also verify against actual source:
- `include/FrameworkConfig.hpp` — for OTA crypto config fields
- `modules/AuthManager/AuthManager.hpp` — for auth API
- `modules/KeyManager/KeyManager.hpp` — for credential storage API (public methods only — check `addKey`, `getKeyById`, `getKeyCategoryIds`)
- `lib/DeviceCrypto/DeviceCrypto.hpp` — for encryption API
- `include/interfaces/IOTAVerifier.hpp` — for verifier interface

Structure the document as follows:
1. **Header** — match the exact format of existing docs (copyright, blockquote description)
2. **Threat Model** — from overview.md (local network, resource-constrained)
3. **Security Layers** — table from overview.md
4. **Hardware Encryption (DeviceCrypto)** — from credential-storage.md
5. **Credential Storage (KeyManager)** — from credential-storage.md, with API examples
6. **Transport Security (TLS)** — from tls-tradeoffs.md, including memory costs, cipher selection, `setInsecure()` policy
7. **Web Portal & MCP Authentication** — from web-portal-auth.md, session cookies, bearer tokens
8. **OTA Firmware Security** — from ota-security.md, signature verification, anti-rollback, SECURITY_VERSION
9. **The Central Tension** — RAM cost reality from overview.md

### Task 2: Create `docs/developer/mcp-server.md`

Source material — read these:
- `.agents/skills/framework-expert/interfaces/mcp-contributor.md`
- `modules/MCPServer/MCPServer.hpp` — for server architecture
- `modules/MCPServer/MCPToolRegistry.hpp` — for tool registration API
- `modules/MCPServer/MCPBuiltinTools.cpp` — for built-in tool listing (first 100 lines for the registration calls)

Structure the document as follows:
1. **Header** — match existing doc format
2. **Overview** — what the MCP server is, JSON-RPC over HTTP
3. **Architecture** — MCPServer, MCPToolRegistry, MCPBuiltinTools
4. **Implementing `IMCPContributor`** — the interface contract, `registerMCPTools()` callback
5. **Registering Tools** — `MCPToolRegistry::registerTool()` API, handler signature
6. **Built-in Tools** — list from MCPBuiltinTools (system_info, capture_display, config read/write, etc.)
7. **HTTP Transport** — endpoint URL, JSON-RPC format, authentication (bearer token)
8. **Testing MCP Tools** — how to test with curl or an MCP client

### Task 3: Update `docs/developer/README.md`

Add the two new guides to the README index:

In the "Advanced Topics" section, after line 29 (Embedded Safety), add:
```
12. [Security](security.md) — Threat model, hardware encryption, credential storage, TLS tradeoffs, web portal authentication, and OTA firmware integrity.
13. [MCP Server](mcp-server.md) — The Model Context Protocol server, tool registration via `IMCPContributor`, built-in tools, and HTTP JSON-RPC transport.
```

Renumber the existing items (Performance becomes 10, Embedded Safety becomes 11).

Wait — actually read the current README.md first to see the current numbering before deciding exact placement. The key requirement is: Security and MCP Server must appear in the Advanced Topics section.

## Verification

1. Confirm both new files exist and have >200 lines each
2. Confirm README.md has been updated with links to both new files
3. Confirm no skill files, source code, or agent workspace files were modified
4. List all modified/created files

## Completion

When finished, STOP and send a message back to the Principal Architect. Do not wrap up or distill until explicitly approved.
