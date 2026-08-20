#pragma once
#include <cstdint>

typedef void* TaskHandle_t;
#define pdPASS  1
#define pdTRUE  1
#define pdFALSE 0
typedef uint32_t configSTACK_DEPTH_TYPE;

/** Passed to ulTaskNotifyTake/xTaskNotifyWait to block indefinitely. */
#define portMAX_DELAY 0xFFFFFFFFUL

typedef void (*TaskFunction_t)(void*);

struct StubTaskYieldException {};

inline int xTaskCreateWithCaps(TaskFunction_t pxTaskCode, const char* const pcName,
                               const configSTACK_DEPTH_TYPE usStackDepth, void* const pvParameters,
                               uint32_t uxPriority, TaskHandle_t* const pxCreatedTask,
                               uint32_t uxMemoryCaps) {
    if (pxCreatedTask) {
        // Give the caller a non-null sentinel handle so _taskHandle != nullptr
        // after begin() returns, allowing performUpdate() to pass the guard check.
        *pxCreatedTask = reinterpret_cast<TaskHandle_t>(1);
    }
    try {
        pxTaskCode(pvParameters);
    } catch (const StubTaskYieldException&) {
        // Task blocked (ulTaskNotifyTake / vTaskDelay) — exit cleanly
    }
    return pdPASS;
}

inline int xTaskCreate(TaskFunction_t pxTaskCode, const char* const pcName,
                       const configSTACK_DEPTH_TYPE usStackDepth, void* const pvParameters,
                       uint32_t uxPriority, TaskHandle_t* const pxCreatedTask) {
    return xTaskCreateWithCaps(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask, 0);
}

inline void vTaskDelete(TaskHandle_t xTaskToDelete) {}
inline void vTaskDelay(uint32_t xTicksToDelay) { throw StubTaskYieldException(); }

/**
 * @brief Stub: blocks the persistent OTA task loop in native builds.
 *
 * On hardware, this blocks until xTaskNotifyGive() is called. In native
 * builds the persistent loop must not spin forever, so we throw
 * StubTaskYieldException (the same signal used by vTaskDelay). The
 * xTaskCreateWithCaps stub catches it and the "task" exits cleanly.
 * ulTaskNotifyTake is called once per loop iteration, so each
 * performUpdate()-triggered run executes one pipeline iteration maximum.
 */
inline uint32_t ulTaskNotifyTake(int xClearCountOnExit, uint32_t xTicksToWait) {
    throw StubTaskYieldException();
    return 0;
}

/**
 * @brief Stub: no-op. On hardware, wakes the task blocked in ulTaskNotifyTake.
 * In native builds the task runs synchronously via xTaskCreateWithCaps so
 * no inter-task signalling is needed.
 */
inline void xTaskNotifyGive(TaskHandle_t xTaskToNotify) {}

typedef uint32_t StackType_t;
// Unsigned counterpart of BaseType_t — needed by callers that store counts
// (e.g. stack high water marks) which are never negative on real FreeRTOS.
typedef unsigned int UBaseType_t;
inline TaskHandle_t xTaskGetHandle(const char* pcNameToQuery) { return nullptr; }
inline uint32_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask) { return 1000; }

// Tick counter type. Native tests do not exercise real timing, so a plain
// uint32_t (matching FreeRTOS.h's pdMS_TO_TICKS 10ms-per-tick convention) is
// enough — callers only need a monotonically-nondecreasing value to diff.
typedef uint32_t TickType_t;

/**
 * @brief Stub: returns a fixed tick count. Native tests do not exercise real
 * timing, so callers that gate periodic work on elapsed ticks (e.g.
 * DataManager's stack-watermark telemetry) simply see no elapsed time.
 */
inline TickType_t xTaskGetTickCount() { return 0; }
