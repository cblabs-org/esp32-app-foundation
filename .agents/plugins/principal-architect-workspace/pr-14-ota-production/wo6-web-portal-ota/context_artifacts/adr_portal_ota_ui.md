# ADR: Portal OTA UI Activation (WO6)

## Decision: Typed Rollback Confirmation over Armed Button

**Context:** The existing rollback button used a 3-second armed confirmation window (click once to arm, click again within 3s to confirm). The WO6 spec required replacing this with a typed confirmation.

**Decision:** Used `window.prompt()` requiring the user to type "ROLLBACK" (case-sensitive). This is simpler, more explicit, and harder to accidentally trigger than a timed window.

**Rationale:** The `prompt()` approach was explicitly recommended by the work order to avoid the complexity of a custom modal. The typed confirmation is also sent as a `confirm` field in the API payload, enforced server-side (returns 403 if missing/invalid), providing defense-in-depth.

## Decision: Dual Data Source for Firmware Info

**Context:** The firmware info card originally only fetched from `GET /api/system`. The WO6 spec requires displaying channel, board ID, and auto-check status.

**Decision:** `loadFirmwareInfo()` now makes two requests: `GET /api/system` for version/partition, then `POST /api/ota { action: "status" }` for channel/boardId/autoCheck. The second request also syncs the channel dropdown.

**Rationale:** The system info endpoint is owned by a different module and should not be modified per the boundary box. The OTA status endpoint was the natural place to expose OTA-specific config fields.

## Decision: Channel Persistence via ConfigManager

**Context:** The `setChannel` API action needs to persist the channel selection across reboots.

**Decision:** Used the existing `ConfigManager::saveModuleConfig(this)` pattern, which serializes via `serializeConfig()` to `/mod/ota.json`. This is identical to how `checkForUpdate()` already persists `_lastCheckedChannel`.

**Rationale:** No new persistence mechanism needed — the IConfigurable infrastructure already handles this cleanly.
