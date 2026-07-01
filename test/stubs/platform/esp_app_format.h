#pragma once
#include <stdint.h>

typedef struct {
    uint8_t magic;
} esp_image_header_t;

typedef struct {
    uint8_t dummy;
} esp_image_segment_header_t;
