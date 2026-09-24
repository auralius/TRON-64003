#include "cute_model_manager.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

extern "C" {
// Absolute symbols exported by STM32H743VITX_FLASH.ld.
// Their addresses delimit the dedicated .cute model slot.
extern uint8_t __cute_start__;
extern uint8_t __cute_end__;
}

namespace {

static constexpr uint8_t CUTE_MODEL_RUNTIME_MAX_DETECTIONS = 32;
static const uint8_t *g_model_base = nullptr;
static bool g_ready = false;
static char g_status[96] = "BOOT";

static bool expected_layer_counts(uint16_t index,
                                  uint8_t hybrid_blocks,
                                  uint8_t head_hidden_layers,
                                  uint16_t flags,
                                  uint32_t *weight_count,
                                  uint32_t *bias_count)
{
  if (!weight_count || !bias_count) return false;

  static const uint32_t STEM_WEIGHT_COUNTS[6] = {36, 36, 576, 576, 2304, 2304};
  static const uint32_t STEM_BIAS_COUNTS[6] = {4, 4, 8, 8, 16, 16};

  if (index < 6) {
    *weight_count = STEM_WEIGHT_COUNTS[index];
    *bias_count = STEM_BIAS_COUNTS[index];
    return true;
  }

  const uint16_t head_index = 6u + 4u * hybrid_blocks;
  if (index < head_index) {
    const uint16_t relative = static_cast<uint16_t>(index - CUTE_LAYER_HYBRID_BASE);
    const uint8_t role = static_cast<uint8_t>(relative % 4u);
    switch (role) {
      case 0:
      case 2:
        *weight_count = 9u * 32u;
        *bias_count = 32u;
        return true;
      case 1:
      case 3:
        *weight_count = 32u * 16u;
        *bias_count = 16u;
        return true;
      default:
        return false;
    }
  }

  const bool mlp_head = (flags & CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD) != 0u;
  if (!mlp_head) {
    if (index != head_index) return false;
    *weight_count = 32u * 5u;
    *bias_count = 5u;
    return true;
  }

  if (head_hidden_layers < CUTE_MODEL_MIN_HEAD_HIDDEN_LAYERS ||
      head_hidden_layers > CUTE_MODEL_MAX_HEAD_HIDDEN_LAYERS)
    return false;

  if (index >= head_index &&
      index < static_cast<uint16_t>(head_index + head_hidden_layers)) {
    *weight_count = 32u * 32u;
    *bias_count = 32u;
    return true;
  }

  const uint16_t obj_index = static_cast<uint16_t>(head_index + head_hidden_layers);
  const uint16_t box_index = static_cast<uint16_t>(obj_index + 1u);

  if (index == obj_index) {
    *weight_count = 32u;
    *bias_count = 1u;
    return true;
  }
  if (index == box_index) {
    *weight_count = 32u * 4u;
    *bias_count = 4u;
    return true;
  }
  return false;
}

static uint16_t expected_layer_count(uint8_t hybrid_blocks,
                                     uint8_t head_hidden_layers,
                                     uint16_t flags)
{
  const bool mlp_head = (flags & CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD) != 0u;
  if (!mlp_head) return static_cast<uint16_t>(7u + 4u * hybrid_blocks);
  return static_cast<uint16_t>(8u + 4u * hybrid_blocks + head_hidden_layers);
}

static uint16_t align_header_bytes(uint16_t value)
{
  return static_cast<uint16_t>(
      (value + CUTE_MODEL_HEADER_ALIGNMENT - 1u) &
      ~(CUTE_MODEL_HEADER_ALIGNMENT - 1u));
}

static uint16_t expected_v2_header_bytes(uint8_t hybrid_blocks,
                                         uint8_t head_hidden_layers,
                                         uint16_t flags)
{
  const uint16_t bytes = static_cast<uint16_t>(
      CUTE_MODEL_BASE_BYTES +
      expected_layer_count(hybrid_blocks, head_hidden_layers, flags) *
          CUTE_MODEL_RECORD_BYTES);
  return align_header_bytes(bytes);
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t n)
{
  while (n--) {
    crc ^= *data++;
    for (uint8_t k = 0; k < 8; ++k) {
      const uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return crc;
}

static uint32_t crc32_memory(const uint8_t *data, size_t n)
{
  uint32_t crc = 0xFFFFFFFFu;
  crc = crc32_update(crc, data, n);
  return crc ^ 0xFFFFFFFFu;
}

static uint32_t crc32_header(const uint8_t *base, size_t header_bytes)
{
  static constexpr size_t CRC_FIELD_OFFSET = 124;
  static constexpr size_t CRC_FIELD_BYTES = 4;

  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < header_bytes; ++i) {
    uint8_t b = base[i];
    if (i >= CRC_FIELD_OFFSET && i < CRC_FIELD_OFFSET + CRC_FIELD_BYTES)
      b = 0;
    crc = crc32_update(crc, &b, 1);
  }
  return crc ^ 0xFFFFFFFFu;
}

static bool range_ok(uint32_t offset, uint32_t bytes,
                     uint32_t total_bytes, uint32_t header_bytes)
{
  if (offset < header_bytes) return false;
  if (offset > total_bytes) return false;
  if (bytes > total_bytes - offset) return false;
  return true;
}

static bool validation_fail(char *reason, size_t reason_size,
                            const char *fmt, ...)
{
  if (reason && reason_size > 0) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(reason, reason_size, fmt, args);
    va_end(args);
  }
  return false;
}

static bool validate_model_memory(const uint8_t *base,
                                  size_t capacity,
                                  CuteModelHeaderBase *out_header,
                                  char *reason,
                                  size_t reason_size)
{
  if (!base)
    return validation_fail(reason, reason_size, "base=null");
  if (capacity < CUTE_MODEL_BASE_BYTES)
    return validation_fail(reason, reason_size, "slot too small");

  CuteModelHeaderBase header = {};
  memcpy(&header, base, sizeof(header));

  if (memcmp(header.magic, "CUTE", 4) != 0)
    return validation_fail(reason, reason_size, "magic %.4s", header.magic);

  uint8_t hybrid_blocks = 0;
  uint8_t head_hidden_layers = 0;
  uint16_t expected_header_bytes = 0;
  uint16_t expected_layers = 0;

  if (header.version == CUTE_MODEL_FORMAT_VERSION_V1) {
    return validation_fail(reason, reason_size,
                           "CUTE v1 unsupported by W32 runtime");
  } else if (header.version == CUTE_MODEL_FORMAT_VERSION_V2) {
    hybrid_blocks = header.hybrid_blocks;
    if (hybrid_blocks < CUTE_MODEL_MIN_HYBRID_BLOCKS ||
        hybrid_blocks > CUTE_MODEL_MAX_HYBRID_BLOCKS)
      return validation_fail(reason, reason_size,
                             "H=%u invalid", (unsigned)hybrid_blocks);

    if (header.flags & ~CUTE_MODEL_KNOWN_FLAGS)
      return validation_fail(reason, reason_size,
                             "flags=0x%04X invalid", (unsigned)header.flags);

    const bool mlp_head =
        (header.flags & CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD) != 0u;
    if (mlp_head) {
      head_hidden_layers = header.head_hidden_layers == 0u
                               ? 1u
                               : header.head_hidden_layers;
      if (head_hidden_layers < CUTE_MODEL_MIN_HEAD_HIDDEN_LAYERS ||
          head_hidden_layers > CUTE_MODEL_MAX_HEAD_HIDDEN_LAYERS)
        return validation_fail(reason, reason_size,
                               "M=%u invalid", (unsigned)head_hidden_layers);
    } else {
      head_hidden_layers = 0u;
      if (header.head_hidden_layers != 0u)
        return validation_fail(reason, reason_size,
                               "linear M byte=%u invalid",
                               (unsigned)header.head_hidden_layers);
    }

    expected_header_bytes = expected_v2_header_bytes(
        hybrid_blocks, head_hidden_layers, header.flags);
    expected_layers = expected_layer_count(
        hybrid_blocks, head_hidden_layers, header.flags);

    if (header.architecture_id != CUTE_MODEL_V2_ARCH_ID)
      return validation_fail(reason, reason_size,
                             "arch=%08lX expected=%08lX",
                             (unsigned long)header.architecture_id,
                             (unsigned long)CUTE_MODEL_V2_ARCH_ID);
  } else {
    return validation_fail(reason, reason_size,
                           "version=%u unsupported", (unsigned)header.version);
  }

  if (header.header_bytes != expected_header_bytes)
    return validation_fail(reason, reason_size,
                           "header=%u expected=%u",
                           (unsigned)header.header_bytes,
                           (unsigned)expected_header_bytes);

  if (header.layer_count != expected_layers)
    return validation_fail(reason, reason_size,
                           "layers=%u expected=%u",
                           (unsigned)header.layer_count,
                           (unsigned)expected_layers);

  if (header.layer_count > CUTE_MODEL_MAX_LAYER_COUNT)
    return validation_fail(reason, reason_size, "too many layers");

  if (header.header_bytes > capacity ||
      header.total_bytes < header.header_bytes ||
      header.total_bytes > capacity)
    return validation_fail(reason, reason_size,
                           "size h=%u total=%lu cap=%lu",
                           (unsigned)header.header_bytes,
                           (unsigned long)header.total_bytes,
                           (unsigned long)capacity);

  const uint32_t actual_header_crc = crc32_header(base, header.header_bytes);
  if (actual_header_crc != header.header_crc32)
    return validation_fail(reason, reason_size,
                           "header CRC %08lX != %08lX",
                           (unsigned long)actual_header_crc,
                           (unsigned long)header.header_crc32);

  if (header.max_detections == 0 ||
      header.max_detections > CUTE_MODEL_RUNTIME_MAX_DETECTIONS)
    return validation_fail(reason, reason_size,
                           "max_det=%u invalid", (unsigned)header.max_detections);

  if (!(header.confidence_threshold >= 0.0f &&
        header.confidence_threshold <= 1.0f))
    return validation_fail(reason, reason_size, "confidence invalid");
  if (!(header.nms_iou_threshold >= 0.0f &&
        header.nms_iou_threshold <= 1.0f))
    return validation_fail(reason, reason_size, "nms invalid");
  if (!(header.min_box_w >= 0.0f && header.min_box_w <= 1.0f))
    return validation_fail(reason, reason_size, "min_box_w invalid");
  if (!(header.min_box_h >= 0.0f && header.min_box_h <= 1.0f))
    return validation_fail(reason, reason_size, "min_box_h invalid");

  if (header.label[23] != '\0')
    return validation_fail(reason, reason_size, "label unterminated");
  if (header.architecture_name[39] != '\0')
    return validation_fail(reason, reason_size, "arch name unterminated");

  for (uint16_t i = 0; i < header.layer_count; ++i) {
    CuteLayerRecord r = {};
    const size_t record_offset =
        CUTE_MODEL_BASE_BYTES + static_cast<size_t>(i) * CUTE_MODEL_RECORD_BYTES;
    memcpy(&r, base + record_offset, sizeof(r));

    uint32_t expected_weights = 0;
    uint32_t expected_bias = 0;
    if (!expected_layer_counts(i, hybrid_blocks, head_hidden_layers,
                               header.flags, &expected_weights, &expected_bias))
      return validation_fail(reason, reason_size, "L%u role", (unsigned)i);

    if (r.weight_count != expected_weights)
      return validation_fail(reason, reason_size,
                             "L%u weights=%lu expected=%lu",
                             (unsigned)i,
                             (unsigned long)r.weight_count,
                             (unsigned long)expected_weights);
    if (r.bias_count != expected_bias)
      return validation_fail(reason, reason_size,
                             "L%u bias=%lu expected=%lu",
                             (unsigned)i,
                             (unsigned long)r.bias_count,
                             (unsigned long)expected_bias);

    if (!range_ok(r.weight_offset, r.weight_count,
                  header.total_bytes, header.header_bytes))
      return validation_fail(reason, reason_size, "L%u weight range", (unsigned)i);
    if (!range_ok(r.bias_offset, r.bias_count * sizeof(int32_t),
                  header.total_bytes, header.header_bytes))
      return validation_fail(reason, reason_size, "L%u bias range", (unsigned)i);
    if (!range_ok(r.multiplier_offset, r.bias_count * sizeof(int32_t),
                  header.total_bytes, header.header_bytes))
      return validation_fail(reason, reason_size, "L%u mult range", (unsigned)i);
    if (!range_ok(r.shift_offset, r.bias_count * sizeof(int32_t),
                  header.total_bytes, header.header_bytes))
      return validation_fail(reason, reason_size, "L%u shift range", (unsigned)i);

    if ((r.weight_offset & 0xFu) ||
        (r.bias_offset & 3u) ||
        (r.multiplier_offset & 3u) ||
        (r.shift_offset & 3u))
      return validation_fail(reason, reason_size, "L%u alignment", (unsigned)i);

    if (!(r.input_scale > 0.0f))
      return validation_fail(reason, reason_size, "L%u input scale", (unsigned)i);
    if (!(r.output_scale > 0.0f))
      return validation_fail(reason, reason_size, "L%u output scale", (unsigned)i);
  }

  const uint32_t payload_crc = crc32_memory(
      base + header.header_bytes,
      header.total_bytes - header.header_bytes);
  if (payload_crc != header.payload_crc32)
    return validation_fail(reason, reason_size,
                           "payload CRC %08lX != %08lX",
                           (unsigned long)payload_crc,
                           (unsigned long)header.payload_crc32);

  if (out_header) *out_header = header;
  if (reason && reason_size > 0) snprintf(reason, reason_size, "OK");
  return true;
}

} // namespace

