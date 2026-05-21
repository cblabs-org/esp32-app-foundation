---
name: pr-05c-wo3
description: Handoff instructions for Work Order 3 — Low-level lib tests (deviceCrypto, logger, md5Utils, githubClient) + keyManager expansion
---

# PR-05c / WO3 — Low-Level Library Tests + KeyManager Expansion

You are an expert senior software engineer and execution-focused worker agent. You are NOT the Principal Architect. Do not make high-level architectural decisions, do not branch, and do not plan. Your task is strictly limited to executing the checklist below.

---

## MANDATORY SKILL BOOT SEQUENCE — Execute FIRST

Run these view_file calls as your first five actions, in this exact order:
1. `view_file .agents/skills/pio-manager/SKILL.md`
2. `view_file .agents/skills/embedded-systems/SKILL.md`
3. `view_file .agents/skills/house-style-docs/SKILL.md`
4. `view_file .agents/skills/oo-expert/SKILL.md`
5. `view_file .agents/skills/pio-manager/references/mcp-agent-reference.md`

Do NOT read any project code or begin work until all five are loaded.

---

## Objective

Create new test suites for four low-level libraries that currently have **zero** test coverage, and expand the existing `keyManager` test suite:

| Module | Current | Target | File |
|---|---|---|---|
| `deviceCrypto` | 0 | 8+ | `test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp` (NEW) |
| `logger` | 0 | 8+ | `test/test_unit/test_logger/test_logger.cpp` (NEW) |
| `md5Utils` | 0 | 6+ | `test/test_unit/test_md5Utils/test_md5Utils.cpp` (NEW) |
| `githubClient` | 0 | 8+ | `test/test_unit/test_githubClient/test_githubClient.cpp` (NEW) |
| `keyManager` | 5 | 12+ | `test/test_unit/test_keyManager/test_keyManager.cpp` (EXPAND) |

---

## Strict Boundary Box

- **DO NOT** modify any file under `modules/` EXCEPT `keyManager/` ONLY if a `NATIVE_BUILD` guard fix is absolutely required.
- **DO NOT** modify `platformio.ini`.
- **DO NOT** delete or alter ANY existing test cases in `test_keyManager/` — ONLY append new ones.
- You are authorized to make **minimal** `NATIVE_BUILD` guard additions to library headers under `lib/` if the header cannot compile under the native environment otherwise.
- You are creating NEW test directories and files for `deviceCrypto`, `logger`, `md5Utils`, and `githubClient`.

---

## Pre-Requisite: Fix Library Headers for Native Build

### `md5Utils.hpp` — Currently has unguarded `#include <Arduino.h>`
Add `NATIVE_BUILD` guards:
```cpp
#ifndef NATIVE_BUILD
#include <Arduino.h>
#else
#include <string>
#include <cstdint>
#include <cstddef>
// Provide String stub if not already available
#ifndef STRING_STUB_DEFINED
#define STRING_STUB_DEFINED
class String {
public:
    String() {}
    String(const char* s) : _str(s ? s : "") {}
    String(const std::string& s) : _str(s) {}
    const char* c_str() const { return _str.c_str(); }
    bool isEmpty() const { return _str.empty(); }
    int length() const { return _str.length(); }
    String operator+(const char* s) const { return String(_str + s); }
    bool operator==(const String& other) const { return _str == other._str; }
    bool operator==(const char* s) const { return _str == s; }
private:
    std::string _str;
};
#endif
#endif
```

### `githubClient.hpp` — Currently has unguarded `#include <Arduino.h>`
Add similar `NATIVE_BUILD` guards. The `String` stub must be consistent with `md5Utils`.

### `deviceCrypto` — Header already has guards, but `.cpp` may need a mock impl
The `deviceCrypto.cpp` uses `mbedtls` and `nvs_flash` which are ESP-IDF only. For native testing, you need to provide a mock `DeviceCrypto` subclass that performs simple XOR or passthrough encryption, since the base class has `virtual` `encrypt()` and `decrypt()` methods.

