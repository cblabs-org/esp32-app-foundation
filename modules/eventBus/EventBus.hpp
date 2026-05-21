/*
 * Departures Board (c) 2025-2026 Gadec Software
 * Refactored for v3.0 by Matt McNeill 2026 CB Labs
 *
 * https://github.com/gadec-uk/departures-board
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/eventBus/EventBus.hpp
 * Description: Typed publish/subscribe event bus system using static function pointers.
 *
 * Exported Functions/Classes:
 * - EventBus: Main class for subscribing to and publishing events
 *   - begin(): Initialize the event bus
 *   - subscribe(): Subscribe to a topic with a callback and context
 *   - unsubscribe(): Remove a subscription
 *   - publish(): Synchronously dispatch an event payload to all subscribers
 */

#ifndef EVENT_BUS_HPP
#define EVENT_BUS_HPP

#include <stdint.h>
#include <stddef.h>

#define EVENT_BUS_MAX_SUBSCRIBERS 32
#define EVENT_BUS_MAX_TOPIC_LENGTH 64

/**
 * @brief Type definition for an event callback.
 * @param topic The topic that was published to.
 * @param payload Pointer to the event data. Cast to expected type.
 * @param ctx Context pointer provided during subscription.
 */
typedef void (*EventCallback)(const char* topic, const void* payload, void* ctx);

/**
 * @brief Structure to hold a single subscription record.
 */
struct Subscription {
    char topic[EVENT_BUS_MAX_TOPIC_LENGTH]; // Topic string
    EventCallback callback;                 // Callback function
    void* ctx;                              // User context
    bool active;                            // Is this slot active?
};

/**
 * @brief Typed publish/subscribe event bus system.
 */
class EventBus {
private:
    Subscription _subscribers[EVENT_BUS_MAX_SUBSCRIBERS];

public:
    /**
     * @brief Initialize the event bus, clearing all subscriptions.
     */
    void begin();

    /**
     * @brief Subscribe to a topic.
     * @param topic Topic string to subscribe to (exact match).
     * @param callback Function to call when an event is published.
     * @param ctx User context to pass to the callback.
     * @return true if subscription succeeded, false if table is full or invalid topic.
     */
    bool subscribe(const char* topic, EventCallback callback, void* ctx = nullptr);

    /**
     * @brief Unsubscribe from a topic.
     * @param topic Topic string to unsubscribe from.
     * @param callback The callback that was originally subscribed.
     * @param ctx The context pointer that was used.
     * @return true if successfully removed, false if not found.
     */
    bool unsubscribe(const char* topic, EventCallback callback, void* ctx = nullptr);

    /**
     * @brief Publish an event synchronously to all matching subscribers.
     * @param topic Topic string to publish to.
     * @param payload Pointer to the payload.
     * @return Number of subscribers that received the event.
     */
    int publish(const char* topic, const void* payload = nullptr);
};

#endif // EVENT_BUS_HPP
