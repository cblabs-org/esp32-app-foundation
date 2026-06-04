# ADR: Config Schema v3 — Independent Screensaver Timeouts

## Decision

The display config namespace was upgraded from schema v2 (single `sleepTimeout`) to schema v3 with three independent keys: `screensaverEnabled` (bool), `dimTimeout` (ms), `blankTimeout` (ms).

## Migration Strategy

`applyConfig()` uses a waterfall approach:
1. If `dimTimeout` key exists → new-style config, use `dimTimeout` + `blankTimeout` directly
2. Else if `sleepTimeout` key exists → legacy migration: `dimTimeout = sleepTimeout / 2`, `blankTimeout = sleepTimeout`
3. Else → factory defaults: `dimTimeout = 150000`, `blankTimeout = 300000`

`serializeConfig()` always writes the new keys and never emits `sleepTimeout`, ensuring one-way migration.

## Defaults

| Key | Default | Rationale |
|---|---|---|
| `screensaverEnabled` | `true` | Preserves pre-existing behaviour |
| `dimTimeout` | `150000` (2.5 min) | Was previously `sleepTimeout / 2` with default `sleepTimeout = 300000` |
| `blankTimeout` | `300000` (5 min) | Was previously `sleepTimeout` default |

## REST API

`POST /api/display/sleep` accepts optional `enabled`, `dimTimeout`, `blankTimeout` fields. Each is independently settable. A legacy `timeout` field is also accepted for backward compatibility (derives dim = timeout/2, blank = timeout).