**Strategy:** Create a `MockDeviceCrypto` in the test file that overrides `encrypt()`/`decrypt()` with trivial implementations (e.g., base64-encode the plaintext for "encrypt", reverse for "decrypt"). This avoids modifying the production code.

---

## Reference Material

Before writing tests, read each library's header AND implementation:
- `lib/deviceCrypto/deviceCrypto.hpp` — `encrypt()`, `decrypt()`, `begin()` — methods are `virtual`
- `lib/deviceCrypto/deviceCrypto.cpp` — Uses `mbedtls_aes_*`, `nvs_*` — all ESP-IDF
- `lib/logger/logger.hpp` — `registerSecret()`, `logSecure()`, `redactInPlace()` — static methods
- `lib/logger/logger.cpp` — Implementation of redaction
- `lib/md5Utils/md5Utils.hpp` — `calculateFileMD5()`, `base64ToHex()`
- `lib/md5Utils/md5Utils.cpp` — Uses `mbedtls_md5_*` — ESP-IDF
- `lib/githubClient/githubClient.hpp` — `getLatestRelease()`, `getLastError()`
- `lib/githubClient/githubClient.cpp` — Uses `WiFiClientSecure` — ESP-IDF
- `modules/keyManager/keyManager.hpp` — `begin()`, `addKey()`, `getKeyById()`, `deleteKey()`
- `test/test_unit/test_keyManager/test_keyManager.cpp` — Existing 5 tests

---

## Line-Item Instructions

### Task 1: `test_deviceCrypto.cpp` (8+ tests) — NEW

Create `test/test_unit/test_deviceCrypto/test_deviceCrypto.cpp`.

Since `deviceCrypto.cpp` uses ESP-IDF mbedtls, create a `MockDeviceCrypto` subclass in the test file that overrides the virtual `encrypt()` and `decrypt()` methods with trivial implementations.

- [ ] **MockDeviceCrypto construction** — No crash on construction.
- [ ] **encrypt() returns non-null** — Mock `encrypt("hello", 5, &outLen)` returns a valid pointer.
- [ ] **decrypt() round-trip** — `encrypt()` → `decrypt()` recovers original plaintext.
- [ ] **encrypt() empty string** — `encrypt("", 0, &outLen)` handles edge case.
- [ ] **encrypt() nullptr** — `encrypt(nullptr, 0, &outLen)` returns nullptr or handles gracefully.
- [ ] **decrypt() invalid input** — `decrypt("not-valid", 9, &outLen)` handles gracefully.
- [ ] **encrypt() output length** — `outLen` is set to a reasonable value after encrypt.
- [ ] **Multiple encrypt calls** — Two sequential encrypts produce valid, independent outputs.

### Task 2: `test_logger.cpp` (8+ tests) — NEW

Create `test/test_unit/test_logger/test_logger.cpp`.

The logger uses `PIO_UNIT_TESTING` define (auto-set by PIO test runner) to stub ESP log macros. The `registerSecret()` and `logSecure()` methods are static.

**NOTE:** The `Logger` class uses a static `secrets` vector. You must be careful about test isolation. Consider calling the tests in a known order or accepting that secrets accumulate across tests. Read `lib/logger/logger.cpp` to understand `redactInPlace()`.

- [ ] **logSecure() without secrets** — `Logger::logSecure("TEST", "Hello %s", "world")` doesn't crash and outputs normally.
- [ ] **registerSecret()** — `Logger::registerSecret("mypassword")` succeeds (no crash).
- [ ] **logSecure() with secret redaction** — After `registerSecret("secret123")`, `logSecure()` with a message containing "secret123" should redact it (verify via stdout capture or just verify no crash).
- [ ] **Multiple secrets registered** — Register 3 secrets, all should be handled.
- [ ] **Empty secret** — `registerSecret("")` doesn't crash.
- [ ] **logSecure() with format specifiers** — `logSecure("TAG", "Value: %d, Name: %s", 42, "test")` works.
- [ ] **logSecure() with empty format** — `logSecure("TAG", "")` doesn't crash.
- [ ] **logSecure() with long message** — 500+ char message doesn't crash.

