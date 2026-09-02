# WO1: Redactor Core Hardening (Completed)

## Tasks Executed
1. **Fixed Storage**: Rewrote `Redactor` to use fixed arrays `char _secrets[MAX_SECRETS][MAX_SECRET_LEN]` instead of `std::string`. `MAX_SECRET_LEN` is `192`. Removed `#include <string>` internally for storage.
2. **RAII WriterLock**: Implemented a local `WriterLock` class wrapper in `Redactor.cpp` encapsulating `portENTER_CRITICAL` and `portEXIT_CRITICAL` / `std::mutex` to prevent early-exit imbalances.
3. **RegisterResult Enum**: Updated `registerSecret` signature to return `RegisterResult` (Registered, AlreadyPresent, TooShort, TooLong, Full, Invalid).
4. **Descending Scan**: Implemented an insertion sort in `registerSecret` to keep the secrets array sorted by length (descending). Tested via `DescendingScanOrder` to prevent partial masking (e.g. `pass` inside `password`).
5. **Find Bounded**: Added static `findBounded` utilizing `std::memchr` and `std::memcmp` to ensure robust checking against non-NUL-terminated byte arrays without out-of-bounds reads.
6. **Tests**: Expanded the GoogleTest suite in `test_redactor.cpp` with tests for tail-prefix masking, unterminated buffers, short/long boundaries, and descending scan logic.

All 17 native test cases in `test_redactor` pass (`pio test -e native -f "*test_redactor*"`).
