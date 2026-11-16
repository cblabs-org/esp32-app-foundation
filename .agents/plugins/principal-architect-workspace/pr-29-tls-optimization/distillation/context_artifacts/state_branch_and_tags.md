# State — Branch Topology, Tags, and Device

## Branch Topology

```
main
└── pr-29-tls-optimization (5415b48) ← feature branch, WO1-WO3 merged + manifest fix + WO4 setup
    ├── pr-29-tls-optimization-wo2-build-test (5e000e5) ← can be deleted
    └── pr-29-tls-optimization-wo3-docs (9366eef) ← can be deleted
```

## Key Commits on `pr-29-tls-optimization`

| Hash | Description |
|---|---|
| `268218c` | WO1: sdkconfig.defaults TLS settings (amended with ALPN fix) |
| `5e000e5` | WO2: Build/test/measure results (tagged `v0.0.10-dev.9`) |
| `9366eef` | WO3: TLS optimization reference doc for framework-expert |
| `c81d387` | Mark project COMPLETE (WO1-WO3) |
| `5f75957` | Fix versioning rules — dev tags ARE published as GitHub pre-releases |
| `e2c8914` | Fix manifest download TLS — CDN uses Let's Encrypt, not Sectigo |
| `5415b48` | Add WO4 work order + versioning rule updates (tagged `v0.0.10-dev.10`) |

## Tags

| Tag | Commit | Deployed as Release? |
|---|---|---|
| `v0.0.10-dev.9` | `5e000e5` (on wo2 branch) | ✅ Yes (user ran deploy_release.py) |
| `v0.0.10-dev.10` | `5415b48` (feature branch HEAD) | ❌ Not yet |

## Device State

- **Board:** ESP32-S3 NanoS3 (Unexpected Maker)
- **Firmware:** `0.0.10-dev.10` (USB flashed)
- **IP:** `<device-ip>`
- **Serial port:** `/dev/cu.usbmodem101`
- **OTA channel:** `dev`
- **Heap (idle):** ~167 KB free, ~90 KB max_alloc

## Versioning Rule Changes Made This Session

1. Line 77: Dev tags ARE published as GitHub pre-releases (was: "never promoted")
2. Summary table: Agents MAY run `deploy_release.py` for dev tags autonomously
3. deploy_release.py references: removed `--tag` flag (auto-discovers), `ensure_tag_on_head` → `discover_tag_on_head` (user edits)

## Files Modified (Source Code)

| File | Change |
|---|---|
| `sdkconfig.defaults` | Added PR-29 mbedTLS tuning block (7 settings) |
| `modules/OTAUpdateManager/OTAUpdateManager.cpp` | Manifest download: `setCACert()` → `setInsecure()` (temporary, WO4 will replace) |
| `.agents/rules/03-versioning.md` | Dev release policy fix + user edits |

## Files Created (Agent Workspace)

| File | Purpose |
|---|---|
| `.agents/plugins/principal-architect-workspace/pr-29-tls-optimization/PROJECT.md` | Project registry |
| `.../wo1-sdkconfig-changes/WORKORDER.md` | WO1 instructions |
| `.../wo2-build-test-measure/WORKORDER.md` | WO2 instructions |
| `.../wo2-build-test-measure/RESULTS.md` | Measured heap/flash data |
| `.../wo3-skill-documentation/WORKORDER.md` | WO3 instructions |
| `.../wo4-ota-ca-pinning/WORKORDER.md` | WO4 instructions (pending) |
| `.agents/skills/framework-expert/performance/tls-memory-optimization.md` | Reference doc (312 lines) |
