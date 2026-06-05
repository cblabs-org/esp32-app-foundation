# ADR: SemVer Engine Design

## Bit Layout

`packVersion()` encodes `FirmwareVersion` into a single `uint32_t`:

```
MAJOR(8) | MINOR(8) | PATCH(8) | CHANNEL(4) | PRE_NUM(4)
  [31:24]    [23:16]    [15:8]      [7:4]       [3:0]
```

This allows single-instruction integer comparison (`<`, `>`, `==`) on MCU with no branching logic across fields.

## Channel Enum Values

Values are non-contiguous to ensure correct sort ordering via the packed integer:

| Channel  | Value | Rationale |
|----------|-------|-----------|
| DEV      | 0     | Lowest priority — always sorts below everything |
| BETA     | 2     | Gap at 1 reserved for future use |
| RC       | 3     | Just below stable |
| STABLE   | 15    | Maximum 4-bit value — always wins in same major.minor.patch |

## Zero-Allocation Strategy

- `parseVersionTag()` uses `strtoul()` with `char*` pointer arithmetic — no heap
- Channel matching uses `strncmp()` against static strings — no `String` objects
- `FirmwareVersion` is returned by value (12 bytes on stack)
- All comparison operators delegate to `packVersion()` — no virtual dispatch

## OTA Integration

`checkForUpdate()` constructs `local` from compile-time `VERSION_MAJOR/MINOR/PATCH` macros (always `Channel::STABLE`, `preNum=0` for local builds). `remote` is parsed from `GithubClient.releaseId` (which contains the GitHub `tag_name` field). Update is available iff `remote > local`.

## inject_version.py

PlatformIO `pre:` extra_script that runs `git describe --tags --exact-match`. On CI tagged builds, it overrides the static `VERSION_*` defines. On local dev builds, it's a no-op — the `platformio.ini` defaults apply.
