/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/eventBus/EventBus.cpp
 * Description: Implementation of the typed pub/sub event bus system.
 *
 * Exported Functions/Classes:
 * - EventBus::begin(): Initialize the event bus
 * - EventBus::subscribe(): Subscribe to a topic
 * - EventBus::unsubscribe(): Unsubscribe from a topic
 * - EventBus::publish(): Synchronously publish an event
 */

#include "EventBus.hpp"
#include <string.h>

void EventBus::begin() {
    for (int i = 0; i < EVENT_BUS_MAX_SUBSCRIBERS; i++) {
        _subscribers[i].active = false;
        _subscribers[i].topic[0] = '\0';
        _subscribers[i].callback = nullptr;
        _subscribers[i].ctx = nullptr;
    }
}

bool EventBus::subscribe(const char* topic, EventCallback callback, void* ctx) {
    if (topic == nullptr || callback == nullptr) {
        return false;
    }

    size_t topicLen = strlen(topic);
    if (topicLen == 0 || topicLen >= EVENT_BUS_MAX_TOPIC_LENGTH) {
        return false;
    }

    // --- Step 1: Find an empty slot ---
    // Look for the first inactive subscription slot to store the new subscription
    for (int i = 0; i < EVENT_BUS_MAX_SUBSCRIBERS; i++) {
        if (!_subscribers[i].active) {
            strncpy(_subscribers[i].topic, topic, EVENT_BUS_MAX_TOPIC_LENGTH - 1);
            _subscribers[i].topic[EVENT_BUS_MAX_TOPIC_LENGTH - 1] = '\0';
            _subscribers[i].callback = callback;
            _subscribers[i].ctx = ctx;
            _subscribers[i].active = true;
            return true;
        }
    }

    // No empty slot found
    return false;
}

bool EventBus::unsubscribe(const char* topic, EventCallback callback, void* ctx) {
    if (topic == nullptr || callback == nullptr) {
        return false;
    }

    for (int i = 0; i < EVENT_BUS_MAX_SUBSCRIBERS; i++) {
        if (_subscribers[i].active) {
            if (strncmp(_subscribers[i].topic, topic, EVENT_BUS_MAX_TOPIC_LENGTH) == 0 &&
                _subscribers[i].callback == callback &&
                _subscribers[i].ctx == ctx) {
                
                _subscribers[i].active = false;
                _subscribers[i].topic[0] = '\0';
                _subscribers[i].callback = nullptr;
                _subscribers[i].ctx = nullptr;
                return true;
            }
        }
    }

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
    for (int i = 0; i < EVENT_BUS_MAX_SUBSCRIBERS; i++) {
        if (_subscribers[i].active) {
            if (strncmp(_subscribers[i].topic, topic, EVENT_BUS_MAX_TOPIC_LENGTH) == 0) {
                // Dispatch event to subscriber
                _subscribers[i].callback(topic, payload, _subscribers[i].ctx);
                count++;
            }
        }
    }

    return count;
}
