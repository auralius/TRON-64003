#ifndef CUTE_MODEL_FORMAT_H
#define CUTE_MODEL_FORMAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * CUTE model package contract used by the current Studio.
 *
 * v2 keeps a stable 128-byte base header and stores H/M explicitly:
 *   byte 65 = hybrid block count H
 *   byte 66 = MLP head hidden depth M
 *
 * Layer records begin at byte 128 and remain 40 bytes each.  The complete
 * header size is therefore dynamic and 16-byte aligned.
 * ------------------------------------------------------------------------- */
#define CUTE_MODEL_FORMAT_VERSION_V1  1u
#define CUTE_MODEL_FORMAT_VERSION_V2  2u
#define CUTE_MODEL_BASE_BYTES         128u
#define CUTE_MODEL_V1_HEADER_BYTES    1024u
#define CUTE_MODEL_RECORD_BYTES       40u
#define CUTE_MODEL_HEADER_ALIGNMENT   16u

#define CUTE_MODEL_V1_HYBRID_BLOCKS   5u
#define CUTE_MODEL_MIN_HYBRID_BLOCKS  1u
#define CUTE_MODEL_MAX_HYBRID_BLOCKS  12u
#define CUTE_MODEL_MIN_HEAD_HIDDEN_LAYERS 1u
#define CUTE_MODEL_MAX_HEAD_HIDDEN_LAYERS 12u

#define CUTE_MODEL_V1_LAYER_COUNT     22u
#define CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD 0x0001u
#define CUTE_MODEL_KNOWN_FLAGS        CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD

#define CUTE_MODEL_MAX_LAYER_COUNT \
    (8u + 4u * CUTE_MODEL_MAX_HYBRID_BLOCKS + \
     CUTE_MODEL_MAX_HEAD_HIDDEN_LAYERS)

#define CUTE_MODEL_V1_ARCH_ID         0xE88F28A8u
#define CUTE_MODEL_V2_ARCH_ID         0xB3E82A71u
#define CUTE_MODEL_V2_ARCH_NAME       "cute_yolo_1_nhybrid_dualdwpw_32"

/*
 * Raw nRF52833 flash layout.
 *
 * 0x00000000 .. 0x00062FFF : current firmware / reserved application area
 * 0x00063000 .. 0x00063FFF : Cute model metadata page
 * 0x00064000 .. 0x0006DFFF : Slot A (40 KiB)
 * 0x0006E000 .. 0x00077FFF : Slot B (40 KiB)
 * 0x00078000 .. 0x0007FFFF : deliberately left free for future Nordic use
 *
 * H6/M1 and H6/M2 packages are currently ~26--28 KiB and fit comfortably.
 * The updater still checks the actual package size against the 40-KiB slot.
 */
#define CUTE_META_ADDR       0x00063000u
#define CUTE_SLOT_A_ADDR     0x00064000u
#define CUTE_SLOT_B_ADDR     0x0006E000u
#define CUTE_SLOT_BYTES      0x0000A000u
#define CUTE_FLASH_PAGE      0x00001000u

#if defined(__GNUC__)
#define CUTE_PACKED __attribute__((packed))
#else
#define CUTE_PACKED
#endif

typedef struct CUTE_PACKED {
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

/* Stable first 128 bytes of both legacy and current package headers. */
typedef struct CUTE_PACKED {
    char magic[4];
    uint16_t version;
    uint16_t header_bytes;
    uint32_t architecture_id;
    uint32_t total_bytes;
    uint32_t payload_crc32;
    uint16_t layer_count;
    uint16_t flags;
    char label[24];
    float confidence_threshold;
    float nms_iou_threshold;
    float min_box_w;
    float min_box_h;
    uint8_t max_detections;
    uint8_t hybrid_blocks;
    uint8_t head_hidden_layers;
    uint8_t reserved0;
    float input_scale;
    int32_t input_zero_point;
    float output_scale;
    int32_t output_zero_point;
    char architecture_name[40];
    uint32_t header_crc32;
} CuteModelHeaderBase;

/* Backward source-level alias used by detector code. */
typedef CuteModelHeaderBase CuteModelHeader;

#if defined(__cplusplus)
static_assert(sizeof(CuteLayerRecord) == CUTE_MODEL_RECORD_BYTES,
              "CuteLayerRecord must be 40 bytes");
static_assert(sizeof(CuteModelHeaderBase) == CUTE_MODEL_BASE_BYTES,
              "CuteModelHeaderBase must be 128 bytes");
#else
_Static_assert(sizeof(CuteLayerRecord) == CUTE_MODEL_RECORD_BYTES,
               "CuteLayerRecord must be 40 bytes");
_Static_assert(sizeof(CuteModelHeaderBase) == CUTE_MODEL_BASE_BYTES,
               "CuteModelHeaderBase must be 128 bytes");
#endif

/* Stable stem indices. Hybrids and head indices are derived from H and M. */
enum {
    CUTE_LAYER_S1A = 0,
    CUTE_LAYER_S1B = 1,
    CUTE_LAYER_S2A = 2,
    CUTE_LAYER_S2B = 3,
    CUTE_LAYER_S3A = 4,
    CUTE_LAYER_S3B = 5,
    CUTE_LAYER_HYBRID_BASE = 6
};

typedef uint16_t CuteLayerId;

#ifdef __cplusplus
}
#endif

#endif
