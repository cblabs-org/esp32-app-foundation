# State: Files Changed in WO1

## Files Modified

| File | Summary |
|---|---|
| `lib/OTAManifest/OTAManifest.hpp` | v2 struct redesign. Added `kMaxChannels`, `kMaxUrlLen`. `OTAAsset` fields: `firmwareUrl[256]`, `signatureUrl[256]` (were `firmware[64]`, `signature[64]`). New `OTAChannel` struct. `OTAManifest` now has `manifestVersion`, `manifestUrl`, `channels[]`, `channelCount`. Added `findChannel()` decl. `findAssetForBoard()` takes `OTAChannel&`. |
| `lib/OTAManifest/OTAManifest.cpp` | v2 parser. `parseManifest()` iterates `channels` JSON object keys. Reads `firmwareUrl`/`signatureUrl`. Implements `findChannel()` (linear scan by name). `findAssetForBoard()` accepts `OTAChannel&`. |
| `scripts/generate_manifest.py` | v2 output. Added `--base-url`, `--manifest-url` args. Outputs `manifestVersion:1`, `manifestUrl`, `channels` object with absolute `firmwareUrl`/`signatureUrl`. Supports single-channel and multi-channel modes. v1 format completely removed. |
| `test/test_unit/test_ota_manifest/test_ota_manifest.cpp` | 14 test cases exercising v2 schema: multi-channel parsing, `findChannel()`, `findAssetForBoard(OTAChannel&)`, overflow capping (channels and boards), cross-channel isolation, URL null termination, per-channel securityVersion, edge cases. |

## Key Struct Signatures (for downstream WOs)

```cpp
// OTAAsset — per-board (v2)
struct OTAAsset {
    char     board[kMaxBoardIdLen];        // 24
    char     firmwareUrl[kMaxUrlLen];      // 256
    char     signatureUrl[kMaxUrlLen];     // 256
    uint32_t size;
    char     sha256[kMaxSha256Len];        // 65
};

// OTAChannel — per-channel (NEW)
struct OTAChannel {
    char     version[kMaxVersionTagLen];   // 32
    char     name[16];
    uint32_t securityVersion;
    OTAAsset assets[kMaxBoards];           // kMaxBoards = 4
    uint8_t  assetCount;
};

// OTAManifest — top-level (v2)
struct OTAManifest {
    uint8_t    manifestVersion;
    char       manifestUrl[kMaxUrlLen];    // 256
    OTAChannel channels[kMaxChannels];     // kMaxChannels = 4
    uint8_t    channelCount;
};

// Free functions
bool parseManifest(const char* json, size_t jsonLen, OTAManifest& out);
const OTAChannel* findChannel(const OTAManifest& manifest, const char* channelName);
const OTAAsset* findAssetForBoard(const OTAChannel& channel, const char* boardId);
```

## Expected Downstream Breakage

These files reference old v1 struct members and will fail to compile until WO3:
- `modules/OTAUpdateManager/OTAUpdateManager.cpp` — `manifest.version`, `manifest.channel`, `manifest.assetCount`, `findAssetForBoard(manifest, BOARD_ID)`
