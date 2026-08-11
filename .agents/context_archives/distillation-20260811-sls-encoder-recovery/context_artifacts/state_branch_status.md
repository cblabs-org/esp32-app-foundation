# State: Branch Status at Session End

## Merged

| PR | Branch | Merge commit | What |
|----|--------|-------------|------|
| [#75](https://github.com/matthewmcneill/esp32-app-foundation/pull/75) | `encoder-recovery` (deleted) | `7b5cc84` | EncoderModule recovery + CrowPanel adapter refactor |

## Open — awaiting hardware bring-up

| Branch | Commits ahead of main | Last rebased | Status |
|--------|----------------------|-------------|--------|
| `sls-templates/crowpanel-128` | 6 | 2026-08-05 (clean, no conflicts) | Blocked on WO10 hardware bring-up |

### Commits on `sls-templates/crowpanel-128`:
1. `e1ea0d0` feat(lvgl): add SquareLine Studio templates for CrowPanel 1.28" display
2. `9723c3e` feat(lvgl): integrate SLS-exported UI code and enable required fonts
3. `84285ac` chore: add .gitignore for SLS backup/cache directories
4. `7460e99` feat(lvgl): switch to smaller 320x100 CB Labs logo
5. `4171de6` chore(pa): draft WO13 — wire SLS screens to framework lifecycle
6. `9e86f9d` feat(lvgl): wire SLS-exported screens to framework lifecycle via SLSSystemScreens

### Key files on SLS branch (not on main):
- `squareline/crowpanel-128/` — SLS project (3 screens: Boot, System, Diagnostics)
- `modules/LVGLDisplay/ui/` — SLS-exported C code (screen init functions, widget globals, image assets)
- `modules/LVGLDisplay/screens/SLSSystemScreens.hpp/.cpp` — ILVGLSystemScreens bridge
- `test/stubs/platform/ui_stubs.c` — native build stubs for SLS globals
- `test/test_unit/test_lvglDisplay/test_sls_screens.cpp` — unit tests

### Build status (at last verification, pre-rebase):
- crowpanel-128-rotary: ✅ 1.76 MB (42% flash)
- native: ✅ 368/368 tests
- esp32s3nano: ✅ 1.22 MB
- **Post-rebase build not yet verified** — recommend running before merge
