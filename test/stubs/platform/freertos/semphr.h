#pragma once
#include <cstdint>

typedef void* SemaphoreHandle_t;
#define pdTRUE 1
#define pdFALSE 0
#define portMAX_DELAY 0xFFFFFFFF

inline SemaphoreHandle_t xSemaphoreCreateMutex() { return (SemaphoreHandle_t)1; }
inline int xSemaphoreTake(SemaphoreHandle_t xSemaphore, uint32_t xBlockTime) { return pdTRUE; }
inline int xSemaphoreGive(SemaphoreHandle_t xSemaphore) { return pdTRUE; }
inline void vSemaphoreDelete(SemaphoreHandle_t xSemaphore) {}
