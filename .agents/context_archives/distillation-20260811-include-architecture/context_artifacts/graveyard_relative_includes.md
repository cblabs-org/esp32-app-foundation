# Graveyard: Relative Include Approaches

## ❌ Attempt 1: Per-file `../Module/` → bare include conversion (without LDF alignment)

**What**: Changed all `../EventBus/EventBus.hpp` to `"EventBus.hpp"` in modules/.
**Why it failed**: Native env uses `chain` LDF mode by default. Without `deep` mode, LDF doesn't follow `#include` chains through headers of non-directly-referenced libraries. Each fix in one file revealed a new failure in a different transitive dependency.
**Symptom**: Whack-a-mole — fixing EventBus.hpp broke DataManager.hpp, fixing that broke ConfigManager.hpp.

## ❌ Attempt 2: Revert to relative paths for "special" modules

**What**: Selectively reverted Application.hpp, ConfigManager.hpp, KeyManager.hpp back to `../` relative paths since they cross module boundaries.
**Why it's wrong**: Creates a byzantine set of rules — "bare includes for same-level modules, relative for Application, double-relative for lib/" — that no human or agent can consistently remember. Every new include is a landmine.

## ❌ Attempt 3: `lib_compat_mode = strict` on native

**What**: Set both `deep` and `strict` on native to match target exactly.
**Why it failed**: `strict` compat mode rejects libraries that don't declare `platforms` in their `library.json`. Libraries in `lib/` (ConfigManifest, DeviceCrypto, etc.) are platform-agnostic and don't declare platforms. LDF refused to discover them.
**Fix**: Used `soft` compat mode for native instead. Acceptable because native is test-only.

## ❌ Attempt 4: Deep mode without explicit `dependencies` in library.json

**What**: Set `lib_ldf_mode = deep` and removed Application from `lib_ignore`, but didn't add `dependencies` to Application's `library.json`.
**Why it failed**: LDF discovered Application and compiled its dependents, but didn't propagate Application's transitive include paths (EventBus, DataManager, etc.) to libraries that declare `"Application": "*"` as a dependency (ButtonModule, EncoderModule). The dependency graph was:
```
EncoderModule → Application → EventBus
```
Without explicit `dependencies` in Application's `library.json`, LDF didn't know to add `modules/EventBus/` to EncoderModule's include path.
**Fix**: Added `"dependencies": {"EventBus": "*", "DataManager": "*", ...}` to Application's library.json.

## Key Insight

The root cause was never per-file include paths — it was the configuration asymmetry. Once native used `deep` mode + Application was discoverable + dependencies were declared, ALL bare includes resolved automatically with zero per-file exceptions.
