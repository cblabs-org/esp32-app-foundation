/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/ButtonModule/ButtonModule.cpp
 * Description: Implementation of ButtonModule button event collector and EventBus publisher.
 */

#include "ButtonModule.hpp"
#include "modules/Application/Application.hpp"
#include "modules/MCPServer/MCPToolRegistry.hpp"
#include <map>
#include <cstring>
#include <new>

#ifndef UNIT_TEST
#include "iot_button.h"
#include "button_gpio.h"
#include "button_adc.h"
#include "button_matrix.h"
#else
#include "iot_button.h"
#endif

#include "platform/LogShim.h"

static constexpr const char* TAG = "ButtonModule";

ButtonModule::ButtonModule() = default;

ButtonModule::~ButtonModule() {
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

void ButtonModule::addButton(const BoardButton& button) {
    _pendingButtons.push_back(button);
}

bool ButtonModule::removeButton(const char* id) {
    if (!id) return false;

    // --- Pre-begin path: remove from pending registration list ---
    if (!_eventQueue) {
        for (auto it = _pendingButtons.begin(); it != _pendingButtons.end(); ++it) {
            if (it->id && strcmp(it->id, id) == 0) {
                _pendingButtons.erase(it);
                return true;
            }
        }
        return false;
    }

    // --- Post-begin path: find and remove active button ---
    // Locate the button index in _pendingButtons (kept for MCP status).
    // Each button produces one _buttonHandles entry and multiple _shimContexts.
    size_t buttonIndex = SIZE_MAX;
    for (size_t i = 0; i < _pendingButtons.size(); i++) {
        if (_pendingButtons[i].id && strcmp(_pendingButtons[i].id, id) == 0) {
            buttonIndex = i;
            break;
        }
    }
    if (buttonIndex == SIZE_MAX) return false;

    // Delete the iot_button handle (1:1 with buttons, indexed by registration order)
    if (buttonIndex < _buttonHandles.size()) {
#ifndef UNIT_TEST
        if (_buttonHandles[buttonIndex]) {
            iot_button_delete(static_cast<button_handle_t>(_buttonHandles[buttonIndex]));
        }
#endif
        _buttonHandles.erase(_buttonHandles.begin() + static_cast<ptrdiff_t>(buttonIndex));
    }

    // Remove all ShimContexts belonging to this button (matched by id pointer)
    const char* targetId = _pendingButtons[buttonIndex].id;
    for (auto it = _shimContexts.begin(); it != _shimContexts.end(); ) {
        if (*it && (*it)->id == targetId) {
            delete *it;
            it = _shimContexts.erase(it);
        } else {
            ++it;
        }
    }

    // Remove from pending list (used by MCP status tool)
    _pendingButtons.erase(_pendingButtons.begin() + static_cast<ptrdiff_t>(buttonIndex));

    return true;
}

void ButtonModule::begin(Application& app) {
    _bus = &app.getEventBus();
    if (!_eventQueue) {
        _eventQueue = xQueueCreate(QUEUE_DEPTH, sizeof(ButtonEvent));
    }

    // Phase 1: Group ADC ladder inputs by pin, process GPIO & Matrix inputs
    std::map<int8_t, std::vector<const BoardButton*>> adcGroups;

    for (const auto& button : _pendingButtons) {
        switch (button.type) {
            case ButtonType::GPIO:
                createGpioButton(button);
                break;
            case ButtonType::ADC_LADDER:
                adcGroups[button.adc.adcPin].push_back(&button);
                break;
            case ButtonType::MATRIX:
                createMatrixButton(button);
                break;
        }
    }

    // Phase 2: Grouped ADC ladder buttons
    for (const auto& pair : adcGroups) {
        createAdcLadder(pair.first, pair.second);
    }
}

void ButtonModule::createGpioButton(const BoardButton& button) {
    static const ButtonGesture gestures[] = {
        ButtonGesture::CLICK,
        ButtonGesture::DOUBLE_CLICK,
        ButtonGesture::LONG_PRESS,
        ButtonGesture::PRESS_DOWN
    };

    button_handle_t btn_handle = nullptr;

#ifndef UNIT_TEST
    button_config_t btn_cfg = {};
    button_gpio_config_t gpio_cfg = {
        .gpio_num = button.gpio.pin,
        .active_level = static_cast<uint8_t>(button.gpio.activeLow ? 0 : 1),
        .enable_power_save = button.gpio.powerSave,
        .disable_pull = button.gpio.disablePull
    };

    iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn_handle);
#endif

    if (btn_handle) {
        _buttonHandles.push_back(btn_handle);
    }

    for (auto gesture : gestures) {
        auto* ctx = new (std::nothrow) ShimContext{
            .id = button.id,
            .gesture = gesture,
            .queue = _eventQueue,
            .droppedCount = 0
        };
        if (!ctx) {
            ESP_LOGE(TAG, "Failed to allocate ShimContext for button %s", button.id);
            continue;
        }
        _shimContexts.push_back(ctx);

#ifndef UNIT_TEST
        if (btn_handle) {
            button_event_t evt = BUTTON_SINGLE_CLICK;
            switch (gesture) {
                case ButtonGesture::CLICK:        evt = BUTTON_SINGLE_CLICK; break;
                case ButtonGesture::DOUBLE_CLICK: evt = BUTTON_DOUBLE_CLICK; break;
                case ButtonGesture::LONG_PRESS:   evt = BUTTON_LONG_PRESS_START; break;
                case ButtonGesture::PRESS_DOWN:   evt = BUTTON_PRESS_DOWN; break;
            }
            iot_button_register_cb(btn_handle, evt, nullptr, _buttonShim, ctx);
        }
#endif
    }
}

