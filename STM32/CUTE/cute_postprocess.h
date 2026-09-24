#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CUTE_POST_MAX_DETECTIONS 32U

typedef struct
{
    float confidence;
    float x1;
    float y1;
    float x2;
    float y2;
} CuteDetection;

typedef struct
{
    uint8_t count;
    CuteDetection items[CUTE_POST_MAX_DETECTIONS];
} CuteDetectionSet;

/*
 * Decode the current 5x16x16 raw INT8 head produced by cute_runtime_run_gray8().
 * Coordinates are normalized to the exact square detector crop, matching the
 * ESP32 CUTE-YOLO decoder.
 *
 * Returns 1 on success, 0 if the runtime head is unavailable/invalid.
 */
int cute_postprocess_decode(CuteDetectionSet *result);

#ifdef __cplusplus
}
#endif
