# Graveyard: WO3 Pipeline Rewire

## Things That Worked First Time
This WO was relatively clean — no dead-ends or failed attempts. The key risk areas were:

1. **Case-sensitive path for git add** — The filesystem has `modules/OTAUpdateManager/` (capital O) but the work order references `modules/otaUpdateManager/`. The `git add` with lowercase path silently succeeds but doesn't stage the file. Must use the exact casing from `git status`.

2. **MCP build serialisation** — Cannot run two PlatformIO MCP builds in parallel for the same project. The second build fails with "A build is already actively running". Must wait for the first to complete.

## Potential Gotchas for WO4/WO5

1. **`checkForUpdate()` is now a stub on hardware** — It returns `false` with a log warning. The MCP `ota_check_now` tool calls this method, so it will always report "no update available" until WO5 wires it to the manifest flow. The `ota_trigger_update` tool calls `performUpdate()` which IS fully wired.

2. **`OTA_GITHUB_TOKEN` define name unchanged** — The `#ifdef OTA_GITHUB_TOKEN` guard in `main.cpp` still uses the old define name. This is intentional — the build system / secrets infrastructure uses this name. Only the struct field was renamed to `otaToken`.

3. **Legacy `downloadAndParseManifestVersion_()` still behind `#if 0`** — Both this and the `checkForUpdate()` GithubClient section are WO5 cleanup targets. They reference the old v1 manifest struct fields (`manifest.version`, `manifest.channel`) that no longer exist in the v2 struct.
