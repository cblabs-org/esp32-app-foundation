# Graveyard: Native Build Stub Issues

## Failed Attempt 1: Include Order in otaUpdateManager.cpp

**Error:**
```
modules/otaUpdateManager/otaUpdateManager.cpp:61:11: error: unknown type name 'FrameworkConfig'
```

**Root Cause:** The native stub `Application` class declared `const FrameworkConfig& getConfig() const` *before* `#include "../../include/FrameworkConfig.hpp"` was processed. The compiler saw `FrameworkConfig` as an undeclared type.

**Fix:** Moved `#include "../../include/FrameworkConfig.hpp"` and `#include "../eventBus/EventBus.hpp"` *above* the Application stub class declaration. The stub now looks like:

```cpp
#else
#include <ArduinoJson.h>
#include "../../include/FrameworkConfig.hpp"
#include "../eventBus/EventBus.hpp"
class Application { ... };
```

## Failed Attempt 2: Incomplete Type in Test File

**Error:**
```
test_otaUpdateManager.cpp:51:17: error: variable has incomplete type 'Application'
```

**Root Cause:** The test file included `otaUpdateManager.hpp` which forward-declares `class Application;`. When the test tried `Application app;`, the compiler couldn't construct an incomplete type. The `.cpp` stub provides the full definition, but that's a separate translation unit — the test's TU never saw the full class.

**Fix:** Added a matching `Application` stub class definition in the test file under `#ifdef NATIVE_BUILD`, before the `otaUpdateManager.hpp` include:

```cpp
#ifdef NATIVE_BUILD
#include "../../../include/FrameworkConfig.hpp"
#include "../../../modules/eventBus/EventBus.hpp"
class Application {
public:
    EventBus& getEventBus();
    const FrameworkConfig& getConfig() const;
};
#endif
#include "../../../modules/otaUpdateManager/otaUpdateManager.hpp"
```

The linker resolves the method implementations from `otaUpdateManager.cpp`'s stub. Both TUs see the same class definition so there's no ODR violation.

## Lesson Learned

When using the "Hollywood Principle" pattern (forward declarations in headers, full includes in `.cpp`), native test files that need to construct the forward-declared type must also provide the full type definition. The pattern is: define the stub in both the `.cpp` and the test, and let the linker merge the implementations.
