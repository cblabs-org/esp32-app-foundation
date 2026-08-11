#pragma once

#include <ArduinoJson.h>
#include <functional>

class IDisplayModule;
class PsychicHttpServer;
class MCPToolRegistry;

class DisplayServiceHelper {
public:
    explicit DisplayServiceHelper(IDisplayModule* display);

    /// Set optional callback to persist config after mutations (brightness, etc.)
    void setSaveCallback(std::function<void()> callback);

    // Route helpers
    void registerBrightnessRoute(PsychicHttpServer& server);
    void registerScreenshotRoute(PsychicHttpServer& server);
    void populateCommonStatus(JsonDocument& doc);

    // MCP helpers
    void registerCaptureTool(MCPToolRegistry& registry);
    void populateCommonMCPStatus(JsonObject& result);

private:
    IDisplayModule* _display;
    std::function<void()> _saveCallback;
};
