/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/U8G2Display/BoardCatalog.hpp
 * Description: Thin accessor struct that holds typed non-owning references to
 *              the three user-surfaced system boards owned by U8G2DisplayModule.
 *              Provides the `display.boards.*` sub-object API described in
 *              design spec §7.8.1. Callers configure boards via their concrete
 *              types rather than the IDisplayBoard interface.
 *
 *              System-managed boards (BootBoard, ConfiguringStatusBoard) and
 *              the display-intrinsic screensaver are excluded per §7.8.4.
 *
 * Exported Classes:
 * - BoardCatalog: Thin accessor struct holding references to user-surfaced boards
 *   - wifiWizard:  Reference to the module's owned WiFiWizardBoard instance
 *   - otaProgress: Reference to the module's owned OTAProgressBoard instance
 *   - error:       Reference to the module's owned ErrorBoard instance
 */

#pragma once

#include "boards/WiFiWizardBoard.hpp"
#include "boards/OTAProgressBoard.hpp"
#include "boards/ErrorBoard.hpp"

/**
 * @brief Thin accessor struct providing typed references to user-surfaced system boards.
 *
 * Exposes the `display.boards.*` sub-object API (design spec §7.8.1). All three
 * member references are non-owning — the boards themselves are owned as member
 * variables of U8G2DisplayModule and outlive this struct.
 *
 * Per §7.8.4, only caller-driven boards appear here:
 * - WiFiWizardBoard  — app calls `display.boards.wifiWizard.configure(hostname)`
 * - OTAProgressBoard — OTA module calls `display.boards.otaProgress.setProgress()`
 * - ErrorBoard       — any module calls `display.boards.error.showError()`
 *
 * System-managed boards (BootBoard, ConfiguringStatusBoard, ScreensaverBoard)
 * are excluded because they are driven internally by the module, not by callers.
 *
 * @note Because this struct holds references, it is not default-constructible
 *       and not copy-assignable. It must be initialised in the U8G2DisplayModule
 *       member-initializer list.
 */
struct BoardCatalog {
    WiFiWizardBoard&  wifiWizard;    ///< WiFi setup wizard screen (CONFIGURING phase)
    OTAProgressBoard& otaProgress;   ///< OTA firmware update progress screen
    ErrorBoard&       error;         ///< Error message display screen

    /**
     * @brief Construct a BoardCatalog bound to the owning module's board instances.
     *
     * All three parameters are non-const lvalue references. The BoardCatalog does
     * not own these objects — they remain owned by U8G2DisplayModule.
     *
     * @param wifiWizard   Reference to the module's owned WiFiWizardBoard.
     * @param otaProgress  Reference to the module's owned OTAProgressBoard.
     * @param error        Reference to the module's owned ErrorBoard.
     */
    BoardCatalog(WiFiWizardBoard& wifiWizard,
                 OTAProgressBoard& otaProgress,
                 ErrorBoard& error)
        : wifiWizard(wifiWizard)
        , otaProgress(otaProgress)
        , error(error) {}
};
