# ADR: PsychicHTTP Multipart Field Dispatch

## Decision

Match upload fields by **file extension** (`.endsWith(".sig")` / `.endsWith(".bin")`) instead of form field name.

## Context

PsychicHTTP's `MultipartProcessor` parses `Content-Disposition` headers and extracts both `name` (form field name) and `filename` (actual file name). However, the upload callback signature is:

```cpp
_uploadCallback(_request, _itemFilename.c_str(), _itemSize - _itemBufferIndex, _itemBuffer, _itemBufferIndex, last);
```

It passes `_itemFilename` — the **actual filename** from the Content-Disposition header (e.g. `"firmware-esp32dev.bin"`), NOT the form field name (`"firmware"`). This is visible in `MultipartProcessor.cpp` line 132.

## Impact

Any code comparing the `filename` parameter against form field names (e.g. `filename == "signature"`) will **silently fail** — the comparison never matches, the field data is silently ignored, no error is raised, and the upload handler effectively becomes a no-op.

## Evidence

- `MultipartProcessor.cpp:132` — `_uploadCallback` receives `_itemFilename`
- `MultipartProcessor.cpp:208-221` — `_itemName` and `_itemFilename` are parsed separately from Content-Disposition
- Only `_itemFilename` is passed to the callback; `_itemName` is used only for `addParam()` (non-file form fields)

## Alternative Considered

Modifying PsychicHTTP to pass field name instead — rejected as it would break the upstream library contract and complicate updates.
