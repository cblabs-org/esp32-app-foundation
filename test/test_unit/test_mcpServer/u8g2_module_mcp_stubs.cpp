/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: test/test_unit/test_u8g2Display/u8g2_module_native_stubs.cpp
 * Description: Stub implementations of U8G2DisplayModule methods for native
 *              host-test builds. The real U8G2DisplayModule.cpp is excluded
 *              on native via exclude_on_native.py because it pulls hardware
 *              dependencies (PsychicHttp, SPI, FreeRTOS task creation).
 *
 *              These stubs provide behaviorally-correct implementations for
 *              the subset of methods exercised by the test suite:
 *              - tick():            delegates to carousel engine
 *              - serializeConfig(): emits expected JSON keys
 *              - applyConfig():     reads JSON keys into member variables
 *              - resetConfig():     restores factory defaults
 *              - showModal():       tracks the active modal board
 *              - visibleBoard():    returns modal or carousel current board
 *              - begin():           stores EventBus reference
 */

#include "NativeU8G2DisplayType.h"
#include "U8G2DisplayModule.hpp"
#include "EventBus.hpp"
#include "events/AppConfiguringPayload.hpp"
#include "MCPToolRegistry.hpp"
#include "ConfigManager.hpp"
#include <U8g2lib.h>
#include <cstring>

// ── Constructor ──────────────────────────────────────────────────────────────

U8G2DisplayModule::U8G2DisplayModule(const BootBranding* branding,
                                     const U8G2BootLogo* bootLogo,
                                     IU8G2SystemScreens* screens)
    : _branding(branding)
    , _bootLogo(bootLogo)
    , _screens(screens)
{
}

// ── Lifecycle ────────────────────────────────────────────────────────────────

void U8G2DisplayModule::begin(EventBus* eventBus) {
    _eventBus = eventBus;
    if (_eventBus) {
        _eventBus->subscribe("app.configuring", _onAppConfiguring, this);
    }
}

void U8G2DisplayModule::_onAppConfiguring(const char* topic, const void* payload, void* ctx) {
    (void)topic;
    auto* self = static_cast<U8G2DisplayModule*>(ctx);
    if (!self || !payload) return;

    const auto* p = static_cast<const AppConfiguringPayload*>(payload);

    // Always clear any existing modal
    self->_modal = nullptr;

    // Always set base board to configuring status
    self->_baseBoard = &self->_configuringStatusBoard;

    // Non-blocking statuses (Transient, Blocked, Met, Unknown) are tracked
    // on the base ConfiguringStatusBoard — do NOT push a modal board.
    if (p->severity != Severity::NeedsUser) {
        return;
    }

    // Tier 1: Consumer override via IU8G2SystemScreens
    if (self->_screens) {
        IDisplayBoard* custom = self->_screens->gateHelperBoard(p);
        if (custom) {
            self->showModal(custom);
            return;
        }
    }
    // Tier 2: Built-in WiFi wizard
    if (p->preconditionName && strcmp(p->preconditionName, PreconditionNames::WIFI_STATION) == 0) {
        self->_wifiWizard.configure(p->hostname, p->deviceIP);
        self->showModal(&self->_wifiWizard);
        return;
    }
    // Tier 3: Generic gate helper fallback
    self->showModal(&self->_genericHelperBoard);
}

void U8G2DisplayModule::tick(uint32_t currentMillis) {
    // Drive the carousel engine — tests depend on rotation, hold, and eviction.
    _carousel.tick(currentMillis);

    // Auto-clear expired modals
    if (_modal && _modal->isExpired()) {
        _modal = nullptr;
    }
}

// ── IStatusReporter ──────────────────────────────────────────────────────────

void U8G2DisplayModule::reportStatus(const char* message, float progress) {
    _bootBoard.setStatus(message, progress);
    showModal(&_bootBoard);
}

void U8G2DisplayModule::reportError(const char* errorMessage) {
    // Check for consumer-provided custom error board
    if (_screens) {
        IDisplayBoard* custom = _screens->errorBoard();
        if (custom) {
            showModal(custom);
            return;
        }
    }
    _error.showError(errorMessage);
    showModal(&_error);
}

