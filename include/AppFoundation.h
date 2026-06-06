/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: include/AppFoundation.h
 * Description: Single public include façade for the ESP32 App Foundation
 *              framework. Application code and downstream app repos should
 *              #include only this file to access all framework types and
 *              interfaces. This prevents direct coupling to internal paths.
 *
 * Exported (via transitively included headers):
 * - Application:       The framework hub class (register modules, lifecycle)
 * - FrameworkConfig:   Boot-phase configuration struct
 * - AppState:          Lifecycle phase enum (BOOTING → RUNNING)
 * - ITickable:         Interface — periodic tick contract
 * - IConfigurable:     Interface — LittleFS config load/save contract
 * - IDataSource:       Interface — background data fetch contract
 * - IStatusReporter:   Interface — display status overlay contract
 * - IWebContributor:   Interface — REST route registration contract
 * - IMCPContributor:   Interface — MCP tool registration contract
 * - IDisplayBoard:     Interface — U8G2 carousel board render contract
 * - UpdateStatus:      Enum — data fetch result codes
 * - PriorityTier:      Enum — DataManager scheduling tier
 */

#pragma once

// Version — must be first; derives BUILD_VERSION from build flags
#include "build_version.h"

// Core framework headers — order matters for dependency resolution
#include "AppState.hpp"
#include "FrameworkConfig.hpp"

// Interface contracts
#include "interfaces/ITickable.hpp"
#include "interfaces/IConfigurable.hpp"
#include "interfaces/IDataSource.hpp"
#include "interfaces/IStatusReporter.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IMCPContributor.hpp"
#include "interfaces/IDisplayBoard.hpp"

// Application hub
#include "../modules/Application/Application.hpp"
#include "../modules/TimeManager/TimeManager.hpp"
