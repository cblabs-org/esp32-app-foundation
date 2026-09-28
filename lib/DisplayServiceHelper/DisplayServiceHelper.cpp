/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: lib/DisplayServiceHelper/DisplayServiceHelper.cpp
 * Description: DisplayServiceHelper implementation.
 *
 */

#include "DisplayServiceHelper.h"
#include "Application.hpp"
#include "interfaces/IDisplayModule.hpp"
#include <PsychicHttp.h>
#include "../../modules/MCPServer/MCPToolRegistry.hpp"
#include "utils/BufferStream.h"
#include <mbedtls/base64.h>
#include <vector>
#include <cstring>

DisplayServiceHelper::DisplayServiceHelper(IDisplayModule* display)
    : _display(display), _saveCallback(nullptr), _saveCtx(nullptr), _isOtaActive(nullptr), _otaCtx(nullptr) {}

void DisplayServiceHelper::setSaveCallback(SaveCallback callback, void* ctx) {
    _saveCallback = callback;
    _saveCtx = ctx;
}

void DisplayServiceHelper::registerBrightnessRoute(PsychicHttpServer& server) {
    server.on("/api/display/brightness", HTTP_POST,
        [this](PsychicRequest *request, PsychicResponse* response) {
            const char* body = request->bodyCStr();
            if (!body || strlen(body) == 0) {
                response->send(400, "application/json", "{\"error\":\"body_required\"}");
                return ESP_FAIL;
            }
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, body);
            if (err || !doc["value"].is<int>()) {
                response->send(400, "application/json", "{\"error\":\"Invalid JSON or missing 'value'\"}");
                return ESP_FAIL;
            }

            int value = doc["value"].as<int>();
            if (value < 0) value = 0;
            if (value > 255) value = 255;

            _display->setBrightness(static_cast<uint8_t>(value));
            if (_saveCallback) _saveCallback(_saveCtx);

            response->send(200, "application/json", "{\"status\":\"ok\"}");
            return ESP_OK;
        }
    );
}

bool DisplayServiceHelper::capturePNGToStream(BufferStream& stream) {
    _executor.begin();
    if (_executor.available()) {
        bool ok = false;
        bool runOk = _executor.run([this, &stream, &ok]() {
            ok = _display->capturePNG(stream);
        }, 5000);

        if (runOk) {
            size_t hwmark = _executor.stackHighWaterMarkBytes();
            ESP_LOGI("DisplayService", "PNG Worker stack high water mark: %lu bytes (expected peak ~10KB)", (unsigned long)hwmark);
        }
        return runOk && ok;
    } else {
        return _display->captureStoredPNG(stream);
    }
}

void DisplayServiceHelper::registerScreenshotRoute(PsychicHttpServer& server) {
    server.on("/api/display/screenshot", HTTP_GET, [this](PsychicRequest* request, PsychicResponse* response) {
        if (isOtaActive()) {
            response->send(503, "application/json", "{\"error\":\"busy_during_update\"}");
            return ESP_FAIL;
        }

        BufferStream stream;
        bool ok = capturePNGToStream(stream);

        if (!ok || stream.buffer.empty()) {
            response->send(500, "application/json", "{\"error\":\"Screenshot capture failed\"}");
            return ESP_FAIL;
        }

        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");
        response->addHeader("X-Display-Width", String(_display->getDisplayWidth()).c_str());
        response->addHeader("X-Display-Height", String(_display->getDisplayHeight()).c_str());

        response->send(200, "image/png", stream.buffer.data(), stream.buffer.size());
        return ESP_OK;
    });
}

void DisplayServiceHelper::populateCommonStatus(JsonDocument& doc) {
    doc["width"]      = _display->getDisplayWidth();
    doc["height"]     = _display->getDisplayHeight();
    doc["brightness"] = _display->getBrightness();
    doc["dimmed"]     = _display->isDimmed();
    doc["sleeping"]   = _display->isSleeping();
}

void DisplayServiceHelper::registerCaptureTool(MCPToolRegistry& registry) {
    JsonDocument captureSchema;
    captureSchema["type"] = "object";
    captureSchema["properties"].to<JsonObject>();

    registry.addTool(
        "capture_display",
        "Capture current display buffer as a base64-encoded PNG image",
        captureSchema,
        [](void* ctx, JsonObject& params, JsonObject& result) {
            auto* self = static_cast<DisplayServiceHelper*>(ctx);
            (void)params;

            if (self->isOtaActive()) {
                result["error"] = "busy_during_update";
                return;
            }

            BufferStream stream;
            if (!self->capturePNGToStream(stream) || stream.buffer.empty()) {
                result["error"] = "Failed to capture display buffer";
                return;
            }

            size_t olen = 0;
            mbedtls_base64_encode(nullptr, 0, &olen, stream.buffer.data(), stream.buffer.size());
            if (olen == 0) {
                result["error"] = "Base64 encoding length calculation failed";
                return;
            }

            std::vector<unsigned char> b64buf(olen);
            int ret = mbedtls_base64_encode(b64buf.data(), b64buf.size(), &olen,
                                            stream.buffer.data(), stream.buffer.size());
            if (ret != 0) {
                result["error"] = "Base64 encoding failed";
                return;
            }
            b64buf.push_back(0); // Ensure null termination for ArduinoJson

            result["format"]   = "png";
            result["width"]    = self->_display->getDisplayWidth();
            result["height"]   = self->_display->getDisplayHeight();
            result["image"]    = reinterpret_cast<char*>(b64buf.data());
            result["encoding"] = "base64";
        },
        this,  // context: DisplayServiceHelper*
        /* mutatesState = */ false
    );
}

void DisplayServiceHelper::populateCommonMCPStatus(JsonObject& result) {
    result["width"]      = _display->getDisplayWidth();
    result["height"]     = _display->getDisplayHeight();
    result["brightness"] = _display->getBrightness();
    result["dimmed"]     = _display->isDimmed();
    result["sleeping"]   = _display->isSleeping();
}
