# Profiling and Measurement

To optimize performance, you must measure it. The framework supports several profiling techniques.

## Measuring Heap

```cpp
size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
ESP_LOGI(TAG, "Free: %zu, Max Block: %zu", free_heap, largest_block);
```
Use this before and after large operations (like JSON parsing or HTTPS fetches) to identify leaks or excessive consumption.

## Measuring Stack Usage

FreeRTOS tasks have a fixed stack size. To check how close a task came to overflowing:

```cpp
UBaseType_t high_water_mark = uxTaskGetStackHighWaterMark(NULL);
ESP_LOGI(TAG, "Task Stack High Water Mark: %u bytes", high_water_mark);
```
*Note: The returned value is in WORDS (4 bytes on ESP32), not bytes, depending on the FreeRTOS port. Check ESP-IDF docs.*

## Execution Timing

For measuring execution duration of `tick()` functions or rendering loops:

```cpp
int64_t start = esp_timer_get_time();
// ... code to measure ...
int64_t end = esp_timer_get_time();
ESP_LOGD(TAG, "Execution took %lld us", (end - start));
```

## FreeRTOS Runtime Stats

If `configGENERATE_RUN_TIME_STATS` is enabled in `sdkconfig`, you can use `vTaskGetRunTimeStats()` to dump CPU utilization per task. This is critical for identifying tasks that are starving the idle task or triggering the Task Watchdog Timer (TWDT).

## See Also
- [ITickable Timing Budget](../interfaces/tickable.md)