void ButtonModule::createAdcLadder(int8_t pin, const std::vector<const BoardButton*>& buttons) {
    static const ButtonGesture gestures[] = {
        ButtonGesture::CLICK,
        ButtonGesture::DOUBLE_CLICK,
        ButtonGesture::LONG_PRESS,
        ButtonGesture::PRESS_DOWN
    };

    uint8_t btnIndex = 0;
    for (const auto* buttonPtr : buttons) {
        if (!buttonPtr) continue;
        const auto& button = *buttonPtr;
        button_handle_t btn_handle = nullptr;

#ifndef UNIT_TEST
        button_config_t btn_cfg = {};
        button_adc_config_t adc_cfg = {
            .adc_handle = nullptr,
            .unit_id = ADC_UNIT_1,
            .adc_channel = static_cast<uint8_t>(button.adc.adcChannel),
            .button_index = btnIndex++,
            .min = button.adc.minMv,
            .max = button.adc.maxMv
        };

        iot_button_new_adc_device(&btn_cfg, &adc_cfg, &btn_handle);
#endif

        if (btn_handle) {
            _buttonHandles.push_back(btn_handle);
        }

        for (auto gesture : gestures) {
            auto* ctx = new (std::nothrow) ShimContext{
                .id = button.id,
                .gesture = gesture,
                .queue = _eventQueue,
                .droppedCount = 0
            };
            if (!ctx) {
                ESP_LOGE(TAG, "Failed to allocate ShimContext for button %s", button.id);
                continue;
            }
            _shimContexts.push_back(ctx);

#ifndef UNIT_TEST
            if (btn_handle) {
                button_event_t evt = BUTTON_SINGLE_CLICK;
                switch (gesture) {
                    case ButtonGesture::CLICK:        evt = BUTTON_SINGLE_CLICK; break;
                    case ButtonGesture::DOUBLE_CLICK: evt = BUTTON_DOUBLE_CLICK; break;
                    case ButtonGesture::LONG_PRESS:   evt = BUTTON_LONG_PRESS_START; break;
                    case ButtonGesture::PRESS_DOWN:   evt = BUTTON_PRESS_DOWN; break;
                }
                iot_button_register_cb(btn_handle, evt, nullptr, _buttonShim, ctx);
            }
#endif
        }
    }
}

