# ADR: iConfigurable Interface Design & IWebContributor Navigation Hierarchy

## iConfigurable — 6-Method Contract

| Method | Signature | Purpose |
|---|---|---|
| `getConfigNamespace()` | `const char*` (const) | Unique namespace identifier (e.g. "wifi", "display") |
| `getSchemaVersion()` | `int` (const) | Current schema version for migration support |
| `applyConfig()` | `void(const JsonObject&)` | Apply a JSON config section to internal state |
| `serializeConfig()` | `void(JsonObject&) const` | Write current state to a JSON section |
| `resetConfig()` | `void()` | Reset to factory defaults |
| `migrateConfig()` | `bool(JsonObject&, int)` | Optional: migrate old schema → current (default: no-op returns false) |

### Design Rationale
- **`applyConfig` takes `const JsonObject&`**: The module reads from the section but never mutates the source document.
- **`serializeConfig` is `const`**: Serialization must not alter module state. Test mocks that track call counts must use `mutable` flags.
- **`migrateConfig` has a default implementation**: Most modules don't need migration, so the base class returns `false` (no migration performed). Only modules with schema evolution override this.
- **Namespace uniqueness**: Not enforced at registration time (no linear scan), but collisions would cause config file overwrites. The fixed MAX_CONFIGURABLES=8 makes manual audit trivial.

## IWebContributor — Navigation Hierarchy

Added two new virtual methods with defaults:
- `getParentTab()` → `nullptr` (top-level tab)
- `getTabOrder()` → `0` (default sort priority)

### Tab Hierarchy Example
```
System (parent=nullptr, order=0)
├── Config (parent="System", order=25)
├── WiFi   (parent=nullptr, order=10)
└── OTA    (parent=nullptr, order=50)
```

The `/api/tabs` endpoint now includes `parent` and `order` fields in the JSON response, enabling the web portal to render nested tab navigation.
