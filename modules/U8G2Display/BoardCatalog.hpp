/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/BoardCatalog.hpp
 * Description: Thin accessor struct that holds typed non-owning references to
 *              the user-surfaced system boards owned by U8G2DisplayModule.
 *              Provides the `display.boards.*` sub-object API described in
 *              design spec §7.8.1. Callers configure boards via their concrete
 *              types rather than the IDisplayBoard interface.
 *
 *              The BootBoard and ScreensaverBoard remain excluded — BootBoard
 *              is only used during the bootstrap phase (before begin()), and
 *              the screensaver is display-intrinsic (driven by inactivity timer).
 *
 * Exported Classes:
 * - BoardCatalog: Thin accessor struct holding references to all display boards
 *   - boot:               Reference to the BootBoard instance
 *   - configuringStatus:  Reference to the ConfiguringStatusBoard instance
 *   - genericHelper:      Reference to the GenericGateHelperBoard instance
 *   - wifiWizard:         Reference to the WiFiWizardBoard instance
 *   - otaProgress:        Reference to the OTAProgressBoard instance
 *   - error:              Reference to the ErrorBoard instance
 */

#pragma once

#include "boards/BootBoard.hpp"
#include "boards/ConfiguringStatusBoard.hpp"
#include "boards/GenericGateHelperBoard.hpp"
#include "boards/WiFiWizardBoard.hpp"
#include "boards/OTAProgressBoard.hpp"
#include "boards/ErrorBoard.hpp"

/**
 * @brief Thin accessor struct providing typed references to caller-driven boards.
 *
 * Exposes the `display.boards.*` sub-object API (design spec §7.8.1). All
 * member references are non-owning — the boards themselves are owned as member
 * variables of U8G2DisplayModule and outlive this struct.
 *
 * Boards included here are the complete set of display boards in the system:
 * - BootBoard              — driven via IStatusReporter during bootstrap
 * - ConfiguringStatusBoard — App pushes via `display.showBase()` during CONFIGURING
 * - WiFiWizardBoard        — App calls `display.boards.wifiWizard.configure(hostname)`
 * - OTAProgressBoard       — OTA module calls `display.boards.otaProgress.setProgress()`
 * - ErrorBoard             — Any module calls `display.boards.error.showError()`
 *
 * Excluded boards:
 * - ScreensaverBoard — display-intrinsic, managed by the screensaver sub-object
 *
 * @note Because this struct holds references, it is not default-constructible
 *       and not copy-assignable. It must be initialised in the U8G2DisplayModule
 *       member-initializer list.
 */
struct BoardCatalog {
    BootBoard&              boot;                ///< Boot splash + loading screen (driven via IStatusReporter)
    ConfiguringStatusBoard& configuringStatus;   ///< Gate progress during CONFIGURING phase
    GenericGateHelperBoard& genericHelper;        ///< Generic fallback helper for config gates
    WiFiWizardBoard&        wifiWizard;           ///< WiFi setup wizard screen
    OTAProgressBoard&       otaProgress;          ///< OTA firmware update progress screen
    ErrorBoard&             error;                ///< Error message display screen

    /**
     * @brief Construct a BoardCatalog bound to the owning module's board instances.
     *
     * All parameters are non-const lvalue references. The BoardCatalog does
     * not own these objects — they remain owned by U8G2DisplayModule.
     *
     * @param boot              Reference to the module's owned BootBoard.
     * @param configuringStatus Reference to the module's owned ConfiguringStatusBoard.
     * @param genericHelper     Reference to the module's owned GenericGateHelperBoard.
     * @param wifiWizard        Reference to the module's owned WiFiWizardBoard.
     * @param otaProgress       Reference to the module's owned OTAProgressBoard.
     * @param error             Reference to the module's owned ErrorBoard.
     */
    BoardCatalog(BootBoard& boot,
                 ConfiguringStatusBoard& configuringStatus,
                 GenericGateHelperBoard& genericHelper,
                 WiFiWizardBoard& wifiWizard,
                 OTAProgressBoard& otaProgress,
                 ErrorBoard& error)
        : boot(boot)
        , configuringStatus(configuringStatus)
        , genericHelper(genericHelper)
        , wifiWizard(wifiWizard)
        , otaProgress(otaProgress)
        , error(error) {}
};
