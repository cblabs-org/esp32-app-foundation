# ADR: BoardCatalog Completeness — BootBoard In, ScreensaverBoard Out

## Decision
BoardCatalog now contains all display boards except ScreensaverBoard:
- `boot` (BootBoard&) — driven via IStatusReporter during bootstrap
- `configuringStatus` (ConfiguringStatusBoard&) — App pushes via showBase() during CONFIGURING
- `wifiWizard` (WiFiWizardBoard&) — App configures directly
- `otaProgress` (OTAProgressBoard&) — OTA module drives
- `error` (ErrorBoard&) — any module via reportError

## Context
The original design spec §7.8.4 excluded BootBoard and ConfiguringStatusBoard as "system-managed". But:
- ConfiguringStatusBoard is now pushed by Application via `display.showBase(&display.boards.configuringStatus)` — it IS caller-driven
- BootBoard: user requested consistency — "feels tidier, like we have a catalog of all displays that we use"

## BootBoard API Temptation Risk
With BootBoard in the catalog, someone could call `display.boards.boot.setStatus()` directly instead of `display.reportStatus()`. The `reportStatus()` route is correct because it also manages the modal layer and resets the activity timer. This is a documentation/convention concern, not a code problem.

## ScreensaverBoard Exclusion
ScreensaverBoard is display-intrinsic — managed by the `display.screensaver` sub-object's inactivity timer. No external caller ever needs to reference it by type. It remains accessed via `display.screensaver`.

## Header Safety
BootBoard.hpp only includes IDisplayBoard.hpp, LabelWidget.hpp, ImageWidget.hpp, ProgressBarWidget.hpp — no hardware dependencies. Compiles cleanly in native builds (verified: 412/412 tests pass).
