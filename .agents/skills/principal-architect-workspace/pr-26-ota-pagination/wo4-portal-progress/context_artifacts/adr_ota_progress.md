# ADR - OTA Real-Time Progress Exposure

## Context
Previously, the web portal showed static hardcoded percentages for OTA states (50% during download, 85% during verification). In contrast, the local hardware display (u8g2) was already receiving real-time fractional progress updates (0.0 to 1.0) via event bus propagation.

## Decision
1. Add `float lastProgress_` state inside `OTAUpdateManager` header to retain the latest progress update received in `publishProgress()`.
2. Expose the `lastProgress_` member directly in the `/api/ota` JSON response payload under the `"progress"` key.
3. Modify `web/portal/ota.js` to compute `pct` dynamically: `let pct = Math.round((response.progress || 0) * 100);` while preserving the textual label updates for downloading and verification stages.

## Consequences
- The progress bar in the Web Portal UI now accurately matches the actual state of downloading and file signature verification instead of jumping to static values.
- Real-time updates match what is shown on the local display.
