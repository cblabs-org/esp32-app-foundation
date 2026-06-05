# State: OTAManifest Integration Points

## File Map

### Created Files
| File | Purpose |
|------|---------|
| `lib/OTAManifest/OTAManifest.hpp` | Public API: `OTAAsset`, `OTAManifest` structs, `parseManifest()`, `findAssetForBoard()` |
| `lib/OTAManifest/OTAManifest.cpp` | ArduinoJson-based parsing implementation |
| `test/test_unit/test_ota_manifest/test_ota_manifest.cpp` | 9 GoogleTest cases for manifest parsing and board matching |

### Modified Files
| File | Change Summary |
|------|---------------|
| `platformio.ini` | Added `-DBOARD_ID` build flags to `esp32dev`, `esp32s3nano`, `native` envs |
| `modules/OTAUpdateManager/OTAUpdateManager.cpp` | Integrated manifest-aware asset selection with legacy fallback |

## OTAUpdateManager Integration Detail

The manifest integration lives in `performUpdateTask()` (the FreeRTOS task function). The flow:

1. **Scan release assets** for `manifest.json` — same loop that finds `.bin`/`.sig` files
2. **If manifest found**: HTTP GET the manifest URL → parse into `OTAManifest` struct → `findAssetForBoard(BOARD_ID)` → resolve firmware/signature filenames back to download URLs by matching against `github.releaseAssetName[]`
3. **If manifest NOT found**: Legacy path — scan for any `.bin`/`.sig` assets directly (backward compat)
4. **If board not in manifest**: Abort with error log `"No firmware for this board in manifest"`

The manifest download reuses the existing `WiFiClientSecure` + `HTTPClient` pattern from the signature fetch code path. The manifest JSON is read into a 2048-byte stack buffer.

## Test Coverage
- 9 dedicated manifest tests + 481 existing tests = **490 total, all passing**
- Tests cover: valid parsing, board matching (2 boards), unknown board, empty assets, malformed JSON, overflow cap, null-termination, securityVersion parsing
