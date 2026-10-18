# WO1: Redactor Core Hardening

## Objective
Harden the Redactor to provide genuine zero-allocation registration and lock-correctness.

## Tasks
1. **Fixed Storage**: Replace `std::string` array with `char _secrets[16][192]` in `Redactor.h/cpp` (default `MAX_SECRET_LEN` to 192).
2. **RAII WriterLock**: Implement a single `WriterLock` wrapper for `portMUX` / `std::mutex`.
3. **Find Bounded**: Implement `findBounded()` to remove reliance on NUL-termination.
4. **RegisterResult**: Return `RegisterResult` enum (Registered, AlreadyPresent, TooShort, TooLong, Full, Invalid) instead of `bool`.
5. **Descending Scan**: Order secret scanning from longest to shortest.
6. **Clear Secrets**: Add `memset` on `clearSecrets()` and compile it out for non-native targets.
7. **Tests**: Update `test_redactor.cpp` to cover bounded-search and unterminated-buffer cases.
