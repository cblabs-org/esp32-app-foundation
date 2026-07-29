# Graveyard — WO4 Flag Coordination & Build Issues

## Failed Attempts & Lessons Learned

1. **Flag syntax error `-U WITH_TIME` vs `-UWITH_TIME` in `platformio.ini`**:
   - *Attempt*: Adding `-U WITH_TIME` to `build_flags` in `platformio.ini`.
   - *Failure*: SCons/GCC treats the space as an invalid macro identifier in command-line defines: `<command-line>: error: macro names must be identifiers`.
   - *Fix*: Use `-UWITH_TIME` without a space.

2. **Missing `U8G2Display` dependency in `sample-app/platformio.ini`**:
   - *Attempt*: Building `sample-app` with `lib_compat_mode = strict` and `U8G2DisplayModule.hpp` included in `main.cpp`.
   - *Failure*: `src/main.cpp:67:10: fatal error: U8G2DisplayModule.hpp: No such file or directory`. `strict` mode did not include U8G2Display's path.
   - *Fix*: Set `lib_compat_mode = off` and explicitly add `U8G2Display` to `lib_deps` in `sample-app/platformio.ini`.
