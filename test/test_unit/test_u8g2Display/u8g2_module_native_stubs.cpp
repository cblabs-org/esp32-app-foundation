/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Module: test/test_unit/test_u8g2Display/u8g2_module_native_stubs.cpp
 * Description: Stub implementations of U8G2DisplayModule methods for native
 *              host-test builds. The real U8G2Display.cpp is excluded
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
#include "U8G2Display.hpp"
#include "EventBus.hpp"
#include "events/AppConfiguringPayload.hpp"
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
        _eventBus->subscribe("precondition.health", _onPreconditionHealth, this);
    }
}

void U8G2DisplayModule::_onPreconditionHealth(const char* /*topic*/, const void* payload, void* ctx) {
    auto* self = static_cast<U8G2DisplayModule*>(ctx);
    if (!self || !payload) return;
    const auto* p = static_cast<const PreconditionHealthPayload*>(payload);
    self->boards.configuringStatus.updatePrecondition(*p);
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

void U8G2DisplayModule::registerTabs(ITabRegistry& registry) {
    registry.addTab("Display", nullptr, 50, getTabIcon(), "/display.js");
    registry.addTab("Settings",   "Display", 10);
    registry.addTab("Screenshot", "Display", 20);
    registry.addTab("Carousel",   "Display", 30, nullptr, "/carousel.js");
}

// ── IMCPContributor ──────────────────────────────────────────────────────────

void U8G2DisplayModule::registerMCPTools(MCPToolRegistry& registry) {
    (void)registry;
}
