# CrowPanel 1.28" Rotary Display SLS Template

This SquareLine Studio (SLS) project provides a complete starter template for the **Elecrow CrowPanel 1.28" Round Display** (ESP32-S3).

## What This Project Contains
This project consists of 3 distinct screens:
- **Boot Screen** (`Screen_Boot`): A splash screen with a logo, title, progress bar, and status label. It features a fade-in animation overlay.
- **System Screen** (`Screen_System`): A consolidated screen containing multiple hidden groups (Error, WiFi Wizard, OTA Progress, and Interstitial) that can be shown dynamically by the firmware.
- **Diagnostics Screen** (`Screen_Diag`): A runtime consumer template for sensor readouts, featuring an edge arc, large value readout, roller for info, and a touch test button.

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
- The system screen consolidates distinct operational states. The firmware manages this by removing the `OBJ_FLAG_HIDDEN` flag from the active group container (e.g., `ui_SystemWizardGroup`) and adding it to the others.

## Round Display Safe Zone Rules
Due to the circular nature of the display (240px diameter), the corners are clipped.
- The **Safe Zone** is a 170px diameter circle in the center.
- Most widgets should remain within a radius of ≤85px from the center. 
- Wide horizontal elements should be avoided to prevent them from being cut off by the display edge.

## Customizing the Boot Logo
The default placeholder logo is provided as `assets/cblabs.png`. 
To replace it with your own brand logo:
1. Replace `cblabs.png` in the `assets/` directory with a 24-bit PNG with alpha channel.
2. Open SLS and select the `ui_BootLogo` image widget.
3. In the Inspector, update the Image Asset to point to your new logo.

## Forking and Customizing
Consumers can use this project as a starting point. Simply copy the `squareline/crowpanel-128/` folder and begin adding or modifying screens. 
To add a new screen, click the **+** button in the Screen panel of SLS and follow the UI patterns established here.

