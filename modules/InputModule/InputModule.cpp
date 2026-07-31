/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/InputModule/InputModule.cpp
 * Description: Implementation of InputModule button event collector and EventBus publisher.
 */

#include "InputModule.hpp"
#include "modules/Application/Application.hpp"
#include "modules/MCPServer/MCPToolRegistry.hpp"
#include <map>

#ifndef UNIT_TEST
#include "iot_button.h"
#else
#include "iot_button.h"
#endif

InputModule::InputModule() = default;

InputModule::~InputModule() {
#ifndef UNIT_TEST
    for (void* handle : _buttonHandles) {
        if (handle) {
            iot_button_delete(static_cast<button_handle_t>(handle));
        }
    }
#endif
    _buttonHandles.clear();

    for (ShimContext* ctx : _shimContexts) {
        delete ctx;
    }
    _shimContexts.clear();

    if (_eventQueue) {
        vQueueDelete(_eventQueue);
        _eventQueue = nullptr;
    }
}

void InputModule::addInput(const BoardInput& input) {
    _pendingInputs.push_back(input);
}

void InputModule::begin(Application& app) {
    _bus = &app.getEventBus();
    if (!_eventQueue) {
        _eventQueue = xQueueCreate(QUEUE_DEPTH, sizeof(InputEvent));
    }

    // Phase 1: Group ADC ladder inputs by pin, process GPIO & Matrix inputs
    std::map<int8_t, std::vector<const BoardInput*>> adcGroups;

    for (const auto& input : _pendingInputs) {
        switch (input.type) {
            case InputType::GPIO:
                createGpioButton(input);
                break;
            case InputType::ADC_LADDER:
                adcGroups[input.adc.adcPin].push_back(&input);
                break;
            case InputType::MATRIX:
                createMatrixButton(input);
                break;
        }
    }

    // Phase 2: Grouped ADC ladder buttons
    for (const auto& pair : adcGroups) {
        createAdcLadder(pair.first, pair.second);
    }
}

void InputModule::createGpioButton(const BoardInput& input) {
    static const InputGesture gestures[] = {
        InputGesture::CLICK,
        InputGesture::DOUBLE_CLICK,
        InputGesture::LONG_PRESS,
        InputGesture::PRESS_DOWN
    };

    button_handle_t btn_handle = nullptr;

#ifndef UNIT_TEST
    button_config_t btn_cfg = {};
    btn_cfg.type = BUTTON_TYPE_GPIO;
    btn_cfg.gpio_button_config.gpio_num = input.gpio.pin;
    btn_cfg.gpio_button_config.active_level = input.gpio.activeLow ? 0 : 1;
    btn_cfg.gpio_button_config.enable_power_save = input.gpio.powerSave;

    button_gpio_config_t gpio_cfg = {
        .gpio_num = input.gpio.pin,
        .active_level = static_cast<uint8_t>(input.gpio.activeLow ? 0 : 1),
        .enable_power_save = input.gpio.powerSave
    };

    esp_err_t err = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn_handle);
    if (err != ESP_OK || !btn_handle) {
        btn_handle = iot_button_create(&btn_cfg);
    }
#endif

    if (btn_handle) {
        _buttonHandles.push_back(btn_handle);
    }

    for (auto gesture : gestures) {
        auto* ctx = new ShimContext{
            .id = input.id,
            .gesture = gesture,
            .queue = _eventQueue,
            .droppedCount = 0
        };
        _shimContexts.push_back(ctx);

#ifndef UNIT_TEST
        if (btn_handle) {
            button_event_t evt = BUTTON_SINGLE_CLICK;
            switch (gesture) {
                case InputGesture::CLICK:        evt = BUTTON_SINGLE_CLICK; break;
                case InputGesture::DOUBLE_CLICK: evt = BUTTON_DOUBLE_CLICK; break;
                case InputGesture::LONG_PRESS:   evt = BUTTON_LONG_PRESS_START; break;
                case InputGesture::PRESS_DOWN:   evt = BUTTON_PRESS_DOWN; break;
            }
            iot_button_register_cb(btn_handle, evt, _buttonShim, ctx);
        }
#endif
    }
}

