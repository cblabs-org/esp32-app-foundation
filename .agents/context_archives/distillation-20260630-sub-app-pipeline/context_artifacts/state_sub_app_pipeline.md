# State: Sub-App Build Pipeline (Final)

## Pre-build Script Chain

All three projects now share the same ordered script chain:

| Order | Script | Root | Sample | Template | Purpose |
|-------|--------|------|--------|----------|---------|
| 1 | `sync_sdkconfig.py` | ✅ | ✅ | ✅ | Invalidate stale sdkconfig when .defaults change |
| 2 | `run_web_tests.py` | ✅ | ❌ | ❌ | Playwright web tests (root-only, guards portal) |
| 3 | `inject_version.py` | ✅ | ✅ | ✅ | Git tag → build flags |
| 4 | `merge_framework_data.py` | ✅ | ✅ | ✅ | Copy framework LittleFS files to sub-app data/ |
| 5 | `portalBuilder.py` | ✅ | ✅ | ✅ | Gzip web assets → PROGMEM headers |

## Build Environments

| Project | esp32dev | esp32s3nano | native |
|---------|----------|-------------|--------|
| Root | ✅ | ✅ | ✅ |
| Sample-app | ✅ | ✅ | ❌ |
| Template-app | ✅ | ✅ | ✅ |

## Key Gotcha: SCons `__file__`
PlatformIO's SCons context does **not** define `__file__`. Pre-build scripts must use relative paths from CWD (PIO sets CWD to `$PROJECT_DIR`) or `env.subst("$PROJECT_DIR")`. The initial `merge_framework_data.py` used `__file__` and failed — fixed by switching to `os.path.join("..", "data")`.

## Commits
- `78654f6` — Pipeline alignment (data merge, portal builder, OTA separation)
- `ff38bff` — Add esp32s3nano to template-app
