# Graveyard: Interface Headers

## Including Heavy Headers in Contracts
Initial drafts included `<ESPAsyncWebServer.h>` in `IWebContributor.hpp` and `<U8g2lib.h>` in `IDisplayBoard.hpp`. This caused compilation issues in the native host test environment since those libraries are not compiled/supported on the host CPU.
- **Lesson Learned**: Always use forward declarations in interface files when pointing to external libraries to keep the contracts fully compile-portable across both target devices and host-native build environments.