bool cute_model_begin()
{
  const uint8_t *base = &__cute_start__;
  const uintptr_t start = reinterpret_cast<uintptr_t>(&__cute_start__);
  const uintptr_t end = reinterpret_cast<uintptr_t>(&__cute_end__);
  const size_t capacity = (end > start) ? static_cast<size_t>(end - start) : 0u;

  CuteModelHeaderBase header = {};
  char reason[80] = {};

  g_ready = validate_model_memory(base, capacity,
                                  &header, reason, sizeof(reason));
  if (!g_ready) {
    g_model_base = nullptr;
    snprintf(g_status, sizeof(g_status), "NO MODEL: %s", reason);
    return false;
  }

  g_model_base = base;
  const uint8_t m = (header.flags & CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD)
                        ? (header.head_hidden_layers == 0u ? 1u : header.head_hidden_layers)
                        : 0u;
  snprintf(g_status, sizeof(g_status),
           "MODEL %s H%u M%u v%u %luB",
           header.label,
           (unsigned)header.hybrid_blocks,
           (unsigned)m,
           (unsigned)header.version,
           (unsigned long)header.total_bytes);
  return true;
}

bool cute_model_ble_begin(const char *) { return false; }
void cute_model_poll() {}
bool cute_model_ready() { return g_ready && g_model_base != nullptr; }
bool cute_model_uploading() { return false; }
void cute_model_set_inference_busy(bool) {}