void ButtonModule::createMatrixButton(const BoardButton& button) {
    static const ButtonGesture gestures[] = {
        ButtonGesture::CLICK,
        ButtonGesture::DOUBLE_CLICK,
        ButtonGesture::LONG_PRESS,
        ButtonGesture::PRESS_DOWN
    };

    button_handle_t btn_handle = nullptr;

#ifndef UNIT_TEST
    button_config_t btn_cfg = {};
    int32_t row_gpios[1] = { button.matrix.rowPin };
    int32_t col_gpios[1] = { button.matrix.colPin };
    button_matrix_config_t matrix_cfg = {
        .row_gpios = row_gpios,
        .col_gpios = col_gpios,
        .row_gpio_num = 1,
        .col_gpio_num = 1
    };
    size_t btn_cnt = 1;
    iot_button_new_matrix_device(&btn_cfg, &matrix_cfg, &btn_handle, &btn_cnt);
#endif

    if (btn_handle) {
        _buttonHandles.push_back(btn_handle);
    }

    for (auto gesture : gestures) {
        auto* ctx = new (std::nothrow) ShimContext{
            .id = button.id,
            .gesture = gesture,
            .queue = _eventQueue,
            .droppedCount = 0
        };
        if (!ctx) {
            ESP_LOGE(TAG, "Failed to allocate ShimContext for button %s", button.id);
            continue;
        }
        _shimContexts.push_back(ctx);

#ifndef UNIT_TEST
        if (btn_handle) {
            button_event_t evt = BUTTON_SINGLE_CLICK;
            switch (gesture) {
                case ButtonGesture::CLICK:        evt = BUTTON_SINGLE_CLICK; break;
                case ButtonGesture::DOUBLE_CLICK: evt = BUTTON_DOUBLE_CLICK; break;
                case ButtonGesture::LONG_PRESS:   evt = BUTTON_LONG_PRESS_START; break;
                case ButtonGesture::PRESS_DOWN:   evt = BUTTON_PRESS_DOWN; break;
            }
            iot_button_register_cb(btn_handle, evt, nullptr, _buttonShim, ctx);
        }
#endif
    }
}

void ButtonModule::_buttonShim(void* handle, void* userData) {
    if (!userData) return;
    auto* ctx = static_cast<ShimContext*>(userData);
    if (!ctx->queue) return;
    ButtonEvent evt = { .id = ctx->id, .gesture = ctx->gesture };
    if (xQueueSend(ctx->queue, &evt, 0) != pdTRUE) {
        ctx->droppedCount++;
    }
}

void ButtonModule::tick(uint32_t currentMillis) {
    if (!_eventQueue || !_bus) return;
    ButtonEvent evt;
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

const char* ButtonModule::gestureToTopic(ButtonGesture gesture) {
    switch (gesture) {
        case ButtonGesture::CLICK:        return "button.click";
        case ButtonGesture::DOUBLE_CLICK: return "button.double_click";
        case ButtonGesture::LONG_PRESS:   return "button.long_press";
        case ButtonGesture::PRESS_DOWN:   return "button.press_down";
        default:                         return "button.unknown";
    }
}

void ButtonModule::registerMCPTools(MCPToolRegistry& registry) {
    registry.addTool(
        "button_status",
        "Get registered hardware buttons, recent event ring buffer state, and dropped event diagnostics",
        [](void* context, JsonObject& args, JsonObject& result) {
            auto* self = static_cast<ButtonModule*>(context);
            if (!self) return;
            JsonArray buttons = result["buttons"].to<JsonArray>();
            for (const auto& button : self->_pendingButtons) {
                JsonObject obj = buttons.add<JsonObject>();
                obj["id"] = button.id ? button.id : "";
                switch (button.type) {
                    case ButtonType::GPIO:       obj["type"] = "GPIO"; break;
                    case ButtonType::ADC_LADDER: obj["type"] = "ADC_LADDER"; break;
                    case ButtonType::MATRIX:     obj["type"] = "MATRIX"; break;
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
