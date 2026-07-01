#pragma once
#include <cstdint>

typedef void* TimerHandle_t;
typedef void (*TimerCallbackFunction_t)(TimerHandle_t);
#define pdPASS 1

inline TimerHandle_t xTimerCreate(const char* const pcTimerName, const uint32_t xTimerPeriodInTicks,
                                  const uint32_t uxAutoReload, void* const pvTimerID,
                                  TimerCallbackFunction_t pxCallbackFunction) {
    return (TimerHandle_t)1;
}

inline int xTimerStart(TimerHandle_t xTimer, uint32_t xTicksToWait) { return pdPASS; }
