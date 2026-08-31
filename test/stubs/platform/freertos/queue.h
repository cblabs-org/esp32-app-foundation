/*
 * ESP32 App Foundation (c) 2026 CB Labs
 *
 * Header: test/stubs/platform/freertos/queue.h
 * Description: Native build stub for FreeRTOS queue API.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>
#include "FreeRTOS.h"

typedef void* QueueHandle_t;

inline QueueHandle_t g_lastCreatedQueue = nullptr;

struct NativeQueue {
    size_t length;
    size_t itemSize;
    std::deque<std::vector<uint8_t>> items;
};

inline QueueHandle_t xQueueCreate(size_t uxQueueLength, size_t uxItemSize) {
    auto* q = new NativeQueue{uxQueueLength, uxItemSize, {}};
    g_lastCreatedQueue = static_cast<QueueHandle_t>(q);
    return g_lastCreatedQueue;
}

inline int xQueueSend(QueueHandle_t xQueue, const void* pvItemToQueue, uint32_t xTicksToWait) {
    if (!xQueue || !pvItemToQueue) return pdFALSE;
    auto* q = static_cast<NativeQueue*>(xQueue);
    if (q->items.size() >= q->length) return pdFALSE;
    const uint8_t* ptr = static_cast<const uint8_t*>(pvItemToQueue);
    q->items.push_back(std::vector<uint8_t>(ptr, ptr + q->itemSize));
    return pdTRUE;
}

inline int xQueueSendToFront(QueueHandle_t xQueue, const void* pvItemToQueue, uint32_t xTicksToWait) {
    if (!xQueue || !pvItemToQueue) return pdFALSE;
    auto* q = static_cast<NativeQueue*>(xQueue);
    if (q->items.size() >= q->length) return pdFALSE;
    const uint8_t* ptr = static_cast<const uint8_t*>(pvItemToQueue);
    q->items.push_front(std::vector<uint8_t>(ptr, ptr + q->itemSize));
    return pdTRUE;
}

inline int xQueueReceive(QueueHandle_t xQueue, void* pvBuffer, uint32_t xTicksToWait) {
    if (!xQueue || !pvBuffer) return pdFALSE;
    auto* q = static_cast<NativeQueue*>(xQueue);
    if (q->items.empty()) return pdFALSE;
    auto front = q->items.front();
    q->items.pop_front();
    std::memcpy(pvBuffer, front.data(), q->itemSize);
    return pdTRUE;
}

inline int xQueuePeek(QueueHandle_t xQueue, void* pvBuffer, uint32_t xTicksToWait) {
    if (!xQueue || !pvBuffer) return pdFALSE;
    auto* q = static_cast<NativeQueue*>(xQueue);
    if (q->items.empty()) return pdFALSE;
    std::memcpy(pvBuffer, q->items.front().data(), q->itemSize);
    return pdTRUE;
}

inline size_t uxQueueMessagesWaiting(QueueHandle_t xQueue) {
    if (!xQueue) return 0;
    auto* q = static_cast<NativeQueue*>(xQueue);
    return q->items.size();
}

inline void vQueueDelete(QueueHandle_t xQueue) {
    if (!xQueue) return;
    if (g_lastCreatedQueue == xQueue) {
        g_lastCreatedQueue = nullptr;
    }
    auto* q = static_cast<NativeQueue*>(xQueue);
    delete q;
}
