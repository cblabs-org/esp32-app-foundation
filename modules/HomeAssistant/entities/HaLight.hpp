#pragma once
#include "IHaEntity.hpp"

class HaLight : public IHaEntity {
private:
    const char* _objectId;
    const char* _name;
    bool _state = false;
    uint8_t _brightness = 255;
    uint8_t _color[3] = {255, 255, 255};
    std::string _effect = "";
    
    bool _supportsBrightness = false;
    bool _supportsColor = false;
    bool _supportsEffect = false;
    
    using CommandFn = void(*)(bool state, uint8_t brightness, uint8_t r, uint8_t g, uint8_t b, const char* effect, void* ctx);
    CommandFn _commandCallback = nullptr;
    void* _commandCtx = nullptr;

public:
    HaLight(const char* objectId, const char* name)
        : _objectId(objectId), _name(name) {}

    void setSupportsBrightness(bool supports) { _supportsBrightness = supports; }
    void setSupportsColor(bool supports) { _supportsColor = supports; }
    void setSupportsEffect(bool supports) { _supportsEffect = supports; }

    void onCommand(CommandFn callback, void* ctx = nullptr) {
        _commandCallback = callback;
        _commandCtx = ctx;
    }

    void setState(bool state, uint8_t brightness = 255, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, const char* effect = nullptr) {
        _state = state;
        _brightness = brightness;
        _color[0] = r; _color[1] = g; _color[2] = b;
        if (effect) _effect = effect; else _effect = "";
    }

    std::string getObjectId() const override { return _objectId; }
    const char* getEntityType() const override { return "light"; }
    bool isCommandable() const override { return true; }

    void buildDiscoveryPayload(JsonObject& payload) const override {
        payload["name"] = _name;
        payload["schema"] = "json";
        
        if (_supportsBrightness) {
            payload["brightness"] = true;
        }
        if (_supportsColor) {
            payload["color_mode"] = true;
            JsonArray colorModes = payload["supported_color_modes"].to<JsonArray>();
            colorModes.add("rgb");
        }
        if (_supportsEffect) {
            payload["effect"] = true;
        }
    }

    std::string getState() const override {
        JsonDocument doc;
        doc["state"] = _state ? "ON" : "OFF";
        if (_supportsBrightness) doc["brightness"] = _brightness;
        if (_supportsColor) {
            JsonObject color = doc["color"].to<JsonObject>();
            color["r"] = _color[0];
            color["g"] = _color[1];
            color["b"] = _color[2];
        }
        if (_supportsEffect && !_effect.empty()) {
            doc["effect"] = _effect;
        }
        std::string output;
        serializeJson(doc, output);
        return output;
    }

    void handleCommand(const std::string& topic, const std::string& payload) override {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (!err) {
            if (doc["state"].is<const char*>()) {
                std::string s = doc["state"].as<std::string>();
                _state = (s == "ON" || s == "on");
            }
            if (doc["brightness"].is<uint8_t>()) {
                _brightness = doc["brightness"];
            }
            if (doc["color"].is<JsonObject>()) {
                JsonObject color = doc["color"];
                if (color["r"].is<uint8_t>()) _color[0] = color["r"];
                if (color["g"].is<uint8_t>()) _color[1] = color["g"];
                if (color["b"].is<uint8_t>()) _color[2] = color["b"];
            }
            if (doc["effect"].is<const char*>()) {
                _effect = doc["effect"].as<std::string>();
            }
            if (_commandCallback) {
                _commandCallback(_state, _brightness, _color[0], _color[1], _color[2], _effect.empty() ? nullptr : _effect.c_str(), _commandCtx);
            }
        }
    }
};
