#pragma once
#include <cstdint>
#include <cstddef>

#define MALLOC_CAP_8BIT (1<<0)
#define MALLOC_CAP_SPIRAM (1<<1)
#define MALLOC_CAP_INTERNAL (1<<2)
#define MALLOC_CAP_DEFAULT (1<<3)

inline size_t heap_caps_get_free_size(uint32_t caps) { return 200000; }
inline size_t heap_caps_get_minimum_free_size(uint32_t caps) { return 150000; }
inline size_t heap_caps_get_total_size(uint32_t caps) { return 300000; }
inline size_t heap_caps_get_largest_free_block(uint32_t caps) { return 100000; }