### Task 3: `test_md5Utils.cpp` (6+ tests) — NEW

Create `test/test_unit/test_md5Utils/test_md5Utils.cpp`.

**Challenge:** `md5Utils.cpp` uses `mbedtls_md5_*` and `LittleFS` — both ESP-IDF only. For native build, you need to either:
- Provide stubs that make the functions return predictable values, OR
- Test only `base64ToHex()` if it can be isolated.

Read `lib/md5Utils/md5Utils.cpp` to determine which functions can be tested natively.

- [ ] **Construction** — `Md5Utils` constructs without crash.
- [ ] **base64ToHex() known vector** — Convert a known base64 MD5 to hex and verify.
- [ ] **base64ToHex() empty input** — Handle empty string gracefully.
- [ ] **base64ToHex() invalid base64** — Handle malformed input without crash.
- [ ] **calculateFileMD5() nonexistent file** — Returns empty string (guarded in native or stubbed).
- [ ] **Multiple base64ToHex() calls** — Sequential calls produce independent results.

### Task 4: `test_githubClient.cpp` (8+ tests) — NEW

Create `test/test_unit/test_githubClient/test_githubClient.cpp`.

**Challenge:** `githubClient.cpp` uses `WiFiClientSecure` and network calls — all ESP-IDF. For native build, test only the constructible/queryable parts:

- [ ] **Construction with repo** — `GithubClient("owner/repo")` constructs successfully.
- [ ] **Construction with repo and token** — `GithubClient("owner/repo", "ghp_token123")` constructs.
- [ ] **getLastError() initial** — Returns a valid string (likely "SUCCESS" or similar initial value).
- [ ] **releaseId initial state** — `releaseId` is empty after construction.
- [ ] **releaseAssets initial state** — `releaseAssets` is 0 after construction.
- [ ] **Construction with empty repo** — `GithubClient("")` doesn't crash.
- [ ] **Construction with empty token** — `GithubClient("owner/repo", "")` constructs fine.
- [ ] **getLatestRelease() on native** — Returns false (no network), doesn't crash.

### Task 5: Expand `test_keyManager.cpp` (+7 tests)

Append to `test/test_unit/test_keyManager/test_keyManager.cpp`. Read existing tests first.

- [ ] **addKey with nullptr id** — `addKey(nullptr, "secret")` returns false.
- [ ] **addKey with empty id** — `addKey("", "secret")` returns false or handles gracefully.
- [ ] **addKey with nullptr secret** — `addKey("mykey", nullptr)` returns false.
- [ ] **getKeyById with nullptr** — `getKeyById(nullptr)` returns empty string.
- [ ] **getKeyById for non-existent key** — Returns empty string.
- [ ] **deleteKey with nullptr** — `deleteKey(nullptr)` returns false.
- [ ] **deleteKey non-existent key** — `deleteKey("no-such-key")` returns false.

---

## Verification

1. `pio test -e native` passes all tests including new ones.
2. All existing tests remain green (no regressions).
3. Production code changes limited to `NATIVE_BUILD` guard additions in `md5Utils.hpp` and `githubClient.hpp`.
4. Test counts: deviceCrypto ≥ 8, logger ≥ 8, md5Utils ≥ 6, githubClient ≥ 8, keyManager ≥ 12.

---

## Closure

Once verified, **STOP and ASK the user** if the implementation is validated and if any iteration is needed. Do not wrap up or distill until explicitly approved. Once approved, invoke the `@distillery` skill to package your output, passing `.agents/skills/principal-architect-workspace/pr-05c-test-hardening/wo3-lib-tests/` as the target directory. Finally, append a status update to this `WORKORDER.md` file and drop control back.
