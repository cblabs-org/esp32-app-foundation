/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * This work is licensed under Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International.
 * To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * Module: modules/u8g2Display/widgets/scrollingMessagePoolWidget.hpp
 * Description: EventBus-driven scrolling message widget that subscribes to the
 *              "display.message" topic and maintains an internal circular buffer
 *              of received messages. Inherits marquee scrolling from
 *              scrollingTextWidget and automatically cycles through buffered
 *              messages as each scroll completes. Uses EventBus for decoupled
 *              message delivery instead of direct MessagePool* dependencies.
 *
 * Exported Classes:
 * - scrollingMessagePoolWidget: [Class] EventBus message rotation scroller.
 *   - begin(): Subscribe to "display.message" on the provided EventBus.
 *   - pushMessage(): Manually add a message to the circular buffer.
 *   - getMessageCount(): Return number of messages currently buffered.
 *   - tick(): Handle pool traversal and marquee progression.
 */

#ifndef SCROLLING_MESSAGE_POOL_WIDGET_HPP
#define SCROLLING_MESSAGE_POOL_WIDGET_HPP

#include "scrollingTextWidget.hpp"
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
 *        feeds messages to the scrollingTextWidget marquee engine and
 *        cycles through them as each scroll completes.
 */
class scrollingMessagePoolWidget : public scrollingTextWidget {
private:
    char messageBuffer[MSG_POOL_BUFFER_SIZE][MSG_POOL_MAX_MSG_LEN]; // Circular message buffer
    size_t writeIndex;       // Next write position in circular buffer
    size_t messageCount;     // Total messages currently in the buffer (max MSG_POOL_BUFFER_SIZE)
    size_t readIndex;        // Current read position for sequential playback
    EventBus* eventBus;      // Cached EventBus reference for unsubscribe (if needed)

    /**
     * @brief Load the next message from the circular buffer into the marquee.
     * @return True if a message was successfully loaded; false if buffer is empty.
     */
    bool loadNextMessage();

    /**
     * @brief Static EventBus callback for "display.message" topic.
     *        Casts ctx to scrollingMessagePoolWidget* and pushes the
     *        payload message into the circular buffer.
     * @param topic The topic string ("display.message").
     * @param payload Pointer to a DisplayMessagePayload struct.
     * @param ctx Context pointer (this widget instance).
     */
    static void _onMessage(const char* topic, const void* payload, void* ctx);

public:
    /**
     * @brief Construct a scrolling message pool widget.
     * @param _x X coordinate of top-left corner.
     * @param _y Y coordinate of top-left corner.
     * @param _w Width of bounding box in pixels.
     * @param _h Height of bounding box in pixels.
     * @param _font U8G2 font for rendering (nullptr for default).
     */
    scrollingMessagePoolWidget(int _x, int _y, int _w, int _h, const uint8_t* _font = nullptr);
    virtual ~scrollingMessagePoolWidget() = default;

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
    size_t getMessageCount() const { return messageCount; }

    /**
     * @brief Tick handler: feeds messages to the marquee and handles transitions.
     *        Automatically loads the next message when the current scroll finishes.
     * @param currentMillis System uptime in milliseconds.
     */
    virtual void tick(uint32_t currentMillis) override;
};

#endif // SCROLLING_MESSAGE_POOL_WIDGET_HPP
