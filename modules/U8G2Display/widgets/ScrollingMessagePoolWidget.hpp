/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/ScrollingMessagePoolWidget.hpp
 * Description: EventBus-driven scrolling message widget that subscribes to the
 *              "display.message" topic and maintains an internal circular buffer
 *              of received messages. Inherits marquee scrolling from
 *              ScrollingTextWidget and automatically cycles through buffered
 *              messages as each scroll completes. Uses EventBus for decoupled
 *              message delivery instead of direct MessagePool* dependencies.
 *
 * Exported Classes:
 * - ScrollingMessagePoolWidget: [Class] EventBus message rotation scroller.
 *   - begin(): Subscribe to "display.message" on the provided EventBus.
 *   - pushMessage(): Manually add a message to the circular buffer.
 *   - getMessageCount(): Return number of messages currently buffered.
 *   - tick(): Handle pool traversal and marquee progression.
 */

#ifndef SCROLLING_MESSAGE_POOL_WIDGET_HPP
#define SCROLLING_MESSAGE_POOL_WIDGET_HPP                   ///< Scrolling Message Pool Widget Hpp

#include "ScrollingTextWidget.hpp"
#include <cstddef>

// Forward declaration — avoids pulling EventBus.hpp into the header
class EventBus;

/**
 * @brief Maximum number of messages held in the circular buffer.
 *        Older messages are overwritten when the buffer is full.
 */
static constexpr size_t MSG_POOL_BUFFER_SIZE = 8;

/**
 * @brief Maximum length of each individual message string (including null terminator).
 */
static constexpr size_t MSG_POOL_MAX_MSG_LEN = 256;

/**
 * @brief EventBus-driven scrolling message pool widget.
 *        Subscribes to the "display.message" EventBus topic and buffers
 *        incoming messages in a fixed-size circular buffer. Automatically
 *        feeds messages to the ScrollingTextWidget marquee engine and
 *        cycles through them as each scroll completes.
 */
class ScrollingMessagePoolWidget : public ScrollingTextWidget {
private:
    char _messageBuffer[MSG_POOL_BUFFER_SIZE][MSG_POOL_MAX_MSG_LEN]; // Circular message buffer
    size_t _writeIndex;      // Next write position in circular buffer
    size_t _messageCount;    // Total messages currently in the buffer (max MSG_POOL_BUFFER_SIZE)
    size_t _readIndex;       // Current read position for sequential playback
    EventBus* _eventBus;     // Cached EventBus reference for unsubscribe (if needed)

    /**
     * @brief Load the next message from the circular buffer into the marquee.
     * @return True if a message was successfully loaded; false if buffer is empty.
     */
    bool loadNextMessage();

    /**
     * @brief Static EventBus callback for "display.message" topic.
     *        Casts ctx to ScrollingMessagePoolWidget* and pushes the
     *        payload message into the circular buffer.
     * @param topic The topic string ("display.message").
     * @param payload Pointer to a DisplayMessagePayload struct.
     * @param ctx Context pointer (this widget instance).
     */
    static void _onMessage(const char* topic, const void* payload, void* ctx);

public:
    /**
     * @brief Construct a scrolling message pool widget.
     * @param x X coordinate of top-left corner.
     * @param y Y coordinate of top-left corner.
     * @param w Width of bounding box in pixels.
     * @param h Height of bounding box in pixels.
     * @param font U8G2 font for rendering (nullptr for default).
     */
    ScrollingMessagePoolWidget(int x, int y, int w, int h, const uint8_t* font = nullptr);
    /** @brief Destructor for ScrollingMessagePoolWidget. */
    virtual ~ScrollingMessagePoolWidget() = default;

    /**
     * @brief Subscribe to the "display.message" EventBus topic.
     *        Must be called during initialisation to receive messages.
     * @param bus Reference to the application's EventBus instance.
     */
    void begin(EventBus& bus);

    /**
     * @brief Manually push a message into the circular buffer.
     *        Used for programmatic injection or testing. Overwrites the
     *        oldest entry when the buffer is full.
     * @param msg Null-terminated message string to buffer.
     */
    void pushMessage(const char* msg);

    /**
     * @brief Return the number of messages currently in the circular buffer.
     * @return Message count (0 to MSG_POOL_BUFFER_SIZE).
     */
    size_t getMessageCount() const { return _messageCount; }

    /**
     * @brief Tick handler: feeds messages to the marquee and handles transitions.
     *        Automatically loads the next message when the current scroll finishes.
     * @param currentMillis System uptime in milliseconds.
     */
    virtual void tick(uint32_t currentMillis) override;
};

#endif // SCROLLING_MESSAGE_POOL_WIDGET_HPP
