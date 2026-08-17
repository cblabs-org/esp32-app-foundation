#pragma once

#include <ArduinoJson.h>
#include <functional>
#include "DeepStackExecutor.h"

class IDisplayModule;
class PsychicHttpServer;
class MCPToolRegistry;
class BufferStream;

class DisplayServiceHelper {
public:
    explicit DisplayServiceHelper(IDisplayModule* display);

    /// Set optional callback to persist config after mutations (brightness, etc.)
    void setSaveCallback(std::function<void()> callback);

    /// Set OTA active predicate to gate screenshots during updates
    void setOtaPredicate(std::function<bool()> isOtaActive) { _isOtaActive = isOtaActive; }

    // Route helpers
    void registerBrightnessRoute(PsychicHttpServer& server);
    void registerScreenshotRoute(PsychicHttpServer& server);
    void populateCommonStatus(JsonDocument& doc);

    // MCP helpers
    void registerCaptureTool(MCPToolRegistry& registry);
    void populateCommonMCPStatus(JsonObject& result);

#ifdef UNIT_TEST
    /// Test-only accessor for the executor (allows setting _stubAvailable)
    DeepStackExecutor& getExecutor() { return _executor; }
#endif

private:
    IDisplayModule* _display;
    std::function<void()> _saveCallback;
    std::function<bool()> _isOtaActive = [](){ return false; };

    // Start with 16384, we will measure this and update it.
    // The PA will measure the peak high-water-mark using the ESP_LOGI output
    // and adjust this size in a future step if necessary.
    DeepStackExecutor _executor{16384, "png_worker"};

    /**
     * @brief Shared capture logic: routes PNG generation through DeepStackExecutor
     *        with OTA gate and stored-PNG fallback.
     * @param stream The BufferStream to write the PNG data into.
     * @return true if capture succeeded, false otherwise.
     */
    bool capturePNGToStream(BufferStream& stream);
};
