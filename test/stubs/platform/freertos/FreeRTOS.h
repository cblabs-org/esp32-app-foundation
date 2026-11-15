#pragma once
#include "task.h"
#include <cstdint>

typedef int BaseType_t;
#define pdMS_TO_TICKS(xTimeInMs) ((xTimeInMs) / 10)
#define pdFALSE 0
#define pdTRUE 1

typedef struct {
    int owner;
} portMUX_TYPE;

#define portMUX_INITIALIZER_UNLOCKED { 0 }
#define portENTER_CRITICAL(mux) do { (void)(mux); } while (0)
#define portEXIT_CRITICAL(mux)  do { (void)(mux); } while (0)
