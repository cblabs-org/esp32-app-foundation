# OTA Floating Tag Architecture — Session Distillation

## Executive Summary

Refactored the OTA update system from a brittle paginated GitHub API scan to an O(1) floating-tag architecture. Fixed a critical SemVer parsing bug where the ESP32 could not extract version numbers from floating tag releases, and updated all documentation layers (agent skills, human docs, code comments) to reflect the new design contract.

## Next Steps

1. **Deploy `v0.0.11-dev.14`** to test OTA end-to-end. The device is currently running `dev.13` with the fixed parser. Run:
   ```bash
   python3 scripts/deploy_release.py --tag v0.0.11-dev.14 --local --build --private-key private_key.pem
   ```
2. **Monitor the device** via MCP `start_monitor` / `query_logs` to confirm it detects `dev.14` via the `latest-dev` floating tag and performs the OTA update.
3. **Retroactively fix `latest-dev` title** — the existing `latest-dev` release on GitHub was created with the old title format (`"Latest Dev Build"` instead of the semantic version). The next `deploy_release.py` run will automatically fix this.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_floating_tags.md` — Architecture decision record for the floating tag design, including why pagination was abandoned and why the release title carries the SemVer.
- `context_artifacts/graveyard_pagination.md` — Failed pagination approach, memory exhaustion evidence, and GitHub API limitations that drove the redesign.
- `context_artifacts/state_deploy_contract.md` — The critical contract between `deploy_release.py` and `GithubClient.cpp` that must be maintained.