const CuteModelHeaderBase *cute_model_header()
{
  return cute_model_ready()
             ? reinterpret_cast<const CuteModelHeaderBase *>(g_model_base)
             : nullptr;
}

const CuteLayerRecord *cute_model_layer(uint16_t index)
{
  const CuteModelHeaderBase *header = cute_model_header();
  if (!header || index >= header->layer_count) return nullptr;
  const size_t offset = CUTE_MODEL_BASE_BYTES +
                        static_cast<size_t>(index) * CUTE_MODEL_RECORD_BYTES;
  if (offset + sizeof(CuteLayerRecord) > header->header_bytes) return nullptr;
  return reinterpret_cast<const CuteLayerRecord *>(g_model_base + offset);
}

const uint8_t *cute_model_base() { return g_model_base; }

const int8_t *cute_model_weight(uint16_t index)
{
  const CuteLayerRecord *r = cute_model_layer(index);
  return r ? reinterpret_cast<const int8_t *>(g_model_base + r->weight_offset) : nullptr;
}

const int32_t *cute_model_bias(uint16_t index)
{
  const CuteLayerRecord *r = cute_model_layer(index);
  return r ? reinterpret_cast<const int32_t *>(g_model_base + r->bias_offset) : nullptr;
}

const int32_t *cute_model_multiplier(uint16_t index)
{
  const CuteLayerRecord *r = cute_model_layer(index);
  return r ? reinterpret_cast<const int32_t *>(g_model_base + r->multiplier_offset) : nullptr;
}