void U8G2DisplayModule::clearStatus() {
    _bootBoard.setComplete();
    _modal = nullptr;
}

// ── IConfigurable ────────────────────────────────────────────────────────────

void U8G2DisplayModule::applyConfig(const JsonObject& section) {
    if (section["brightness"].is<JsonVariant>())
        _brightness = section["brightness"].as<uint8_t>();
    if (section["flip"].is<JsonVariant>())
        _flipScreen = section["flip"].as<bool>();
    if (section["screensaverEnabled"].is<JsonVariant>())
        _screensaver.setEnabled(section["screensaverEnabled"].as<bool>());
    if (section["dimTimeout"].is<JsonVariant>())
        _screensaver.setDimTimeout(section["dimTimeout"].as<unsigned long>());
    if (section["blankTimeout"].is<JsonVariant>())
        _screensaver.setBlankTimeout(section["blankTimeout"].as<unsigned long>());
    if (section["rotationInterval"].is<JsonVariant>())
        _carousel.setInterval(section["rotationInterval"].as<unsigned long>());

    // Migration: legacy sleepTimeout → dim/blank split (dim = half, blank = full)
    if (section["sleepTimeout"].is<JsonVariant>() &&
        !section["dimTimeout"].is<JsonVariant>() &&
        !section["blankTimeout"].is<JsonVariant>()) {
        unsigned long sleepMs = section["sleepTimeout"].as<unsigned long>();
        _screensaver.setDimTimeout(sleepMs / 2);
        _screensaver.setBlankTimeout(sleepMs);
    }
}

void U8G2DisplayModule::serializeConfig(JsonObject& section) const {
    section["brightness"] = _brightness;
    section["flip"] = _flipScreen;
    section["screensaverEnabled"] = _screensaver.isEnabled();
    section["dimTimeout"] = _screensaver.getDimTimeout();
    section["blankTimeout"] = _screensaver.getBlankTimeout();
    section["rotationInterval"] = _carousel.getInterval();
}

void U8G2DisplayModule::resetConfig() {
    _brightness = 255;
    _flipScreen = false;
    _screensaver.setEnabled(true);
    _screensaver.setDimTimeout(30000);
    _screensaver.setBlankTimeout(60000);
    _carousel.setInterval(5000);
}

// ── Display control ──────────────────────────────────────────────────────────

void U8G2DisplayModule::showModal(IDisplayBoard* board) {
    _modal = board;
    if (_modal) {
        _modal->onActivate();
    }
}

IDisplayBoard* U8G2DisplayModule::visibleBoard() const {
    if (_modal && !_modal->isExpired()) return _modal;
    if (_baseBoard) return _baseBoard;
    return _carousel.currentBoard();
}

void U8G2DisplayModule::setBrightness(uint8_t brightness) {
    _brightness = brightness;
}

void U8G2DisplayModule::setFlipScreen(bool flip) {
    _flipScreen = flip;
}

void U8G2DisplayModule::setSleepTimeout(unsigned long timeoutMs) {
    _screensaver.setBlankTimeout(timeoutMs);
}

void U8G2DisplayModule::setRotationInterval(unsigned long intervalMs) {
    _carousel.setInterval(intervalMs);
}

// ── ITickable ────────────────────────────────────────────────────────────────

bool U8G2DisplayModule::beginEarly() {
    return true;
}

// ── IDisplayModule ───────────────────────────────────────────────────────────

void U8G2DisplayModule::resetActivity() {
}

uint16_t U8G2DisplayModule::getDisplayWidth() const {
    return _displayWidth;
}

uint16_t U8G2DisplayModule::getDisplayHeight() const {
    return _displayHeight;
}

bool U8G2DisplayModule::isDimmed() const {
    return false;
}

bool U8G2DisplayModule::isSleeping() const {
    return false;
}

bool U8G2DisplayModule::capturePNG(Print& sink) {
    (void)sink;
    return false;
}

void U8G2DisplayModule::showSystemState(const SystemDisplayParams& params) {
    (void)params;
}

// ── IWebContributor ──────────────────────────────────────────────────────────

void U8G2DisplayModule::registerRoutes(PsychicHttpServer& server) {
    (void)server;
}

