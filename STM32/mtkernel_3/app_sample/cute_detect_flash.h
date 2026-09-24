#ifndef CUTE_DETECT_FLASH_H
#define CUTE_DETECT_FLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CUTE_DETECT_MAX_DETECTIONS 32u

typedef struct {
    float confidence;
    float x1;
    float y1;
    float x2;
    float y2;
} CuteDetection;

typedef struct {
    uint8_t count;
    CuteDetection items[CUTE_DETECT_MAX_DETECTIONS];
} CuteDetectionSet;

int cute_detect_flash_decode(
    const int8_t *head,
    uint32_t head_size,
    CuteDetectionSet *result);

#ifdef __cplusplus
}
#endif

#endif
