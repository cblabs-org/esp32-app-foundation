# Architecture Decision Record: Interface Headers

## Pure Interface Files
All interfaces under `include/interfaces/` are authored as pure abstract classes containing only pure virtual methods and a default virtual destructor (e.g. `virtual ~ITickable() = default;`). This enforces clear, decoupled contracts between the kernel and application modules.

## Forward Declarations for Heavy Classes
To avoid dragging in heavy external dependency headers (like AsyncWebServer or U8G2) into the interface headers, we forward-declare classes (e.g. `class AsyncWebServer;` and `class U8G2;`) instead of `#include`ing their headers. This minimizes compile times and header pollution.

## Naming Conventions
File and class capitalizations strictly follow the program spec:
- PascalCase with I-prefix for: `ITickable.hpp`, `IStatusReporter.hpp`, `IWebContributor.hpp`, `IDisplayBoard.hpp`
- camelCase with lowercase i-prefix for: `iConfigurable.hpp`, `iDataSource.hpp`
