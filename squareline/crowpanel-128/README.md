# CrowPanel 1.28" Rotary Display SLS Template

This SquareLine Studio (SLS) project provides a complete starter template for the **Elecrow CrowPanel 1.28" Round Display** (ESP32-S3).

## What This Project Contains
This project consists of 3 distinct screens:
- **Boot Screen** (`Screen_Boot`): A splash screen with a logo, title, progress bar, and status label. It features a fade-in animation overlay.
- **System Screen** (`Screen_System`): A consolidated screen containing multiple hidden groups that the firmware shows dynamically, one at a time:
  - **Error** — fatal/terminal error (icon, title, message, dismiss button).
  - **Setup** — WiFi provisioning helper: AP SSID plus the config-portal URL to visit.
  - **OTA Progress** — firmware update status, progress bar, and percentage.
  - **Busy** — indeterminate "please wait" spinner with a status message.
  - **Message** — generic gate-helper modal (icon, title, body) for configuration prompts.
- **Diagnostics Screen** (`Screen_Diag`): A runtime consumer template for sensor readouts, featuring an edge arc, a category label, a large value readout, a status line, and a counter.

## Opening in SquareLine Studio
You can open this project in **SquareLine Studio v1.5+** by double-clicking the `SquareLine_Project.spj` file.
1. The project targets the `Arduino with TFT_eSPI` board (version `v2.3.0`).
2. It uses `LVGL 9.2.2`.
3. The display shape is configured as `CIRCLE` with a 240×240 resolution.

## Exporting Generated Code
1. Open the project in SquareLine Studio.
2. Ensure you have the `Arduino with TFT_eSPI` board pack installed in SLS.
3. Select **Export -> Create Template Project** (if not already created) and then **Export -> Export UI Files**.
4. Set the UI export path to your project's `modules/LVGLDisplay/ui/` directory.

## Connecting to LVGLDisplayModule
The generated `ui_init()` function must be called from within the `LVGLDisplayModule` setup sequence.
The module manages screen transitions and group visibility by querying widgets via their unique `ui_*` names and updating LVGL states.

## Widget Naming Conventions
Widgets are strictly prefixed with `ui_Boot*`, `ui_System*`, and `ui_Diag*`. 
This is critical because the C code generation relies on these names to produce global variables (e.g., `ui_BootProgressBar`), which the `LVGLDisplayModule` references programmatically.

## Managing Screen Transitions and Group Visibility
- The boot screen handles an initial fade-in via the `ui_BootFadeOverlay`.
- The system screen consolidates distinct operational states. The firmware manages this by removing the `OBJ_FLAG_HIDDEN` flag from the active group container (e.g., `ui_SystemSetupGroup`) and adding it to the others.

## Round Display Safe Zone Rules
Due to the circular nature of the display (240px diameter), the corners are clipped.
- The **Safe Zone** is a 170px diameter circle in the center.
- Most widgets should remain within a radius of ≤85px from the center. 
- Wide horizontal elements should be avoided to prevent them from being cut off by the display edge.

## Customizing the Boot Logo
The boot screen uses `assets/cblabs_180x58.png` (a compact 180×58 logo sized for the
170px round safe zone). The full-resolution source `assets/cblabs.png` is retained for
reference/re-scaling.
To replace it with your own brand logo:
1. Add your logo PNG (24-bit with alpha channel, sized to fit the safe zone) to the `assets/` directory.
2. Open SLS and select the `ui_BootLogo` image widget.
3. In the Inspector, update the Image Asset to point to your new logo.

## Forking and Customizing
Consumers can use this project as a starting point. Simply copy the `squareline/crowpanel-128/` folder and begin adding or modifying screens. 
To add a new screen, click the **+** button in the Screen panel of SLS and follow the UI patterns established here.

