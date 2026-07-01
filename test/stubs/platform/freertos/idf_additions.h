#pragma once
#include "task.h"  // for TaskHandle_t

/**
 * @brief Stub: correct paired destructor for xTaskCreateWithCaps().
 *
 * On hardware, this frees the heap_caps-allocated task stack. In native
 * builds there is no caps allocator, so this is a no-op. Tasks created
 * by the xTaskCreateWithCaps stub run synchronously and return; no real
 * stack memory to free.
 */
inline void vTaskDeleteWithCaps(TaskHandle_t xTaskToDelete) {}