void InputModule::createAdcLadder(int8_t pin, const std::vector<const BoardInput*>& buttons) {
    static const InputGesture gestures[] = {
        InputGesture::CLICK,
        InputGesture::DOUBLE_CLICK,
        InputGesture::LONG_PRESS,
        InputGesture::PRESS_DOWN
    };

    for (const auto* inputPtr : buttons) {
        if (!inputPtr) continue;
        const auto& input = *inputPtr;
        button_handle_t btn_handle = nullptr;

#ifndef UNIT_TEST
        button_config_t btn_cfg = {};
        btn_cfg.type = BUTTON_TYPE_ADC;
        btn_cfg.adc_button_config.adc_channel = input.adc.adcChannel;
        btn_cfg.adc_button_config.min_voltage = input.adc.minMv;
        btn_cfg.adc_button_config.max_voltage = input.adc.maxMv;

        btn_handle = iot_button_create(&btn_cfg);
#endif

        if (btn_handle) {
            _buttonHandles.push_back(btn_handle);
        }

        for (auto gesture : gestures) {
            auto* ctx = new ShimContext{
                .id = input.id,
                .gesture = gesture,
                .queue = _eventQueue,
                .droppedCount = 0
            };
            _shimContexts.push_back(ctx);

#ifndef UNIT_TEST
            if (btn_handle) {
                button_event_t evt = BUTTON_SINGLE_CLICK;
                switch (gesture) {
                    case InputGesture::CLICK:        evt = BUTTON_SINGLE_CLICK; break;
                    case InputGesture::DOUBLE_CLICK: evt = BUTTON_DOUBLE_CLICK; break;
                    case InputGesture::LONG_PRESS:   evt = BUTTON_LONG_PRESS_START; break;
                    case InputGesture::PRESS_DOWN:   evt = BUTTON_PRESS_DOWN; break;
                }
                iot_button_register_cb(btn_handle, evt, _buttonShim, ctx);
            }
#endif
        }
    }
}

void InputModule::createMatrixButton(const BoardInput& input) {
    static const InputGesture gestures[] = {
        InputGesture::CLICK,
        InputGesture::DOUBLE_CLICK,
        InputGesture::LONG_PRESS,
        InputGesture::PRESS_DOWN
    };

    button_handle_t btn_handle = nullptr;

#ifndef UNIT_TEST
    button_config_t btn_cfg = {};
    btn_cfg.type = BUTTON_TYPE_MATRIX;
    btn_handle = iot_button_create(&btn_cfg);
#endif

    if (btn_handle) {
        _buttonHandles.push_back(btn_handle);
    }

    for (auto gesture : gestures) {
        auto* ctx = new ShimContext{
            .id = input.id,
            .gesture = gesture,
            .queue = _eventQueue,
            .droppedCount = 0
        };
        _shimContexts.push_back(ctx);

#ifndef UNIT_TEST
        if (btn_handle) {
            button_event_t evt = BUTTON_SINGLE_CLICK;
            switch (gesture) {
                case InputGesture::CLICK:        evt = BUTTON_SINGLE_CLICK; break;
                case InputGesture::DOUBLE_CLICK: evt = BUTTON_DOUBLE_CLICK; break;
                case InputGesture::LONG_PRESS:   evt = BUTTON_LONG_PRESS_START; break;
                case InputGesture::PRESS_DOWN:   evt = BUTTON_PRESS_DOWN; break;
            }
            iot_button_register_cb(btn_handle, evt, _buttonShim, ctx);
        }
#endif
    }
}

void InputModule::_buttonShim(void* handle, void* userData) {
    if (!userData) return;
    auto* ctx = static_cast<ShimContext*>(userData);
    if (!ctx->queue) return;
    InputEvent evt = { .id = ctx->id, .gesture = ctx->gesture };
    if (xQueueSend(ctx->queue, &evt, 0) != pdTRUE) {
        ctx->droppedCount++;
    }
}

void InputModule::tick(uint32_t currentMillis) {
    if (!_eventQueue || !_bus) return;
    InputEvent evt;
    int drained = 0;
    while (drained < MAX_EVENTS_PER_TICK &&
           xQueueReceive(_eventQueue, &evt, 0) == pdTRUE) {
        _eventRing[_ringHead] = evt;
        const char* topic = gestureToTopic(evt.gesture);
        _bus->publish(topic, &_eventRing[_ringHead]);
        _ringHead = (_ringHead + 1) % EVENT_RING_SIZE;
        drained++;
    }
}

const char* InputModule::gestureToTopic(InputGesture gesture) {
    switch (gesture) {
        case InputGesture::CLICK:        return "input.click";
        case InputGesture::DOUBLE_CLICK: return "input.double_click";
        case InputGesture::LONG_PRESS:   return "input.long_press";
        case InputGesture::PRESS_DOWN:   return "input.press_down";
        default:                         return "input.unknown";
    }
}

void InputModule::registerMCPTools(MCPToolRegistry& registry) {
    registry.addTool(
        "input_status",
        "Get registered hardware inputs, recent event ring buffer state, and dropped event diagnostics",
        [](void* context, JsonObject& args, JsonObject& result) {
            auto* self = static_cast<InputModule*>(context);
            if (!self) return;
            JsonArray inputs = result["inputs"].to<JsonArray>();
            for (const auto& input : self->_pendingInputs) {
                JsonObject obj = inputs.add<JsonObject>();
                obj["id"] = input.id ? input.id : "";
                switch (input.type) {
                    case InputType::GPIO:       obj["type"] = "GPIO"; break;
                    case InputType::ADC_LADDER: obj["type"] = "ADC_LADDER"; break;
                    case InputType::MATRIX:     obj["type"] = "MATRIX"; break;
                }
            }

            uint32_t totalDropped = 0;
            for (const auto* ctx : self->_shimContexts) {
                if (ctx) totalDropped += ctx->droppedCount;
            }
            result["dropped_events"] = totalDropped;
            result["ring_head"] = self->_ringHead;
        },
        this,
        false
    );
}
