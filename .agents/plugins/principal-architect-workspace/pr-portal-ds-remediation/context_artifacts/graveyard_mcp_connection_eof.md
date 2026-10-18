# Graveyard: PlatformIO MCP Server EOF Failures

## Symptom

Calls to PlatformIO MCP tools (`build_project`, `upload_firmware`, etc.) fail with:

```
connection closed: calling "tools/call": client is closing: EOF
```

This occurs intermittently, seemingly correlated with long-running sessions or after the MCP server has been idle.

## Workaround

Fall back to direct CLI invocation:

```bash
# Instead of MCP build_project:
pio run -e crowpanel-128-rotary 2>&1 | tail -30

# Instead of MCP upload_firmware:
pio run -e crowpanel-128-rotary --target upload 2>&1 | tail -15
```

## Notes

- The MCP server may need a restart between long sessions
- The `background: true` parameter on MCP tools does not help — the connection drops before the response
- CLI fallback is reliable and produces identical results
- When using CLI with `tail`, the log pipe may appear empty during execution — this is normal; check task status or wait for completion notification
