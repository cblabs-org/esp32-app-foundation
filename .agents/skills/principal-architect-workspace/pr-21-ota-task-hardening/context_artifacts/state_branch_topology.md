# State — PR-21 Branch Topology & Commit Map

## Final Branch Map

```
main (5f15490) ← merged PR-21
│
└── pr-21-ota-task-hardening (feature branch)
     ├── pr-21-ota-task-hardening-wo1-heap-guard
     │    ├── 175dfc8  feat(ota): heap guard + PSRAM stack allocation
     │    └── a58d046  docs(pr-21-wo1): distill session + WORKORDER complete
     │
     └── pr-21-ota-task-hardening-wo2-portal-warning
          └── 4d8aff6  feat(portal): low-memory OTA advisory warning banner
```

## Commits on main (PR-21 scope) above prior HEAD `df510d3`

| SHA | Type | Description |
|-----|------|-------------|
| `5f15490` | merge | PR-21 feature branch → main (PA close) |
| `36dbd13` | docs | PROJECT.md + MEMORY.md closure |
| `98456d5` | merge | WO2 → feature branch |
| `4d8aff6` | feat | portal: low-memory OTA advisory warning banner |
| `6686919` | merge | WO1 → feature branch |
| `a58d046` | docs | WO1 distillation + WORKORDER complete |
| `175dfc8` | feat | OTA heap guard + PSRAM stack allocation |
| `a0c91dc` | fix | WiFi wizard modal wiring + %zu printf format |
| `dff3b22` | fix | crypto: return 0 on invalid PKCS7 padding byte |
| `f75862a` | chore | agents: add Security section to todo taxonomy |

## Files Modified

### WO1 — Firmware
- `modules/OTAUpdateManager/OTAUpdateManager.hpp` — 2 new constexpr constants
- `modules/OTAUpdateManager/OTAUpdateManager.cpp` — xTaskCreateWithCaps, pre-flight guard, 2 new status fields

### WO2 — Portal
- `web/portal/ota.js` — warning div in Card 3 HTML scaffold + loadStatus() rendering logic

### Bug Fixes (other agent)
- `lib/DeviceCrypto/DeviceCrypto.cpp` — PKCS7 padding return value fix
- `modules/Application/Application.cpp` / `.hpp` — WiFi wizard modal wiring + %zu format

### PA Workspace
- `.agents/skills/principal-architect-workspace/MEMORY.md` — PR-21 registered + marked complete
- `.agents/skills/principal-architect-workspace/pr-21-ota-task-hardening/PROJECT.md` — full project ledger
- `wo1-firmware-heap-guard/` — WORKORDER + distillation artifacts
- `wo2-portal-memory-warning/` — WORKORDER + distillation artifacts
- `pr-22-portal-ui-consistency/` — pre-staged PROJECT.md by WO2 worker (included in merge)
