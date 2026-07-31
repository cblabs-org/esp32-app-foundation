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
#include <queue>
#include <vector>
#include "FreeRTOS.h"

typedef void* QueueHandle_t;

struct NativeQueue {
    size_t length;
    size_t itemSize;
    std::queue<std::vector<uint8_t>> items;
};

inline QueueHandle_t xQueueCreate(size_t uxQueueLength, size_t uxItemSize) {
    auto* q = new NativeQueue{uxQueueLength, uxItemSize, {}};
    return static_cast<QueueHandle_t>(q);
}

inline int xQueueSend(QueueHandle_t xQueue, const void* pvItemToQueue, uint32_t xTicksToWait) {
    if (!xQueue || !pvItemToQueue) return pdFALSE;
    auto* q = static_cast<NativeQueue*>(xQueue);
    if (q->items.size() >= q->length) return pdFALSE;
    const uint8_t* ptr = static_cast<const uint8_t*>(pvItemToQueue);
    q->items.push(std::vector<uint8_t>(ptr, ptr + q->itemSize));
    return pdTRUE;
}

inline int xQueueReceive(QueueHandle_t xQueue, void* pvBuffer, uint32_t xTicksToWait) {
    if (!xQueue || !pvBuffer) return pdFALSE;
    auto* q = static_cast<NativeQueue*>(xQueue);
    if (q->items.empty()) return pdFALSE;
    auto front = q->items.front();
    q->items.pop();
    std::memcpy(pvBuffer, front.data(), q->itemSize);
    return pdTRUE;
}

inline void vQueueDelete(QueueHandle_t xQueue) {
    if (!xQueue) return;
    auto* q = static_cast<NativeQueue*>(xQueue);
    delete q;
}
