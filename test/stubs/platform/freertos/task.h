#pragma once
#include <cstdint>

typedef void* TaskHandle_t;
#define pdPASS 1
typedef uint32_t configSTACK_DEPTH_TYPE;

typedef void (*TaskFunction_t)(void*);

inline int xTaskCreateWithCaps(TaskFunction_t pxTaskCode, const char* const pcName,
                               const configSTACK_DEPTH_TYPE usStackDepth, void* const pvParameters,
                               uint32_t uxPriority, TaskHandle_t* const pxCreatedTask,
                               uint32_t uxMemoryCaps) {
    pxTaskCode(pvParameters);
    return pdPASS;
}

inline void vTaskDelete(TaskHandle_t xTaskToDelete) {}
inline void vTaskDelay(uint32_t xTicksToDelay) {}
