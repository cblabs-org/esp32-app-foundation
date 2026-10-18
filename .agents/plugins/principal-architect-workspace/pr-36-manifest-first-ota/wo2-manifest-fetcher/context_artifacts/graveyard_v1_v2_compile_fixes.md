# Graveyard: v1→v2 Compile Fixes in Legacy Pipeline

## Problem

WO1 changed the `OTAManifest` struct from a flat v1 schema (with `version`, `channel`, `assetCount`, `firmware`, `signature` fields) to a v2 multi-channel schema (with `channels[]`, `channelCount`, `manifestUrl`, `firmwareUrl`, `signatureUrl` fields). The existing `performUpdateTask()` code still referenced v1 fields and the now-`#if 0`'d `downloadAndParseManifestVersion_()` method.

## Errors Encountered

1. `findAssetForBoard(manifest, BOARD_ID)` — v1 API passed `OTAManifest&`, v2 requires `OTAChannel&`
2. `boardAsset->firmware` — renamed to `boardAsset->firmwareUrl` in v2
3. `boardAsset->signature` — renamed to `boardAsset->signatureUrl` in v2
4. `manifest.version` / `manifest.channel` / `manifest.assetCount` — v1 flat fields removed in v2

## Resolution (Temporary Shims)

Rather than fully porting the legacy pipeline to v2 (which WO3 replaces entirely), applied minimal shims:

1. **Floating tag path** (lines ~525-554): Replaced the `downloadAndParseManifestVersion_()` call with a graceful error return: "Legacy manifest download path disabled (pending WO3 rewire)". The floating tag path was already broken by WO1's struct changes.

2. **Manifest-based asset lookup** (lines ~585-628): Replaced the entire manifest path (which used `downloadAndParseManifestVersion_()`, `findAssetForBoard(manifest, ...)`, and v1 field names) with a fallback to the legacy `.bin`/`.sig` extension-based asset search. The manifest path now logs "Manifest-based asset lookup disabled (WO2)" and falls through.

3. **Old method in `#if 0`** (lines ~1085-1202): The `downloadAndParseManifestVersion_()` references to `manifest.version`, `manifest.channel`, `manifest.assetCount` are inside `#if 0` so they don't compile. No fix needed.

## WO3 Must

- Remove all temporary shims
- Wire `fetchAndVerifyManifest_()` into the pipeline
- Use `findChannel()` + `findAssetForBoard()` with the v2 API
- Remove the legacy asset search fallback
