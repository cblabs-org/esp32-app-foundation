#pragma once
#include <cstdint>

typedef void* SemaphoreHandle_t;
#ifndef pdTRUE
#define pdTRUE 1
#endif
#ifndef pdFALSE
#define pdFALSE 0
#endif
#ifndef portMAX_DELAY
#define portMAX_DELAY 0xFFFFFFFF
#endif

inline SemaphoreHandle_t xSemaphoreCreateMutex() { return (SemaphoreHandle_t)1; }
inline SemaphoreHandle_t xSemaphoreCreateBinary() { return (SemaphoreHandle_t)1; }
inline int xSemaphoreTake(SemaphoreHandle_t xSemaphore, uint32_t xBlockTime) { return pdTRUE; }
inline int xSemaphoreGive(SemaphoreHandle_t xSemaphore) { return pdTRUE; }
inline void vSemaphoreDelete(SemaphoreHandle_t xSemaphore) {}
