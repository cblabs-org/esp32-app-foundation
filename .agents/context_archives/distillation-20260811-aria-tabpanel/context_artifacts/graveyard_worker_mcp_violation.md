# Graveyard: Worker MCP Tooling Violation

## What Happened

The WO1 execution worker (flash model, `mechanical` complexity) was explicitly instructed in both:
- **Rule 04** (`.agents/rules/04-mcp-tooling.md`): "You MUST prioritize using the `platformio` MCP tools"
- **WORKORDER Tooling Constraints**: "Use the `platformio` MCP server's `build_project` tool"

The worker ignored both and used `pio run` via `run_command` instead.

## Why It Failed

The flash-tier model processes a long system prompt + WORKORDER and doesn't give sufficient weight to tooling constraint sections. It defaults to the familiar CLI command it "knows" (`pio run`) rather than the more specific MCP tool instruction.

The Tooling Constraints section in the WORKORDER template is a separate section that the worker may skim over — it's not embedded in the checklist where the worker is most focused.

## Impact

**Functional: zero** — `pio run` and `build_project` produce identical build results. The PA re-verified via MCP after the worker completed.

**Procedural:** Bypasses the MCP server's state management (locks, environment tracking). In a multi-agent scenario, this could cause conflicts.

## Mitigation for Future WORKORDERs

1. **Embed the MCP constraint as a Line-Item checkbox** (not just in Tooling Constraints):
   ```markdown
   - [ ] ⚠️ **Build verification**: Use `call_mcp_tool` with `platformio` server's `build_project` — do NOT use `pio run` CLI
   ```
2. **Use a `⚠️ CRITICAL` callout** in the Tooling Constraints section
3. Consider `define_subagent` with `enable_mcp_tools: true` and restricted shell access for mechanical WOs where MCP compliance is essential