const char* U8G2DisplayModule::getTabIcon() const {
    return nullptr;
}

// ── IMCPContributor ──────────────────────────────────────────────────────────

void U8G2DisplayModule::registerMCPTools(MCPToolRegistry& registry) {

    _serviceHelper.registerCaptureTool(registry);

    registry.addTool(
        "display_status",
        "Get current display state including brightness, active board, screensaver status, and timeout configuration",
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<U8G2DisplayModule*>(ctx);
            (void)params;

            self->_serviceHelper.populateCommonMCPStatus(result);

            result["flip"]       = self->_flipScreen;

            IDisplayBoard* active = self->visibleBoard();
            result["activeBoard"] = (active != nullptr) ? active->getBoardName() : "none";

            JsonObject ss = result["screensaver"].to<JsonObject>();
            ss["enabled"]      = self->_screensaver.isEnabled();

            if (self->_screensaver.isBlanked()) {
                ss["state"] = "blanked";
            } else if (self->_screensaver.isDimmed()) {
                ss["state"] = "dimmed";
            } else {
                ss["state"] = "active";
            }

            ss["dimTimeout"]   = self->_screensaver.getDimTimeout();
            ss["blankTimeout"] = self->_screensaver.getBlankTimeout();

            JsonObject car = result["carousel"].to<JsonObject>();
            car["boardCount"]        = self->_carousel.boardCount();
            car["rotationInterval"]  = self->_carousel.getInterval();
            car["suspended"]         = self->_carousel.isSuspended();
        },
        this,
        false
    );

    JsonDocument controlSchema;
    controlSchema["type"] = "object";
    JsonObject props = controlSchema["properties"].to<JsonObject>();

    props["action"]["type"]        = "string";
    props["action"]["description"] = "Immediate action: 'wake' to reset inactivity, 'sleep' to force screensaver";
    JsonArray actionEnum = props["action"]["enum"].to<JsonArray>();
    actionEnum.add("wake");
    actionEnum.add("sleep");

    props["brightness"]["type"]        = "integer";
    props["brightness"]["description"] = "Display brightness level (0-255)";
    props["brightness"]["minimum"]     = 0;
    props["brightness"]["maximum"]     = 255;

    props["screensaverEnabled"]["type"]        = "boolean";
    props["screensaverEnabled"]["description"] = "Enable or disable the screensaver";

    props["dimTimeout"]["type"]        = "integer";
    props["dimTimeout"]["description"] = "Milliseconds of inactivity before dimming";

    props["blankTimeout"]["type"]        = "integer";
    props["blankTimeout"]["description"] = "Milliseconds of inactivity before blanking";

    registry.addTool(
        "display_control",
        "Control display settings: wake/sleep, enable/disable screensaver, set brightness and timeouts",
        controlSchema,
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<U8G2DisplayModule*>(ctx);
            bool needsPersist = false;

            if (params["action"].is<const char*>()) {
                const char* action = params["action"].as<const char*>();
                if (strcmp(action, "wake") == 0) {
                    self->resetActivity();
                } else if (strcmp(action, "sleep") == 0) {
                    self->_screensaver.forceTimeout();
                }
            }

            if (params["brightness"].is<int>()) {
                int val = params["brightness"].as<int>();
                if (val < 0) val = 0;
                if (val > 255) val = 255;
                self->setBrightness(static_cast<uint8_t>(val));
                needsPersist = true;
            }

            if (params["screensaverEnabled"].is<bool>()) {
                self->_screensaver.setEnabled(params["screensaverEnabled"].as<bool>());
                needsPersist = true;
            }

            if (params["dimTimeout"].is<int>()) {
                self->_screensaver.setDimTimeout(
                    static_cast<unsigned long>(params["dimTimeout"].as<int>()));
                needsPersist = true;
            }

            if (params["blankTimeout"].is<int>()) {
                self->_screensaver.setBlankTimeout(
                    static_cast<unsigned long>(params["blankTimeout"].as<int>()));
                needsPersist = true;
            }

            if (needsPersist && self->_configManager != nullptr) {
                self->_configManager->saveModuleConfig(self);
            }

            result["status"] = "ok";
        },
        this,
        true
    );
}
