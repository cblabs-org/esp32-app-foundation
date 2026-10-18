# Graveyard: WO3 Library Tests

### Linker Error: Architecture Mismatch
- **Attempt**: Executed `pio test -e native` immediately after WO3 implementation.
- **Failure**: The compilation crashed during the linking stage with `ld: warning: ignoring file ... found architecture 'x86_64', required architecture 'arm64'` and `Undefined symbols for architecture arm64: "_main"`.
- **Resolution**: Object files from older runs were cached under a different architecture. Executing `pio run -t clean -e native` before testing resolved the architecture mismatch and allowed the tests to correctly compile and execute.
