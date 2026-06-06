# Graveyard: GetProjectOption for board_build.* Options

**Session:** WO2 — sync_sdkconfig.py  
**Date:** 2026-06-06

## What Failed

### Attempt: `env.GetProjectOption("board_build.cmake_extra_args", "")`

The WORKORDER spec provided this code pattern:
```python
cmake_args = env.GetProjectOption("board_build.cmake_extra_args", "")
```

**Symptom:** No `[sync_sdkconfig]` output appeared in the build log at all — not even an error. The script ran completely silently. The `try/except Exception: pass` block was silently swallowing the failure.

**Root cause:** `GetProjectOption` does not know about `board_build.*` options. PlatformIO's `board_build.*` INI options are NOT registered as standard project options (see `platformio/project/options.py` — only a few fixed ones like `board_build.mcu`, `board_build.f_cpu` are registered there). The `cmake_extra_args` key specifically is NOT a registered project option. Calling `GetProjectOption("board_build.cmake_extra_args")` raises an exception internally, which the `try/except` silently swallows, causing `_get_defaults_files()` to return `[]`, which causes `sync_sdkconfig()` to return immediately without printing anything.

**Diagnosis method:** 
1. Noticed no `[sync_sdkconfig]` output in the build log
2. Confirmed `portalBuilder.py` and `inject_version.py` output WAS present → script was running but silently falling through
3. Read `pioplatform.py` `LoadPioPlatform()` to understand the `board_build.* → BoardConfig("build.*")` merge
4. Read `espidf.py` line 2573 to confirm `board.get("build.cmake_extra_args")` is the correct API

## What Was Also Tried (Regex Issues)

The WORKORDER regex `r'-DSDKCONFIG_DEFAULTS=["\'"]?([^"\'\\s]+)["\'"]?'` had character class issues when embedded in Python via file editor (doubled single quotes `[\'\']` in negated class). The fix was to use:
```python
r'-DSDKCONFIG_DEFAULTS=["\'"]?([^"\'\s]+)["\'"]?'
```

The actual `cmake_extra_args` string is: `-DSDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.defaults.esp32s3nano"` — includes shell-style double-quoted value.

## What Works

```python
board = env.BoardConfig()
cmake_args = board.get("build.cmake_extra_args", "")
match = re.search(r'-DSDKCONFIG_DEFAULTS=["\'"]?([^"\'\s]+)["\'"]?', cmake_args)
```

This is proven correct by `espidf.py` using the same `board.get()` pattern on line 2573.

## Do NOT Repeat

- Do NOT use `env.GetProjectOption("board_build.*")` for any `board_build.*` key
- Do NOT use `env.GetProjectOption("board_build.cmake_extra_args")` specifically — it silently returns empty string or raises
- Always wrap `BoardConfig()` calls in `try/except` because `BoardConfig()` can fail if no board is configured (e.g., `native` env)
