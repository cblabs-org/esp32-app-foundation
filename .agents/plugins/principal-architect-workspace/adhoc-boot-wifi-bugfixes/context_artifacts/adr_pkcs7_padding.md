# ADR: PKCS7 Padding Silent Corruption in DeviceCrypto

## Context

During a serial monitor debugging session on 2026-06-11, the device was booting
correctly, loading WiFi credentials from KeyManager, logging "Decryption verified"
— but consistently failing 802.11 authentication with `AUTH_EXPIRE` (reason 2) on
every connection attempt to a known-good WPA2 network.

## Investigation Path

1. Serial monitor confirmed the device was reaching the AP (auth phase started)
   but being rejected — classic wrong-password symptom.
2. The access point's admin interface confirmed `<home-ssid>` is 2.4GHz-only WPA2, device MAC never appeared in
   client history, ruling out band/AP-side issues.
3. The credential round-trip (save → encrypt → persist → reload → decrypt) *appeared*
   to succeed: no error logs, "Decryption verified" printed.
4. Deep inspection of `DeviceCrypto::removePKCS7Padding()` revealed the bug.

## Root Cause

`removePKCS7Padding()` in `lib/DeviceCrypto/DeviceCrypto.cpp` line 101:

```cpp
// BEFORE (broken):
if (paddingValue == 0 || paddingValue > 16) return paddedLen;

// AFTER (fixed):
if (paddingValue == 0 || paddingValue > 16) return 0; // signal error
```

The fallback for an "invalid" padding byte was returning `paddedLen` — the *full*
AES block-aligned buffer length — instead of `0` (the error sentinel).

`getKeyById()` then constructed `std::string result(decrypted.get(), outLen)` with
`outLen = paddedLen`, silently including the raw PKCS7 padding bytes in the returned
string. For a typical WiFi password (e.g. 10 chars padded to 16), this appended 6
bytes of `\x06` to the password before it was passed to `WiFi.begin()`.

## Why It Was Silent

The branch `return paddedLen` logged *nothing*. The success-path log
`"Decryption verified"` was reached because `unpaddedLen > 0 && unpaddedLen <=
ciphertextLen` evaluated true (since `paddedLen == ciphertextLen` in this path).

## Decision

Return `0` from the invalid-padding branch. The caller already checks `outLen == 0`
and returns an empty string on error — a safe, visible failure rather than silent
corruption.

## Commit

`dff3b22` on `pr-21-ota-task-hardening`

## Files Changed

- `lib/DeviceCrypto/DeviceCrypto.cpp` line 101
