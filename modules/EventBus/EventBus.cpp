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
#include <string.h>

static const char* TAG = "EventBus";

void EventBus::begin() {
    ESP_LOGI(TAG, "Initializing EventBus");
    _subscribers.clear();
}

bool EventBus::subscribe(const char* topic, EventCallback callback, void* ctx) {
    if (topic == nullptr || callback == nullptr) {
        ESP_LOGW(TAG, "subscribe() failed: null topic or callback");
        return false;
    }

    size_t topicLen = strlen(topic);
    if (topicLen == 0) {
        ESP_LOGW(TAG, "subscribe() failed: topic is empty");
        return false;
    }

    _subscribers.push_back({std::string(topic), callback, ctx, true});
    ESP_LOGD(TAG, "Subscribed to topic: %s", topic);
    return true;
}

bool EventBus::unsubscribe(const char* topic, EventCallback callback, void* ctx) {
    if (topic == nullptr || callback == nullptr) {
        ESP_LOGD(TAG, "unsubscribe() called with null topic or callback");
        return false;
    }

    for (auto it = _subscribers.begin(); it != _subscribers.end(); ++it) {
        if (it->active) {
            if (it->topic == topic &&
                it->callback == callback &&
                it->ctx == ctx) {
                
                _subscribers.erase(it);
                return true;
            }
        }
    }

    ESP_LOGD(TAG, "unsubscribe(): no matching subscription found for topic '%s'", topic);
    return false;
}

int EventBus::publish(const char* topic, const void* payload) {
    if (topic == nullptr) {
        return 0;
    }

    int count = 0;
    
    // --- Step 1: Iterate over subscribers and dispatch ---
    // The publish is synchronous, meaning each callback is executed before returning.
    // Payloads are only valid for the duration of the callback.
    // We iterate over a copy of the subscribers to ensure safety if a callback unsubscribes itself.
    std::vector<Subscription> targets = _subscribers;
    for (const auto& sub : targets) {
        if (sub.active && sub.topic == topic) {
            // Dispatch event to subscriber
            sub.callback(topic, payload, sub.ctx);
            count++;
        }
    }

    ESP_LOGD(TAG, "Published topic '%s' to %d subscriber(s)", topic, count);
    return count;
}
