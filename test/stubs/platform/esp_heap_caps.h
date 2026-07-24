#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <stdlib.h>

#define MALLOC_CAP_8BIT (1<<0)
#define MALLOC_CAP_SPIRAM (1<<1)
#define MALLOC_CAP_INTERNAL (1<<2)
#define MALLOC_CAP_DEFAULT (1<<3)
#define MALLOC_CAP_DMA (1<<4)

inline size_t heap_caps_get_free_size(uint32_t caps) { (void)caps; return 200000; }
inline size_t heap_caps_get_minimum_free_size(uint32_t caps) { (void)caps; return 150000; }
inline size_t heap_caps_get_total_size(uint32_t caps) { (void)caps; return 300000; }
inline size_t heap_caps_get_largest_free_block(uint32_t caps) { (void)caps; return 100000; }

inline void* heap_caps_malloc(size_t size, uint32_t caps) { (void)caps; return malloc(size); }
inline void heap_caps_free(void* ptr) { free(ptr); }
