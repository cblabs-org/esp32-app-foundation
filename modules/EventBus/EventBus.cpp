/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/EventBus/EventBus.cpp
 * Description: Implementation of the typed pub/sub event bus system.
 *
 * Exported Functions/Classes:
 * - EventBus::begin(): Initialize the event bus
 * - EventBus::subscribe(): Subscribe to a topic
 * - EventBus::unsubscribe(): Unsubscribe from a topic
 * - EventBus::publish(): Synchronously publish an event
 */

#include "EventBus.hpp"
#include "platform/LogShim.h"
#include <algorithm>

static constexpr const char* TAG = "EventBus";

#ifndef UNIT_TEST
#include "esp_event.h"
ESP_EVENT_DEFINE_BASE(APP_EVENT_BASE);

static void event_trampoline(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data) {
    Subscription* sub = static_cast<Subscription*>(handler_arg);
    const void* payload = nullptr;
    if (event_data != nullptr) {
        payload = *static_cast<const void**>(event_data);
    }
    if (sub->callback) {
        sub->callback(sub->topicStr, payload, sub->ctx);
    }
}
#endif

void EventBus::begin() {
    ESP_LOGI(TAG, "Initializing EventBus");
#ifdef UNIT_TEST
    _subscribers.clear();
    _dispatchDepth = 0;
    _needsCleanup = false;
#else
    esp_event_loop_create_default();
    for (Subscription* sub : _subscribers) {
        esp_event_handler_instance_unregister(APP_EVENT_BASE, sub->topicHash, (esp_event_handler_instance_t)sub->instance);
        delete sub;
    }
    _subscribers.clear();
#endif
}

#ifdef UNIT_TEST
void EventBus::cleanup() {
    _subscribers.erase(
        std::remove_if(_subscribers.begin(), _subscribers.end(),
                       [](const Subscription& s) { return !s.active; }),
        _subscribers.end()
    );
    _needsCleanup = false;
}
#endif

bool EventBus::subscribe(const char* topic, EventCallback callback, void* ctx) {
    if (topic == nullptr || callback == nullptr) {
        ESP_LOGW(TAG, "subscribe() failed: null topic or callback");
        return false;
    }

    if (topic[0] == '\0') {
        ESP_LOGW(TAG, "subscribe() failed: topic is empty");
        return false;
    }

    uint32_t topicHash = hashTopic(topic);

#ifdef UNIT_TEST
    if (_dispatchDepth == 0) {
        // Reuse an inactive slot if available when not dispatching
        for (auto& sub : _subscribers) {
            if (!sub.active) {
                sub.topicHash = topicHash;
                sub.callback = callback;
                sub.ctx = ctx;
                sub.active = true;
                ESP_LOGD(TAG, "Subscribed to topic: %s (slot reused)", topic);
                return true;
            }
        }
    }

    _subscribers.push_back({topicHash, callback, ctx, true});
#else
    Subscription* sub = new Subscription();
    sub->topicHash = topicHash;
    sub->callback = callback;
    sub->ctx = ctx;
    sub->topicStr = topic;
    
    esp_event_handler_instance_t instance;
    esp_err_t err = esp_event_handler_instance_register(APP_EVENT_BASE, topicHash, event_trampoline, sub, &instance);
    if (err != ESP_OK) {
        delete sub;
        return false;
    }
    sub->instance = instance;
    _subscribers.push_back(sub);
#endif
    ESP_LOGD(TAG, "Subscribed to topic: %s", topic);
    return true;
}

bool EventBus::unsubscribe(const char* topic, EventCallback callback, void* ctx) {
    if (topic == nullptr || callback == nullptr || topic[0] == '\0') {
        ESP_LOGD(TAG, "unsubscribe() called with invalid topic or callback");
        return false;
    }

    uint32_t topicHash = hashTopic(topic);

#ifdef UNIT_TEST
    for (size_t i = 0; i < _subscribers.size(); ++i) {
        if (_subscribers[i].active &&
            _subscribers[i].topicHash == topicHash &&
            _subscribers[i].callback == callback &&
            _subscribers[i].ctx == ctx) {

            if (_dispatchDepth > 0) {
                _subscribers[i].active = false;
                _needsCleanup = true;
            } else {
                _subscribers.erase(_subscribers.begin() + i);
            }
            return true;
        }
    }
#else
    for (auto it = _subscribers.begin(); it != _subscribers.end(); ++it) {
        Subscription* sub = *it;
        if (sub->topicHash == topicHash && sub->callback == callback && sub->ctx == ctx) {
            esp_event_handler_instance_unregister(APP_EVENT_BASE, topicHash, (esp_event_handler_instance_t)sub->instance);
            delete sub;
            _subscribers.erase(it);
            return true;
        }
    }
#endif

    ESP_LOGD(TAG, "unsubscribe(): no matching subscription found for topic '%s'", topic);
    return false;
}

int EventBus::publish(const char* topic, const void* payload) {
    if (topic == nullptr || topic[0] == '\0') {
        return 0;
    }

    uint32_t topicHash = hashTopic(topic);
    int count = 0;
#ifdef UNIT_TEST
    _dispatchDepth++;

    const size_t initialSize = _subscribers.size();
    for (size_t i = 0; i < initialSize; ++i) {
        if (_subscribers[i].active && _subscribers[i].topicHash == topicHash) {
            _subscribers[i].callback(topic, payload, _subscribers[i].ctx);
            count++;
        }
    }

    _dispatchDepth--;
    if (_dispatchDepth == 0 && _needsCleanup) {
        cleanup();
    }
#else
    const void* p = payload;
    esp_err_t err = esp_event_post(APP_EVENT_BASE, topicHash, &p, sizeof(p), portMAX_DELAY);
    if (err == ESP_OK) {
        for (Subscription* sub : _subscribers) {
            if (sub->topicHash == topicHash) {
                count++;
            }
        }
    }
#endif

    ESP_LOGD(TAG, "Published topic '%s' to %d subscriber(s)", topic, count);
    return count;
}
