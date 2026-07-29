/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/EventBus/EventBus.hpp
 * Description: Typed publish/subscribe event bus system using static function pointers.
 *
 * Exported Functions/Classes:
 * - EventBus: Main class for subscribing to and publishing events
 *   - begin(): Initialize the event bus
 *   - subscribe(): Subscribe to a topic with a callback and context
 *   - unsubscribe(): Remove a subscription
 *   - publish(): Synchronously dispatch an event payload to all subscribers
 *   - hashTopic(): Static helper to hash topic strings to uint32_t
 */

#ifndef EVENT_BUS_HPP
#define EVENT_BUS_HPP

#include <stdint.h>
#include <stddef.h>
#include <vector>

/**
 * @brief Concurrency & Lifecycle Contract for EventBus:
 * 
 * 1. Thread Safety & Context:
 *    - In host/native unit testing, dispatch is synchronous and lock-free within the caller thread.
 *    - On target (ESP32/FreeRTOS), publish calls execute synchronously across subscribers, or via esp_event.
 * 
 * 2. Topic Interning & Allocation Guarantees:
 *    - Topics are interned to 32-bit FNV-1a hashes (uint32_t) at subscription and publish time.
 *    - `publish()` incurs ZERO heap allocations on the hot path (in-place vector iteration).
 * 
 * 3. Re-entrancy & Mutation Safety:
 *    - Callbacks are executed in-place without duplicating subscriber vectors.
 *    - Callbacks MAY safely call `subscribe()` or `unsubscribe()` (including self-unsubscription)
 *      during event dispatch. Soft-deletion (tombstoning) ensures iterator and index stability,
 *      and inactive slots are compacted after top-level dispatch completes.
 *    - Subscriptions added during an active `publish()` will not receive the currently dispatched event,
 *      but will participate in future publications.
 * 
 * 4. Execution Budget & Payload Lifetime:
 *    - Callback routines MUST comply with the cooperative tick budget (<= 10 ms per callback).
 *    - Event payload pointers are valid ONLY for the duration of the callback execution.
 */

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
    uint32_t topicHash;                     // Interned topic identity (32-bit FNV-1a hash)
    EventCallback callback;                 // Callback function
    void* ctx;                              // User context
#ifdef UNIT_TEST
    bool active;                            // Is this slot active?
#else
    const char* topicStr;
    void* instance;
#endif
};

/**
 * @brief Typed publish/subscribe event bus system.
 */
class EventBus {
private:
#ifdef UNIT_TEST
    std::vector<Subscription> _subscribers;
    uint32_t _dispatchDepth = 0;
    bool _needsCleanup = false;

    void cleanup();
#else
    std::vector<Subscription*> _subscribers;
#endif

public:
    /**
     * @brief Utility function to compute FNV-1a 32-bit hash of a topic string.
     * @param str Topic string.
     * @return uint32_t 32-bit hash value, or 0 if string is null or empty.
     */
    static inline uint32_t hashTopic(const char* str) {
        if (!str || str[0] == '\0') return 0;
        uint32_t hash = 2166136261u;
        while (*str) {
            hash ^= static_cast<uint8_t>(*str++);
            hash *= 16777619u;
        }
        return hash;
    }

    /**
     * @brief Initialize the event bus, clearing all subscriptions and resetting state.
     */
    void begin();

    /**
     * @brief Subscribe to a topic.
     * @param topic Topic string to subscribe to (exact match).
     * @param callback Function to call when an event is published.
     * @param ctx User context to pass to the callback.
     * @return true if subscription succeeded, false if invalid topic or callback.
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