const int32_t *cute_model_shift(uint16_t index)
{
  const CuteLayerRecord *r = cute_model_layer(index);
  return r ? reinterpret_cast<const int32_t *>(g_model_base + r->shift_offset) : nullptr;
}

uint16_t cute_model_format_version()
{
  const CuteModelHeaderBase *h = cute_model_header();
  return h ? h->version : 0;
}
uint16_t cute_model_header_bytes()
{
  const CuteModelHeaderBase *h = cute_model_header();
  return h ? h->header_bytes : 0;
}
uint16_t cute_model_layer_count()
{
  const CuteModelHeaderBase *h = cute_model_header();
  return h ? h->layer_count : 0;
}
uint8_t cute_model_hybrid_blocks()
{
  const CuteModelHeaderBase *h = cute_model_header();
  if (!h) return 0;
  if (h->version == CUTE_MODEL_FORMAT_VERSION_V1) return CUTE_MODEL_V1_HYBRID_BLOCKS;
  if (h->version == CUTE_MODEL_FORMAT_VERSION_V2) return h->hybrid_blocks;
  return 0;
}
bool cute_model_has_mlp_head()
{
  const CuteModelHeaderBase *h = cute_model_header();
  return h && h->version == CUTE_MODEL_FORMAT_VERSION_V2 &&
         (h->flags & CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD) != 0u;
}
uint8_t cute_model_head_hidden_layers()
{
  const CuteModelHeaderBase *h = cute_model_header();
  if (!h || !cute_model_has_mlp_head()) return 0u;
  const uint8_t m = h->head_hidden_layers == 0u ? 1u : h->head_hidden_layers;
  if (m < CUTE_MODEL_MIN_HEAD_HIDDEN_LAYERS ||
      m > CUTE_MODEL_MAX_HEAD_HIDDEN_LAYERS) return 0u;
  return m;
}
uint16_t cute_model_head_layer_index()
{
  const uint8_t n = cute_model_hybrid_blocks();
  return n ? static_cast<uint16_t>(6u + 4u * n) : 0xFFFFu;
}
uint16_t cute_model_head_hidden_layer_index(uint8_t hidden_index)
{
  const uint16_t base = cute_model_head_layer_index();
  const uint8_t m = cute_model_head_hidden_layers();
  if (base == 0xFFFFu || !m || hidden_index >= m) return 0xFFFFu;
  return static_cast<uint16_t>(base + hidden_index);
}
uint16_t cute_model_head_objectness_layer_index()
{
  const uint16_t base = cute_model_head_layer_index();
  const uint8_t m = cute_model_head_hidden_layers();
  if (base == 0xFFFFu || !m) return 0xFFFFu;
  return static_cast<uint16_t>(base + m);
}
uint16_t cute_model_head_box_layer_index()
{
  const uint16_t obj = cute_model_head_objectness_layer_index();
  return obj == 0xFFFFu ? 0xFFFFu : static_cast<uint16_t>(obj + 1u);
}

const char *cute_model_label()
{
  const CuteModelHeaderBase *h = cute_model_header();
  return h ? h->label : "none";
}
float cute_model_confidence_threshold()
{
  const CuteModelHeaderBase *h = cute_model_header();
  return h ? h->confidence_threshold : 0.50f;
}
float cute_model_nms_iou_threshold()
{
  const CuteModelHeaderBase *h = cute_model_header();
  return h ? h->nms_iou_threshold : 0.35f;
}
float cute_model_min_box_w()
{
  const CuteModelHeaderBase *h = cute_model_header();
  return h ? h->min_box_w : 0.05f;
}
float cute_model_min_box_h()
{
  const CuteModelHeaderBase *h = cute_model_header();
  return h ? h->min_box_h : 0.05f;
}
uint8_t cute_model_max_detections()
{
  const CuteModelHeaderBase *h = cute_model_header();
  return h ? h->max_detections : CUTE_MODEL_RUNTIME_MAX_DETECTIONS;
}
const char *cute_model_status() { return g_status; }
