#pragma once

#include <stdint.h>
#include <stddef.h>

// -----------------------------------------------------------------------------
// CUTE model package versions / geometry
// Ported from the latest ESP32 firmware, with storage replaced by memory-mapped
// STM32H743 internal flash.
// -----------------------------------------------------------------------------

static constexpr uint16_t CUTE_MODEL_FORMAT_VERSION_V1 = 1;
static constexpr uint16_t CUTE_MODEL_FORMAT_VERSION_V2 = 2;

static constexpr uint16_t CUTE_MODEL_BASE_BYTES = 128;
static constexpr uint16_t CUTE_MODEL_V1_HEADER_BYTES = 1024;
static constexpr uint16_t CUTE_MODEL_RECORD_BYTES = 40;
static constexpr uint16_t CUTE_MODEL_HEADER_ALIGNMENT = 16;

static constexpr uint8_t CUTE_MODEL_V1_HYBRID_BLOCKS = 5;
static constexpr uint8_t CUTE_MODEL_MIN_HYBRID_BLOCKS = 1;
static constexpr uint8_t CUTE_MODEL_MAX_HYBRID_BLOCKS = 12;
static constexpr uint8_t CUTE_MODEL_MIN_HEAD_HIDDEN_LAYERS = 1;
static constexpr uint8_t CUTE_MODEL_MAX_HEAD_HIDDEN_LAYERS = 12;

static constexpr uint16_t CUTE_MODEL_V1_LAYER_COUNT = 22;

static constexpr uint16_t CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD = 0x0001u;
static constexpr uint16_t CUTE_MODEL_KNOWN_FLAGS =
    CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD;

static constexpr uint16_t CUTE_MODEL_MAX_LAYER_COUNT =
    8u + 4u * CUTE_MODEL_MAX_HYBRID_BLOCKS
    + CUTE_MODEL_MAX_HEAD_HIDDEN_LAYERS;

static constexpr uint32_t CUTE_MODEL_V1_ARCH_ID = 0xE88F28A8u;
static constexpr uint32_t CUTE_MODEL_V2_ARCH_ID = 0xB3E82A71u;

// Storage is defined by the linker script, not by a hard-coded runtime pointer.
// The supplied STM32H743VITX_FLASH_CUTE.ld exports:
//   __cute_start__ = 0x081E0000
//   __cute_end__   = 0x08200000
// and leaves the final 128 KiB flash sector exclusively for the .cute package.
// cute_model_begin() uses those linker symbols directly.

struct __attribute__((packed)) CuteLayerRecord {
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
};

struct __attribute__((packed)) CuteModelHeaderBase {
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
};

static_assert(sizeof(CuteLayerRecord) == CUTE_MODEL_RECORD_BYTES,
              "CuteLayerRecord must be 40 bytes");
static_assert(sizeof(CuteModelHeaderBase) == CUTE_MODEL_BASE_BYTES,
              "CuteModelHeaderBase must be 128 bytes");

enum CuteFixedLayerIndex : uint16_t {
  CUTE_LAYER_S1A = 0,
  CUTE_LAYER_S1B = 1,
  CUTE_LAYER_S2A = 2,
  CUTE_LAYER_S2B = 3,
  CUTE_LAYER_S3A = 4,
  CUTE_LAYER_S3B = 5,
  CUTE_LAYER_HYBRID_BASE = 6,
};

// Same accessor surface as the ESP32 model manager. BLE/update functions are
// harmless stubs on this first STM32 port so the runtime interface stays stable.
bool cute_model_begin();
bool cute_model_ble_begin(const char *device_name = "Cute-YOLO");
void cute_model_poll();

bool cute_model_ready();
bool cute_model_uploading();
void cute_model_set_inference_busy(bool busy);

const CuteModelHeaderBase *cute_model_header();
const CuteLayerRecord *cute_model_layer(uint16_t index);
const uint8_t *cute_model_base();

const int8_t *cute_model_weight(uint16_t index);
const int32_t *cute_model_bias(uint16_t index);
const int32_t *cute_model_multiplier(uint16_t index);
const int32_t *cute_model_shift(uint16_t index);

uint16_t cute_model_format_version();
uint16_t cute_model_header_bytes();
uint16_t cute_model_layer_count();
uint8_t cute_model_hybrid_blocks();
bool cute_model_has_mlp_head();
uint8_t cute_model_head_hidden_layers();
uint16_t cute_model_head_layer_index();
uint16_t cute_model_head_hidden_layer_index(uint8_t hidden_index);
uint16_t cute_model_head_objectness_layer_index();
uint16_t cute_model_head_box_layer_index();

const char *cute_model_label();
float cute_model_confidence_threshold();
float cute_model_nms_iou_threshold();
float cute_model_min_box_w();
float cute_model_min_box_h();
uint8_t cute_model_max_detections();
const char *cute_model_status();
