# Graveyard: Native Build Guards

## Stripping Headers using Python Script without Preprocessor Awareness
We initially attempted to strip `#ifndef NATIVE_BUILD` and `#endif // NATIVE_BUILD` using a dumb string replacement python script.
**Why it failed:** The script blindly stripped `#ifndef NATIVE_BUILD` and comments, but missed bare `#endif`s and `#else` blocks that didn't have comments attached to them. This led to orphaned `#endif`s and `#else` blocks (e.g., in `U8G2DisplayModule.hpp` where fallback native variables were defined in an `#else` block).
**The Fix:** We used the `unifdef -m -UNATIVE_BUILD` utility tool over the module files, which natively understands C preprocessor directives and perfectly pruned out the native branches while preserving the hardware branches.

## Leaving `test_displayMcpTools.cpp` Untouched
After ignoring `U8G2Display` in `platformio.ini`, `test_mcpServer` failed in native build.
**Why it failed:** `test_displayMcpTools.cpp` specifically `#include`s `U8G2DisplayModule.hpp` to test MCP tool responses. Since `U8G2DisplayModule.hpp` was no longer available in native include paths, the build failed.
**The Fix:** Wrapped the contents of `test_displayMcpTools.cpp` in `#ifdef HAS_DISPLAY` so that the GoogleTest suite sees an empty file and ignores it during the native build.
