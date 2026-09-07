/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: include/AppFoundation.h
 * Description: Single public include façade for the ESP32 App Foundation
 *              framework. Application code and downstream app repos should
 *              #include only this file to access all framework types and
 *              interfaces. This prevents direct coupling to internal paths.
 *
 * Exported (via transitively included headers):
 * - Application:                The framework hub class (register modules, lifecycle)
 * - FrameworkConfig:            Boot-phase configuration struct
 * - AppState:                   Lifecycle phase enum (BOOTING → RUNNING)
 * - ITickable:                  Interface — periodic tick contract
 * - IConfigurable:              Interface — ConfigManager gateway config contract
 * - IDataSource:                Interface — background data fetch contract
 * - IStatusReporter:            Interface — display status overlay contract
 * - IWebContributor:            Interface — REST route registration contract
 * - ITabRegistry:               Interface — navigation tab registration contract
 * - IMCPContributor:            Interface — MCP tool registration contract
 * - IPreconditionContributor:   Interface — precondition contributor contract
 * - IPreconditionRegistry:      Interface — precondition registration contract
 * - IPrecondition:              Types and enums for precondition system
 * - IOTAVerifier:               Interface — OTA firmware signature verification contract
 * - ISanitizer:                 Interface — log sanitization contract
 * - IBootReporter:              Interface — boot progress reporting contract
 * - IDisplayModule:             Interface — unified display module contract
 * - UpdateStatus:               Enum — data fetch result codes
 * - PriorityTier:               Enum — DataManager scheduling tier
 * - FetchContext:               Transport context and deadline token for fetches
 * - DeadlineToken:              Cooperative cancellation token
 * - SourceHealth:               Point-in-time health snapshot for a data source
 * - SourceHealthState:          Health state enum (Unknown, Healthy, Degraded, Quarantined, Suspended)
 * - DataSourceHealthPayload:    EventBus payload for data source health transitions
 * - PreconditionHealthPayload:  EventBus payload for precondition health transitions
 * - AppConfiguringPayload:      EventBus payload for app configuring state
 */

#pragma once

// Version — must be first; derives BUILD_VERSION from build flags
#include "build_version.h"

// Core framework headers — order matters for dependency resolution
#include "types/AppState.hpp"
#include "types/FrameworkConfig.hpp"

// Interface contracts
#include "interfaces/ITickable.hpp"
#include "interfaces/IConfigurable.hpp"
#include "interfaces/IDataSource.hpp"
#include "interfaces/IStatusReporter.hpp"
#include "interfaces/ITabRegistry.hpp"
#include "interfaces/IWebContributor.hpp"
#include "interfaces/IMCPContributor.hpp"
#include "interfaces/IPrecondition.hpp"
#include "interfaces/IPreconditionRegistry.hpp"
#include "interfaces/IPreconditionContributor.hpp"
#include "interfaces/IOTAVerifier.hpp"
#include "interfaces/ISanitizer.hpp"
#include "interfaces/IBootReporter.hpp"
#include "interfaces/IDisplayModule.hpp"

// Event payloads
#include "events/DataSourceHealthPayload.hpp"
#include "events/PreconditionHealthPayload.hpp"
#include "events/AppConfiguringPayload.hpp"

// Application hub — target builds only.
// Native test builds should include only the interfaces and event payloads
// above; the concrete Application and TimeManager classes pull platform
// headers (esp_event.h, WiFi.h) that don't exist on the host.
#ifdef ESP_PLATFORM
#include "../modules/Application/Application.hpp"
#include "../modules/TimeManager/TimeManager.hpp"
#endif
