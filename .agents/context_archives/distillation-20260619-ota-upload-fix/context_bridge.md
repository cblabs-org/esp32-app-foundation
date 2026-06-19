---
title: "OTA Manual Upload Fix — PsychicHTTP Filename Matching & Progress Bar"
distilled_at: "2026-06-19T20:08:00Z"
original_plan_id: standalone
artifacts:
  - context_artifacts/adr_psychichttp_multipart_field_dispatch.md
  - context_artifacts/adr_upload_verification_lifecycle.md
  - context_artifacts/graveyard_upload_debugging.md
---

# Executive Summary

Fixed a critical bug where browser-based manual OTA firmware uploads would hang indefinitely with no response. Root cause was twofold: (1) PsychicHTTP passes the actual *filename* to the upload callback, not the form field name — so field-name comparisons silently dropped both firmware and signature data; (2) multipart fields are processed sequentially, so the signature hadn't arrived when the firmware's final chunk triggered verification. Added a real-time progress bar, detailed serial instrumentation, and proper error handling.

# What Was Done

1. **Bug fix (filename matching):** Changed `filename == "signature"` / `filename != "firmware"` to `filename.endsWith(".sig")` / `filename.endsWith(".bin")` in `handleSignedUpload()`.
2. **Bug fix (field ordering):** Moved all verification + response logic from the per-chunk `handleSignedUpload(final==true)` block into a new `finaliseSignedUpload()` method, called from PsychicHTTP's `onRequest` callback (fires after ALL multipart fields are received).
3. **Progress bar:** Replaced `fetch()` + `ApiQueue` with direct `XMLHttpRequest` — enables `upload.onprogress` for real-time KB counter. UI transitions: blue (uploading) → amber (verifying) → green (success) / red (error).
4. **Serial instrumentation:** Logs at every stage — field identification, verifier init, flash progress every 128 KB, verification result, reboot.
5. **Error handling:** XHR with 120s timeout, network error handler, server error messages surfaced in UI with colour-coded status.
6. **deploy_release.py:** Added `--sign-only` flag for local signing without GitHub release creation.
7. **FormData ordering:** Signature sent before firmware as belt-and-suspenders.

# Key Files Changed

| File | Change |
|------|--------|
| `modules/OTAUpdateManager/OTAUpdateManager.cpp` | Fix field matching, new `finaliseSignedUpload()`, instrumentation |
| `modules/OTAUpdateManager/OTAUpdateManager.hpp` | Add `finaliseSignedUpload()` declaration |
| `web/portal/ota.js` | XHR progress bar, bypass ApiQueue, error handling |
| `scripts/deploy_release.py` | `--sign-only` flag |
| `docs/img/cblabs_320x100.png` | CB Labs logo (38 KB, not yet integrated into portal) |

# Next Steps

- [ ] **CB Labs logo in Credits tab** — deferred pending decision on LittleFS static asset serving (see todo backlog). The 38 KB PNG is too large to embed directly without a serving architecture decision.
- [ ] **Test wrong-device binary upload** — user mentioned wanting to test this but didn't get to it. The ESP32 magic byte check (0xE9) is the same for all ESP32 variants, so a wrong-device binary would pass header validation but may fail at `Update.end()` or at boot.
- [ ] **GitHub OTA JSON parse failures** — `IncompleteInput` errors on `/releases` endpoint persist. Pre-existing issue, unrelated to this fix.

> [!WARNING]
> Do NOT read the detailed artifacts below unless your current task explicitly requires the deep context. If needed, use your read tool on `context_artifacts/[filename]`.

- `context_artifacts/adr_psychichttp_multipart_field_dispatch.md` - How PsychicHTTP's MultipartProcessor dispatches upload callbacks and why field-name matching fails
- `context_artifacts/adr_upload_verification_lifecycle.md` - Why verification must happen in onRequest, not in the per-chunk handler
- `context_artifacts/graveyard_upload_debugging.md` - Failed debugging attempts and red herrings during the upload investigation
