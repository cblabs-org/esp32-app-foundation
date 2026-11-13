/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/cblabs-org/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/EventBus/EventBus.hpp
 * Description: Typed publish/subscribe event bus system using static function pointers.
 *
 * Exported Functions/Classes:
 * - EventBus: Main class for subscribing to and publishing events
 *   - begin(): Initialize the event bus
 *   - subscribe(): Subscribe to a topic with a callback and context
 *   - unsubscribe(): Remove a subscription
 *   - publish(): Dispatch an event payload to all subscribers (see contract below)
 *   - hashTopic(): Static helper to hash topic strings to uint32_t
 */

#ifndef EVENT_BUS_HPP
#define EVENT_BUS_HPP                                       ///< Event Bus Hpp

#include <stdint.h>
#include <stddef.h>
#include <new>
#include <vector>

/**
 * @brief Concurrency & Lifecycle Contract for EventBus:
 *
 * 1. Execution Context (CRITICAL — differs between native and target):
 *    - **Native/unit-test builds:** dispatch is synchronous and inline within
 *      the caller's thread. publish() returns only after all matching
 *      callbacks have executed.
 *    - **Target (ESP32/FreeRTOS):** publish() posts to the esp_event default
 *      event loop via esp_event_post(). Callbacks execute on the **default
 *      event loop task** — NOT the publisher's task. This means:
 *        • The publisher and subscriber may run on different tasks.
 *        • There is NO synchronous ordering guarantee between a publish()
 *          call returning and the subscriber's callback executing.
 *        • Subscribers reacting to lifecycle events (e.g. APP_CONFIGURING,
 *          APP_RUNNING) should treat their reaction as best-effort within
 *          the next event-loop cycle. Callers MUST NOT assume the subscriber
 *          has acted by the time publish() returns.
 *        • The default event loop task has a small stack
 *          (CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE — 2304 bytes in this
 *          project's sdkconfigs). Callbacks performing heavy work (large JSON
 *          serialization, LittleFS flushes, HTTP requests, starting servers)
 *          risk stack overflow. Prefer setting an atomic flag and acting on it
 *          in tick() (§6); otherwise offload to a dedicated task.
 *
 * 2. Blocking Behaviour:
 *    - publish() uses portMAX_DELAY when posting to the event queue. If the
 *      default event loop queue is full, the publisher BLOCKS indefinitely
 *      until a slot becomes available. This is safe for normal event rates
 *      but could cause watchdog resets under event storms.
 *
 * 3. Topic Interning & Allocation Guarantees:
 *    - Topics are interned to 32-bit FNV-1a hashes (uint32_t) at subscription and publish time.
 *    - `publish()` incurs ZERO heap allocations on the hot path (in-place vector iteration).
 *
 * 4. Re-entrancy & Mutation Safety:
 *    - Callbacks are executed in-place without duplicating subscriber vectors.
 *    - Callbacks MAY safely call `subscribe()` or `unsubscribe()` (including self-unsubscription)
 *      during event dispatch. Soft-deletion (tombstoning) ensures iterator and index stability,
 *      and inactive slots are compacted after top-level dispatch completes.
 *    - Subscriptions added during an active `publish()` will not receive the currently dispatched event,
 *      but will participate in future publications.
 *
 * 5. Execution Budget & Payload Lifetime:
 *    - Callback routines MUST comply with the cooperative tick budget (<= 10 ms per callback).
 *    - SUBSCRIBER-SIDE: Event payload pointers are valid ONLY for the duration of the callback execution.
 *      If the subscriber needs data beyond the callback, it must copy the payload.
 *    - PUBLISHER-SIDE: On target, dispatch is asynchronous (§1). The payload
 *      must remain valid from publish() until all subscribers have executed —
 *      which is AFTER publish() returns. Do NOT pass the address of a stack
 *      local. Use one of these patterns:
 *        (a) A persistent member variable (e.g. Application::_systemHeapPayload)
 *        (b) A ring buffer sized to the maximum publish cadence (e.g.
 *            ButtonModule::_eventRing, EncoderModule::_eventRing)
 *      On native (UNIT_TEST), dispatch is synchronous, so stack-locals appear
 *      to work — but the bug manifests only on target, intermittently.
 *
 * 6. Thread Safety — LVGL and Other Single-Threaded Libraries:
 *    - On target, callbacks execute on the esp_event default loop task (see §1),
 *      NOT the main loop. Never call LVGL APIs (lv_obj_*, lv_scr_*, lv_label_*,
 *      lv_anim_*, etc.) from an EventBus callback — they race lv_timer_handler()
 *      on the main loop, and LV_USE_OS is unset (no internal locking).
 *    - Instead, set an atomic flag in the callback and act on it in tick().
 *      Example: _pendingInputWake.store(true) in callback, then
 *      if (_pendingInputWake.exchange(false)) { ... } in tick().
 *    - The same rule applies to any library with single-threaded assumptions
 *      (U8G2 rendering, direct SPI transactions, etc.).
 *
 * Example — ring buffer pattern (recommended for tick-driven publishers):
 *
 *   MyEvent& evt = _eventRing[_ringHead];
 *   evt.field = value;
 *   _ringHead = (_ringHead + 1) % RING_SIZE;
 *   _bus->publish("my.topic", &evt);
 *
 * The ring slot remains valid until overwritten on the next wrap.
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
    void _ensureEventLoop();  ///< Lazy-create default esp_event loop on first use
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

