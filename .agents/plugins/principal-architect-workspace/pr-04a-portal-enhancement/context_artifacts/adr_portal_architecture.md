# ADR: Portal SPA Architecture

## Context
The portal uses a single-page application pattern driven by `app.js`. Bottom navigation tabs and sub-pills are built dynamically from `/api/tabs` JSON.

## Decisions

### Sequential Script Loading
Scripts MUST be loaded sequentially via `onload` chaining (not `forEach` + `createElement`). The reason: `loadTabs()` dispatches `tab-changed` events to activate the default tab. If fragment scripts (wifi.js, system.js, etc.) haven't finished loading when the event fires, their listeners aren't registered and the initial data fetch never happens. The boot sequence is: `loadScripts(callback)` → callback calls `loadTabs()`.

### Tab-Changed Event Suppression (Removed)
The original code had `if (!isInit)` guards that suppressed `tab-changed` events during initial load. This was removed because it prevented WiFi auto-population. The `isInit` variable still exists for the sub-pill path (where `activateSubPill` always dispatches) but the top-level else branch now always dispatches.

### Status Sub-Pill Injection
The System tab's "Status" pill is not backed by an `IWebContributor` module. It is injected synthetically in `webServer.cpp` before serializing `/api/tabs`:
```cpp
JsonObject statusTab = tabsArr.add<JsonObject>();
statusTab["name"]   = "Status";
statusTab["parent"] = "System";
statusTab["order"]  = 10;
```
The container `<div id="tab-status">` lives in `index.html`. `system.js` targets `tab-status`, NOT `tab-system`.

### Password Indicator Pattern
WiFi password uses `●●●●●●●● (saved)` placeholder with greyed eye toggle. Two state variables track this:
- `hasExistingPass` — set from `/api/wifi/status` response field `passExists`
- `userIsTypingPass` — set on input event, cleared when input emptied

### Storage Bar Convention
All progress bars show **free** space (not used). RAG coloring: ≥50% free = green, ≥25% = amber, <25% = red. Storage was originally "Used" which inverted the RAG meaning.

### Screenshot Tool
Standalone page at `/screenshot.html` (not embedded in SPA). Linked from the Status tab via a card. Uses U8G2 vertical-byte XBM decoding client-side. Served as PROGMEM asset via portalBuilder.py pipeline.
