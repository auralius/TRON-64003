#ifndef CUTE_MODEL_FORMAT_H
#define CUTE_MODEL_FORMAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CUTE_MODEL_FORMAT_VERSION 1u
#define CUTE_MODEL_HEADER_BYTES   1024u
#define CUTE_MODEL_LAYER_COUNT    22u
#define CUTE_MODEL_ARCH_ID        0xE88F28A8u

#define CUTE_MODEL_ARCH_NAME \
    "cute_yolo_fixed_dualcore_hybrid_8_24"

/*
 * Raw nRF52833 flash layout.
 *
 * 0x00000000 .. 0x00062FFF : current firmware / future S113 application area
 * 0x00063000 .. 0x00063FFF : Cute model metadata page
 * 0x00064000 .. 0x0006DFFF : Slot A (40 KiB)
 * 0x0006E000 .. 0x00077FFF : Slot B (40 KiB)
 * 0x00078000 .. 0x0007FFFF : deliberately left free for a future Nordic
 *                              bootloader / MBR parameter / settings layout
 *
 * The fixed .cute package produced by the current Studio is about 28.4 KiB,
 * so each 40-KiB slot has substantial headroom.
 */
#define CUTE_META_ADDR       0x00063000u
#define CUTE_SLOT_A_ADDR     0x00064000u
#define CUTE_SLOT_B_ADDR     0x0006E000u
#define CUTE_SLOT_BYTES      0x0000A000u
#define CUTE_FLASH_PAGE      0x00001000u

typedef struct {
    uint32_t weight_offset;
    uint32_t weight_count;
    uint32_t bias_offset;
    uint32_t bias_count;
    uint32_t multiplier_offset;
    uint32_t shift_offset;
    float input_scale;
    float output_scale;
    int32_t input_zero_point;
    int32_t output_zero_point;
} CuteLayerRecord;

typedef struct {
    char magic[4];
    uint16_t version;
    uint16_t header_bytes;
    uint32_t architecture_id;
    uint32_t total_bytes;
    uint32_t payload_crc32;
    uint16_t layer_count;
    uint16_t reserved0;
    char label[24];
    float confidence_threshold;
    float nms_iou_threshold;
    float min_box_w;
    float min_box_h;
    uint8_t max_detections;
    uint8_t reserved1[3];
    float input_scale;
    int32_t input_zero_point;
    float output_scale;
    int32_t output_zero_point;
    char architecture_name[40];
    uint32_t header_crc32;

    CuteLayerRecord layers[CUTE_MODEL_LAYER_COUNT];

    uint8_t reserved_tail[16];
} CuteModelHeader;

_Static_assert(sizeof(CuteLayerRecord) == 40, "CuteLayerRecord must be 40 bytes");
_Static_assert(sizeof(CuteModelHeader) == CUTE_MODEL_HEADER_BYTES,
               "CuteModelHeader must be 1024 bytes");

typedef enum {
    CUTE_LAYER_S1A = 0,
    CUTE_LAYER_S1B,
    CUTE_LAYER_S2A,
    CUTE_LAYER_S2B,
    CUTE_LAYER_S3A,
    CUTE_LAYER_S3B,

    CUTE_LAYER_H1A,
    CUTE_LAYER_H1DW,
    CUTE_LAYER_H1PW,

    CUTE_LAYER_H2A,
    CUTE_LAYER_H2DW,
    CUTE_LAYER_H2PW,

    CUTE_LAYER_H3A,
    CUTE_LAYER_H3DW,
    CUTE_LAYER_H3PW,

    CUTE_LAYER_H4A,
    CUTE_LAYER_H4DW,
    CUTE_LAYER_H4PW,

    CUTE_LAYER_H5A,
    CUTE_LAYER_H5DW,
    CUTE_LAYER_H5PW,

    CUTE_LAYER_HEAD
} CuteLayerId;

#ifdef __cplusplus
}
#endif

#endif
