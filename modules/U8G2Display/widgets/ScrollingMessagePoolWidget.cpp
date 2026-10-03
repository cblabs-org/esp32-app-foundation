/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * https://github.com/matthewmcneill/esp32-app-foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Module: modules/U8G2Display/widgets/ScrollingMessagePoolWidget.cpp
 * Description: Implementation of the EventBus-driven scrolling message pool
 *              widget. Subscribes to "display.message" via static callback,
 *              buffers incoming messages in a fixed circular buffer, and feeds
 *              them sequentially to the inherited ScrollingTextWidget marquee.
 *
 * Exported Functions/Classes:
 * - ScrollingMessagePoolWidget: [Class implementation]
 *   - begin(): Registers EventBus subscription for "display.message".
 *   - pushMessage(): Adds a message to the circular buffer.
 *   - tick(): Automated marquee management and message cycling.
 *   - _onMessage(): Static EventBus callback.
 */

#include "ScrollingMessagePoolWidget.hpp"
#include "EventBus.hpp"
#include "U8G2Display.hpp"   // For DisplayMessagePayload struct
#include <cstring>

// =============================================================================
// Constructor
// =============================================================================

/**
 * @brief Initialise the message pool widget with empty circular buffer.
 * @param _x X coordinate.
 * @param _y Y coordinate.
 * @param _w Width of bounding box.
 * @param _h Height of bounding box.
 * @param _font Optional font override.
 */
ScrollingMessagePoolWidget::ScrollingMessagePoolWidget(int _x, int _y, int _w, int _h, const uint8_t* _font)
    : ScrollingTextWidget(_x, _y, _w, _h, _font),
      writeIndex(0), messageCount(0), readIndex(0), eventBus(nullptr) {
    // Clear all buffer slots
    for (size_t i = 0; i < MSG_POOL_BUFFER_SIZE; i++) {
        messageBuffer[i][0] = '\0';
    }
}

// =============================================================================
// EventBus integration
// =============================================================================

/**
 * @brief Subscribe to the "display.message" EventBus topic.
 * @param bus Reference to the application's EventBus instance.
 */
void ScrollingMessagePoolWidget::begin(EventBus& bus) {
    eventBus = &bus;
    bus.subscribe(U8G2DisplayEvents::DISPLAY_MESSAGE, _onMessage, this);
}

/**
 * @brief Static EventBus callback for "display.message" topic.
 *        Extracts the message text from DisplayMessagePayload and pushes
 *        it into the widget's circular buffer.
 * @param topic The topic string (unused beyond type safety).
 * @param payload Pointer to a DisplayMessagePayload struct.
 * @param ctx Context pointer (this widget instance).
 */
void ScrollingMessagePoolWidget::_onMessage(const char* topic, const void* payload, void* ctx) {
    (void)topic;
    auto* self = static_cast<ScrollingMessagePoolWidget*>(ctx);
    const auto* msg = static_cast<const DisplayMessagePayload*>(payload);
    if (self && msg && msg->message) {
        self->pushMessage(msg->message);
    }
}

// =============================================================================
// Circular buffer management
// =============================================================================

/**
 * @brief Push a message into the circular buffer.
 *        Overwrites the oldest entry when the buffer is full.
 * @param msg Null-terminated message string.
 */
void ScrollingMessagePoolWidget::pushMessage(const char* msg) {
    if (!msg) return;

    strncpy(messageBuffer[writeIndex], msg, MSG_POOL_MAX_MSG_LEN - 1);
    messageBuffer[writeIndex][MSG_POOL_MAX_MSG_LEN - 1] = '\0';

    writeIndex = (writeIndex + 1) % MSG_POOL_BUFFER_SIZE;

    if (messageCount < MSG_POOL_BUFFER_SIZE) {
        messageCount++;
    }
}

/**
 * @brief Load the next message from the circular buffer into the marquee.
 *        Cycles through messages sequentially, wrapping around the buffer.
 * @return True if a message was loaded; false if the buffer is empty.
 */
bool ScrollingMessagePoolWidget::loadNextMessage() {
    if (messageCount == 0) return false;

    // Determine valid read range: if buffer not yet full, read [0, messageCount)
    // If full, read from readIndex wrapping around
    if (readIndex >= messageCount && messageCount < MSG_POOL_BUFFER_SIZE) {
        readIndex = 0;  // Wrap around to start
    } else if (messageCount == MSG_POOL_BUFFER_SIZE) {
        // Full buffer — readIndex wraps modulo
        readIndex = readIndex % MSG_POOL_BUFFER_SIZE;
    }

    const char* msg = messageBuffer[readIndex];
    if (msg[0] == '\0') return false;

    setText(msg);
    readIndex = (readIndex + 1) % MSG_POOL_BUFFER_SIZE;
    return true;
}

// =============================================================================
// Tick — message cycling
// =============================================================================

/**
 * @brief Tick handler: feeds messages to the marquee and handles transitions.
 *        If no text is currently showing, attempts to load one from the buffer.
 *        When the current scroll finishes, loads the next message and resets.
 * @param currentMillis System uptime in milliseconds.
 */
void ScrollingMessagePoolWidget::tick(uint32_t currentMillis) {
    if (!isVisible) return;

    // If we have no text currently, try to load one
    if (currentText[0] == '\0') {
        if (!loadNextMessage()) return;
    }

    // Call base class to handle pixel movement
    ScrollingTextWidget::tick(currentMillis);

    // If base class signaled it's done with the current string, load next
    if (isScrollFinished()) {
        if (loadNextMessage()) {
            resetScroll();
        }
    }
}
