# ADR: Upload Verification Lifecycle — onRequest vs Per-Chunk

## Decision

Perform signature verification and `Update.end()` in the `onRequest` callback, NOT in the per-chunk `onUpload` handler's `final == true` branch.

## Context

PsychicHTTP's multipart processing works as follows:

1. `onUpload` callback fires for each chunk of each field, sequentially
2. `onRequest` callback fires ONCE after ALL fields have been fully received

The `FormData` from the browser sends fields in append order. If firmware is appended before signature, the entire firmware field (all chunks including `final == true`) is processed before the first byte of the signature field arrives.

## The Bug

Original code verified the signature in `handleSignedUpload()` when `final == true` on the firmware field. But at that point, `_uploadSigLen == 0` because the `.sig` field hadn't been received yet. Result: `SECURITY ABORT — no signature provided` every time.

## Solution

Split into two methods:
- `handleSignedUpload()` — pure data collection (write firmware to flash, accumulate signature bytes). No response sent.
- `finaliseSignedUpload(PsychicResponse*)` — called from `onRequest` after ALL fields received. Performs ECDSA verification, sends HTTP response, triggers reboot on success.

## Belt-and-Suspenders

Also swapped FormData append order in `ota.js` so signature is sent first. This means the signature arrives before firmware even in the per-chunk handler, but the architectural fix (onRequest) is the correct solution regardless of field order.
