#include "cute_model_flash.h"

#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------
 * Current runtime contract: CUTE v2, configurable H/M, W=32.
 *
 * Layer order:
 *   0..5                split stems S1A/S1B/S2A/S2B/S3A/S3B
 *   6..(6+4H-1)         H1..HN: DWA, PWA, DWB, PWB
 *   6+4H .. +M-1        optional MLP hidden 32->32 layers
 *   6+4H+M              objectness 32->1
 *   6+4H+M+1            box 32->4
 *
 * A linear v2 head (flags=0) is also accepted as one 32->5 record.
 * Legacy v1 8+24 packages are intentionally rejected by this firmware.
 * ------------------------------------------------------------ */

static int expected_layer_counts(
    uint16_t index,
    uint8_t hybrid_blocks,
    uint8_t head_hidden_layers,
    uint16_t flags,
    uint32_t *weight_count,
    uint32_t *bias_count)
{
    static const uint32_t STEM_WEIGHT_COUNTS[6] = {
        36u, 36u, 576u, 576u, 2304u, 2304u
    };
    static const uint32_t STEM_BIAS_COUNTS[6] = {
        4u, 4u, 8u, 8u, 16u, 16u
    };

    if (!weight_count || !bias_count) return 0;

    if (index < 6u) {
        *weight_count = STEM_WEIGHT_COUNTS[index];
        *bias_count = STEM_BIAS_COUNTS[index];
        return 1;
    }

    const uint16_t head_index =
        (uint16_t)(6u + 4u * (uint16_t)hybrid_blocks);

    if (index < head_index) {
        const uint16_t relative =
            (uint16_t)(index - CUTE_LAYER_HYBRID_BASE);
        const uint8_t role = (uint8_t)(relative & 3u);

        switch (role) {
        case 0u: /* DWA: DW3x3 32->32 */
        case 2u: /* DWB: DW3x3 32->32 */
            *weight_count = 9u * 32u;
            *bias_count = 32u;
            return 1;

        case 1u: /* PWA: PW1x1 32->16 */
        case 3u: /* PWB: PW1x1 32->16 */
            *weight_count = 32u * 16u;
            *bias_count = 16u;
            return 1;

        default:
            return 0;
        }
    }

    const int mlp_head =
        (flags & CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD) != 0u;

    if (!mlp_head) {
        if (index != head_index) return 0;
        *weight_count = 32u * 5u;
        *bias_count = 5u;
        return 1;
    }

    if (head_hidden_layers < CUTE_MODEL_MIN_HEAD_HIDDEN_LAYERS ||
        head_hidden_layers > CUTE_MODEL_MAX_HEAD_HIDDEN_LAYERS) {
        return 0;
    }

    if (index >= head_index &&
        index < (uint16_t)(head_index + head_hidden_layers)) {
        *weight_count = 32u * 32u;
        *bias_count = 32u;
        return 1;
    }

    const uint16_t obj_index =
        (uint16_t)(head_index + head_hidden_layers);
    const uint16_t box_index = (uint16_t)(obj_index + 1u);

    if (index == obj_index) {
        *weight_count = 32u;
        *bias_count = 1u;
        return 1;
    }

    if (index == box_index) {
        *weight_count = 32u * 4u;
        *bias_count = 4u;
        return 1;
    }

    return 0;
}

static uint16_t expected_layer_count(
    uint8_t hybrid_blocks,
    uint8_t head_hidden_layers,
    uint16_t flags)
{
    if ((flags & CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD) == 0u) {
        return (uint16_t)(7u + 4u * (uint16_t)hybrid_blocks);
    }

    return (uint16_t)(
        8u + 4u * (uint16_t)hybrid_blocks + head_hidden_layers
    );
}

static uint16_t align_header_bytes(uint16_t value)
{
    return (uint16_t)(
        (value + CUTE_MODEL_HEADER_ALIGNMENT - 1u) &
        ~(CUTE_MODEL_HEADER_ALIGNMENT - 1u)
    );
}

static uint16_t expected_v2_header_bytes(
    uint8_t hybrid_blocks,
    uint8_t head_hidden_layers,
    uint16_t flags)
{
    const uint16_t records =
        expected_layer_count(
            hybrid_blocks,
            head_hidden_layers,
            flags);

    return align_header_bytes(
        (uint16_t)(
            CUTE_MODEL_BASE_BYTES +
            records * CUTE_MODEL_RECORD_BYTES
        )
    );
}

/* ------------------------------------------------------------
 * nRF52833 NVMC backend.
 *
 * IMPORTANT: this backend is for the non-SoftDevice μT-Kernel state.
 * If model upload is later performed while a Nordic SoftDevice is active,
 * erase/write must be routed through the SoftDevice flash API instead.
 * ------------------------------------------------------------ */
#define NVMC_BASE           0x4001E000u
#define NVMC_READY          (*(volatile uint32_t *)(NVMC_BASE + 0x400u))
#define NVMC_CONFIG         (*(volatile uint32_t *)(NVMC_BASE + 0x504u))
#define NVMC_ERASEPAGE      (*(volatile uint32_t *)(NVMC_BASE + 0x508u))

#define NVMC_CONFIG_REN     0u
#define NVMC_CONFIG_WEN     1u
#define NVMC_CONFIG_EEN     2u

static void nvmc_wait_ready(void)
{
    while (NVMC_READY == 0u) {
    }
}

static void nvmc_read_mode(void)
{
    nvmc_wait_ready();
    NVMC_CONFIG = NVMC_CONFIG_REN;
    nvmc_wait_ready();
}

static int nvmc_erase_page(uint32_t address)
{
    if ((address & (CUTE_FLASH_PAGE - 1u)) != 0u) {
        return -1;
    }

    nvmc_wait_ready();
    NVMC_CONFIG = NVMC_CONFIG_EEN;
    nvmc_wait_ready();

    NVMC_ERASEPAGE = address;
    nvmc_wait_ready();

    nvmc_read_mode();
    return 0;
}

static int nvmc_write_word(uint32_t address, uint32_t word)
{
    if (address & 3u) {
        return -1;
    }

    nvmc_wait_ready();
    NVMC_CONFIG = NVMC_CONFIG_WEN;
    nvmc_wait_ready();

    *(volatile uint32_t *)(uintptr_t)address = word;
    nvmc_wait_ready();

    nvmc_read_mode();
    return 0;
}

/* ------------------------------------------------------------ */

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t n)
{
    while (n--) {
        crc ^= *data++;

        for (uint8_t k = 0; k < 8; ++k) {
            const uint32_t mask =
                (uint32_t)-(int32_t)(crc & 1u);

            crc =
                (crc >> 1) ^
                (0xEDB88320u & mask);
        }
    }

    return crc;
}

static uint32_t crc32_memory(const uint8_t *data, uint32_t n)
{
    uint32_t crc = 0xFFFFFFFFu;
    crc = crc32_update(crc, data, n);
    return crc ^ 0xFFFFFFFFu;
}

static uint32_t crc32_header_flash(uint32_t base, uint16_t header_bytes)
{
    /* header_crc32 is fixed at bytes 124..127 in the 128-byte base. */
    static const uint8_t zeros[4] = {0, 0, 0, 0};

    if (header_bytes < CUTE_MODEL_BASE_BYTES) return 0u;

    uint32_t crc = 0xFFFFFFFFu;

    crc = crc32_update(
        crc,
        (const uint8_t *)(uintptr_t)base,
        124u);

    crc = crc32_update(crc, zeros, 4u);

    if (header_bytes > CUTE_MODEL_BASE_BYTES) {
        crc = crc32_update(
            crc,
            (const uint8_t *)(uintptr_t)(base + CUTE_MODEL_BASE_BYTES),
            (uint32_t)header_bytes - CUTE_MODEL_BASE_BYTES);
    }

    return crc ^ 0xFFFFFFFFu;
}

static int range_ok(
    uint32_t offset,
    uint32_t bytes,
    uint32_t total,
    uint32_t header_bytes)
{
    if (offset < header_bytes) return 0;
    if (offset > total) return 0;
    if (bytes > total - offset) return 0;
    return 1;
}

static int quant_equal(
    float scale_a,
    int32_t zp_a,
    float scale_b,
    int32_t zp_b)
{
    /* These values are copied from the same TFLite tensor boundaries by the
     * Studio exporter, so bit-identical float equality is intentional here.
     * Raw INT8 concatenation is only valid when both scale and zero point
     * agree exactly. */
    return scale_a == scale_b && zp_a == zp_b;
}

static const CuteLayerRecord *record_at(uint32_t base, uint16_t index)
{
    return (const CuteLayerRecord *)(uintptr_t)(
        base + CUTE_MODEL_BASE_BYTES +
        (uint32_t)index * CUTE_MODEL_RECORD_BYTES
    );
}

static int validate_model_at(uint32_t base)
{
    if (base != CUTE_SLOT_A_ADDR &&
        base != CUTE_SLOT_B_ADDR) {
        return 0;
    }

    const CuteModelHeaderBase *h =
        (const CuteModelHeaderBase *)(uintptr_t)base;

    if (memcmp(h->magic, "CUTE", 4) != 0) return 0;

    /* Current μT-Kernel runtime executes only the folded-PW W32 v2 family. */
    if (h->version != CUTE_MODEL_FORMAT_VERSION_V2 ||
        h->architecture_id != CUTE_MODEL_V2_ARCH_ID) {
        return 0;
    }

    const uint8_t hybrid_blocks = h->hybrid_blocks;

    if (hybrid_blocks < CUTE_MODEL_MIN_HYBRID_BLOCKS ||
        hybrid_blocks > CUTE_MODEL_MAX_HYBRID_BLOCKS) {
        return 0;
    }

    if (h->flags & ~CUTE_MODEL_KNOWN_FLAGS) return 0;

    uint8_t head_hidden_layers = 0u;

    if (h->flags & CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD) {
        /* Early v2 split-head packages encoded M=1 as zero. */
        head_hidden_layers =
            h->head_hidden_layers == 0u
                ? 1u
                : h->head_hidden_layers;

        if (head_hidden_layers < CUTE_MODEL_MIN_HEAD_HIDDEN_LAYERS ||
            head_hidden_layers > CUTE_MODEL_MAX_HEAD_HIDDEN_LAYERS) {
            return 0;
        }
    } else {
        if (h->head_hidden_layers != 0u) return 0;
    }

    const uint16_t expected_layers =
        expected_layer_count(
            hybrid_blocks,
            head_hidden_layers,
            h->flags);

    const uint16_t expected_header =
        expected_v2_header_bytes(
            hybrid_blocks,
            head_hidden_layers,
            h->flags);

    if (h->layer_count != expected_layers ||
        h->layer_count > CUTE_MODEL_MAX_LAYER_COUNT ||
        h->header_bytes != expected_header) {
        return 0;
    }

    if (h->header_bytes > CUTE_SLOT_BYTES ||
        h->total_bytes < h->header_bytes ||
        h->total_bytes > CUTE_SLOT_BYTES) {
        return 0;
    }

    if (h->label[23] != '\0' ||
        h->architecture_name[39] != '\0') {
        return 0;
    }

    if (!(h->confidence_threshold >= 0.0f &&
          h->confidence_threshold <= 1.0f)) return 0;

    if (!(h->nms_iou_threshold >= 0.0f &&
          h->nms_iou_threshold <= 1.0f)) return 0;

    if (!(h->min_box_w >= 0.0f &&
          h->min_box_w <= 1.0f)) return 0;

    if (!(h->min_box_h >= 0.0f &&
          h->min_box_h <= 1.0f)) return 0;

    if (h->max_detections == 0u ||
        h->max_detections > 32u) return 0;

    if (!(h->input_scale > 0.0f) ||
        !(h->output_scale > 0.0f)) return 0;

    if (crc32_header_flash(base, h->header_bytes) !=
        h->header_crc32) {
        return 0;
    }

    for (uint16_t i = 0u;
         i < h->layer_count;
         ++i) {

        const uint32_t record_addr =
            base + CUTE_MODEL_BASE_BYTES +
            (uint32_t)i * CUTE_MODEL_RECORD_BYTES;

        const CuteLayerRecord *r =
            (const CuteLayerRecord *)(uintptr_t)record_addr;

        uint32_t expected_weights = 0u;
        uint32_t expected_biases = 0u;

        if (!expected_layer_counts(
                i,
                hybrid_blocks,
                head_hidden_layers,
                h->flags,
                &expected_weights,
                &expected_biases)) {
            return 0;
        }

        if (r->weight_count != expected_weights ||
            r->bias_count != expected_biases) {
            return 0;
        }

        if (!range_ok(
                r->weight_offset,
                r->weight_count,
                h->total_bytes,
                h->header_bytes) ||
            !range_ok(
                r->bias_offset,
                r->bias_count * 4u,
                h->total_bytes,
                h->header_bytes) ||
            !range_ok(
                r->multiplier_offset,
                r->bias_count * 4u,
                h->total_bytes,
                h->header_bytes) ||
            !range_ok(
                r->shift_offset,
                r->bias_count * 4u,
                h->total_bytes,
                h->header_bytes)) {
            return 0;
        }

        if ((r->bias_offset & 3u) ||
            (r->multiplier_offset & 3u) ||
            (r->shift_offset & 3u)) {
            return 0;
        }

        if (!(r->input_scale > 0.0f) ||
            !(r->output_scale > 0.0f)) {
            return 0;
        }
    }

    /* --------------------------------------------------------
     * Validate every raw-INT8 tensor boundary used by the runtime.
     * The micro:bit concatenates split branches without requantization,
     * therefore matching quantization is part of the package contract.
     * -------------------------------------------------------- */
    const CuteLayerRecord *s1a = record_at(base, CUTE_LAYER_S1A);
    const CuteLayerRecord *s1b = record_at(base, CUTE_LAYER_S1B);
    const CuteLayerRecord *s2a = record_at(base, CUTE_LAYER_S2A);
    const CuteLayerRecord *s2b = record_at(base, CUTE_LAYER_S2B);
    const CuteLayerRecord *s3a = record_at(base, CUTE_LAYER_S3A);
    const CuteLayerRecord *s3b = record_at(base, CUTE_LAYER_S3B);

    if (!quant_equal(
            h->input_scale, h->input_zero_point,
            s1a->input_scale, s1a->input_zero_point) ||
        !quant_equal(
            s1a->input_scale, s1a->input_zero_point,
            s1b->input_scale, s1b->input_zero_point) ||
        !quant_equal(
            s1a->output_scale, s1a->output_zero_point,
            s1b->output_scale, s1b->output_zero_point) ||
        !quant_equal(
            s1a->output_scale, s1a->output_zero_point,
            s2a->input_scale, s2a->input_zero_point) ||
        !quant_equal(
            s2a->input_scale, s2a->input_zero_point,
            s2b->input_scale, s2b->input_zero_point) ||
        !quant_equal(
            s2a->output_scale, s2a->output_zero_point,
            s2b->output_scale, s2b->output_zero_point) ||
        !quant_equal(
            s2a->output_scale, s2a->output_zero_point,
            s3a->input_scale, s3a->input_zero_point) ||
        !quant_equal(
            s3a->input_scale, s3a->input_zero_point,
            s3b->input_scale, s3b->input_zero_point) ||
        !quant_equal(
            s3a->output_scale, s3a->output_zero_point,
            s3b->output_scale, s3b->output_zero_point)) {
        return 0;
    }

    float previous_scale = s3a->output_scale;
    int32_t previous_zp = s3a->output_zero_point;

    for (uint8_t block = 0u; block < hybrid_blocks; ++block) {
        const uint16_t first = (uint16_t)(
            CUTE_LAYER_HYBRID_BASE + 4u * (uint16_t)block
        );
        const CuteLayerRecord *dwa = record_at(base, first + 0u);
        const CuteLayerRecord *pwa = record_at(base, first + 1u);
        const CuteLayerRecord *dwb = record_at(base, first + 2u);
        const CuteLayerRecord *pwb = record_at(base, first + 3u);

        if (!quant_equal(
                previous_scale, previous_zp,
                dwa->input_scale, dwa->input_zero_point) ||
            !quant_equal(
                previous_scale, previous_zp,
                dwb->input_scale, dwb->input_zero_point) ||
            !quant_equal(
                dwa->output_scale, dwa->output_zero_point,
                pwa->input_scale, pwa->input_zero_point) ||
            !quant_equal(
                dwb->output_scale, dwb->output_zero_point,
                pwb->input_scale, pwb->input_zero_point) ||
            !quant_equal(
                pwa->output_scale, pwa->output_zero_point,
                pwb->output_scale, pwb->output_zero_point)) {
            return 0;
        }

        previous_scale = pwa->output_scale;
        previous_zp = pwa->output_zero_point;
    }

    const uint16_t head_index = (uint16_t)(
        CUTE_LAYER_HYBRID_BASE + 4u * (uint16_t)hybrid_blocks
    );

    if (h->flags & CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD) {
        for (uint8_t hidden = 0u; hidden < head_hidden_layers; ++hidden) {
            const CuteLayerRecord *r = record_at(
                base, (uint16_t)(head_index + hidden)
            );
            if (!quant_equal(
                    previous_scale, previous_zp,
                    r->input_scale, r->input_zero_point)) {
                return 0;
            }
            previous_scale = r->output_scale;
            previous_zp = r->output_zero_point;
        }

        const CuteLayerRecord *obj = record_at(
            base, (uint16_t)(head_index + head_hidden_layers)
        );
        const CuteLayerRecord *box = record_at(
            base, (uint16_t)(head_index + head_hidden_layers + 1u)
        );

        if (!quant_equal(
                previous_scale, previous_zp,
                obj->input_scale, obj->input_zero_point) ||
            !quant_equal(
                previous_scale, previous_zp,
                box->input_scale, box->input_zero_point) ||
            !quant_equal(
                obj->output_scale, obj->output_zero_point,
                box->output_scale, box->output_zero_point) ||
            !quant_equal(
                h->output_scale, h->output_zero_point,
                obj->output_scale, obj->output_zero_point)) {
            return 0;
        }
    } else {
        const CuteLayerRecord *head = record_at(base, head_index);
        if (!quant_equal(
                previous_scale, previous_zp,
                head->input_scale, head->input_zero_point) ||
            !quant_equal(
                h->output_scale, h->output_zero_point,
                head->output_scale, head->output_zero_point)) {
            return 0;
        }
    }

    const uint8_t *payload =
        (const uint8_t *)(uintptr_t)(base + h->header_bytes);

    const uint32_t payload_n =
        h->total_bytes - h->header_bytes;

    if (crc32_memory(payload, payload_n) != h->payload_crc32) {
        return 0;
    }

    return 1;
}

/* ------------------------------------------------------------
 * Tiny append-only metadata log, one flash page.
 * ------------------------------------------------------------ */
#define META_MAGIC 0x41544D43u  /* "CMTA" little endian */

typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint32_t active_slot;
    uint32_t crc32;
} CuteMetaRecord;

_Static_assert(sizeof(CuteMetaRecord) == 16, "metadata record size");

static uint32_t g_meta_sequence = 0;
static char g_active_slot = '\0';
static uint32_t g_active_base = 0;

static int metadata_read_latest(char *slot, uint32_t *sequence)
{
    const CuteMetaRecord *records =
        (const CuteMetaRecord *)(uintptr_t)CUTE_META_ADDR;

    const uint32_t count =
        CUTE_FLASH_PAGE / sizeof(CuteMetaRecord);

    int found = 0;
    uint32_t best_seq = 0;
    char best_slot = '\0';

    for (uint32_t i = 0; i < count; ++i) {
        const CuteMetaRecord *r = &records[i];

        if (r->magic == 0xFFFFFFFFu) {
            break;
        }

        if (r->magic != META_MAGIC) {
            continue;
        }

        const uint32_t crc =
            crc32_memory(
                (const uint8_t *)r,
                12u);

        if (crc != r->crc32) {
            continue;
        }

        const char s = (char)(r->active_slot & 0xFFu);

        if (s != 'A' && s != 'B') {
            continue;
        }

        if (!found || r->sequence > best_seq) {
            found = 1;
            best_seq = r->sequence;
            best_slot = s;
        }
    }

    if (!found) return 0;

    *slot = best_slot;
    *sequence = best_seq;

    return 1;
}

static int metadata_append(char slot)
{
    CuteMetaRecord rec;

    rec.magic = META_MAGIC;
    rec.sequence = g_meta_sequence + 1u;
    rec.active_slot = (uint32_t)(uint8_t)slot;
    rec.crc32 =
        crc32_memory(
            (const uint8_t *)&rec,
            12u);

    CuteMetaRecord *records =
        (CuteMetaRecord *)(uintptr_t)CUTE_META_ADDR;

    const uint32_t count =
        CUTE_FLASH_PAGE / sizeof(CuteMetaRecord);

    uint32_t index = count;

    for (uint32_t i = 0; i < count; ++i) {
        if (records[i].magic == 0xFFFFFFFFu) {
            index = i;
            break;
        }
    }

    if (index == count) {
        if (nvmc_erase_page(CUTE_META_ADDR) != 0) {
            return -1;
        }

        index = 0;
    }

    const uint32_t addr =
        CUTE_META_ADDR +
        index * sizeof(CuteMetaRecord);

    const uint32_t *words =
        (const uint32_t *)&rec;

    for (uint32_t i = 0; i < 4u; ++i) {
        if (nvmc_write_word(
                addr + 4u * i,
                words[i]) != 0) {
            return -2;
        }
    }

    g_meta_sequence = rec.sequence;

    return 0;
}

static uint32_t base_for_slot(char slot)
{
    return slot == 'B'
        ? CUTE_SLOT_B_ADDR
        : CUTE_SLOT_A_ADDR;
}

static void activate_slot(char slot)
{
    g_active_slot = slot;
    g_active_base = base_for_slot(slot);
}

int cute_model_flash_init(void)
{
    char preferred = '\0';
    uint32_t seq = 0;

    const int has_meta =
        metadata_read_latest(
            &preferred,
            &seq);

    g_meta_sequence =
        has_meta ? seq : 0u;

    if (has_meta &&
        validate_model_at(
            base_for_slot(preferred))) {
        activate_slot(preferred);
        return 1;
    }

    if (validate_model_at(CUTE_SLOT_A_ADDR)) {
        activate_slot('A');
        return 1;
    }

    if (validate_model_at(CUTE_SLOT_B_ADDR)) {
        activate_slot('B');
        return 1;
    }

    g_active_slot = '\0';
    g_active_base = 0u;

    return 0;
}

int cute_model_ready(void)
{
    return g_active_base != 0u;
}

char cute_model_active_slot(void)
{
    return g_active_slot;
}

const CuteModelHeaderBase *cute_model_header(void)
{
    if (!cute_model_ready()) return 0;

    return
        (const CuteModelHeaderBase *)
        (uintptr_t)g_active_base;
}

const CuteLayerRecord *cute_model_layer(uint16_t index)
{
    const CuteModelHeaderBase *h =
        cute_model_header();

    if (!h || index >= h->layer_count) {
        return 0;
    }

    const uint32_t offset =
        CUTE_MODEL_BASE_BYTES +
        (uint32_t)index * CUTE_MODEL_RECORD_BYTES;

    if (offset + sizeof(CuteLayerRecord) > h->header_bytes) {
        return 0;
    }

    return
        (const CuteLayerRecord *)
        (uintptr_t)(g_active_base + offset);
}

const int8_t *cute_model_weight(uint16_t index)
{
    const CuteLayerRecord *r = cute_model_layer(index);
    if (!r) return 0;

    return
        (const int8_t *)
        (uintptr_t)(g_active_base + r->weight_offset);
}

const int32_t *cute_model_bias(uint16_t index)
{
    const CuteLayerRecord *r = cute_model_layer(index);
    if (!r) return 0;

    return
        (const int32_t *)
        (uintptr_t)(g_active_base + r->bias_offset);
}

const int32_t *cute_model_multiplier(uint16_t index)
{
    const CuteLayerRecord *r = cute_model_layer(index);
    if (!r) return 0;

    return
        (const int32_t *)
        (uintptr_t)(g_active_base + r->multiplier_offset);
}

const int32_t *cute_model_shift(uint16_t index)
{
    const CuteLayerRecord *r = cute_model_layer(index);
    if (!r) return 0;

    return
        (const int32_t *)
        (uintptr_t)(g_active_base + r->shift_offset);
}

uint16_t cute_model_format_version(void)
{
    const CuteModelHeaderBase *h = cute_model_header();
    return h ? h->version : 0u;
}

uint16_t cute_model_header_bytes(void)
{
    const CuteModelHeaderBase *h = cute_model_header();
    return h ? h->header_bytes : 0u;
}

uint16_t cute_model_layer_count(void)
{
    const CuteModelHeaderBase *h = cute_model_header();
    return h ? h->layer_count : 0u;
}

uint32_t cute_model_total_bytes(void)
{
    const CuteModelHeaderBase *h = cute_model_header();
    return h ? h->total_bytes : 0u;
}

uint8_t cute_model_hybrid_blocks(void)
{
    const CuteModelHeaderBase *h = cute_model_header();
    if (!h) return 0u;

    if (h->version == CUTE_MODEL_FORMAT_VERSION_V1) {
        return CUTE_MODEL_V1_HYBRID_BLOCKS;
    }

    if (h->version == CUTE_MODEL_FORMAT_VERSION_V2) {
        return h->hybrid_blocks;
    }

    return 0u;
}

int cute_model_has_mlp_head(void)
{
    const CuteModelHeaderBase *h = cute_model_header();
    if (!h || h->version != CUTE_MODEL_FORMAT_VERSION_V2) {
        return 0;
    }

    return
        (h->flags & CUTE_MODEL_FLAG_MLP32_SPLIT_HEAD) != 0u;
}

uint8_t cute_model_head_hidden_layers(void)
{
    const CuteModelHeaderBase *h = cute_model_header();

    if (!h || !cute_model_has_mlp_head()) {
        return 0u;
    }

    const uint8_t m =
        h->head_hidden_layers == 0u
            ? 1u
            : h->head_hidden_layers;

    if (m < CUTE_MODEL_MIN_HEAD_HIDDEN_LAYERS ||
        m > CUTE_MODEL_MAX_HEAD_HIDDEN_LAYERS) {
        return 0u;
    }

    return m;
}

uint16_t cute_model_head_layer_index(void)
{
    const uint8_t n = cute_model_hybrid_blocks();
    if (!n) return 0xFFFFu;

    return (uint16_t)(6u + 4u * (uint16_t)n);
}

uint16_t cute_model_head_hidden_layer_index(uint8_t hidden_index)
{
    const uint16_t base = cute_model_head_layer_index();
    const uint8_t m = cute_model_head_hidden_layers();

    if (base == 0xFFFFu || !m || hidden_index >= m) {
        return 0xFFFFu;
    }

    return (uint16_t)(base + hidden_index);
}

uint16_t cute_model_head_objectness_layer_index(void)
{
    const uint16_t base = cute_model_head_layer_index();
    const uint8_t m = cute_model_head_hidden_layers();

    if (base == 0xFFFFu || !m) return 0xFFFFu;

    return (uint16_t)(base + m);
}

uint16_t cute_model_head_box_layer_index(void)
{
    const uint16_t obj =
        cute_model_head_objectness_layer_index();

    if (obj == 0xFFFFu) return 0xFFFFu;

    return (uint16_t)(obj + 1u);
}

const char *cute_model_label(void)
{
    const CuteModelHeaderBase *h = cute_model_header();
    return h ? h->label : "none";
}

float cute_model_confidence_threshold(void)
{
    const CuteModelHeaderBase *h = cute_model_header();
    return h ? h->confidence_threshold : 0.50f;
}

float cute_model_nms_iou_threshold(void)
{
    const CuteModelHeaderBase *h = cute_model_header();
    return h ? h->nms_iou_threshold : 0.35f;
}

float cute_model_min_box_w(void)
{
    const CuteModelHeaderBase *h = cute_model_header();
    return h ? h->min_box_w : 0.05f;
}

float cute_model_min_box_h(void)
{
    const CuteModelHeaderBase *h = cute_model_header();
    return h ? h->min_box_h : 0.05f;
}

uint8_t cute_model_max_detections(void)
{
    const CuteModelHeaderBase *h = cute_model_header();
    return h ? h->max_detections : 16u;
}

/* ------------------------------------------------------------
 * Streaming uploader.
 * ------------------------------------------------------------ */
static char g_upload_slot = '\0';
static uint32_t g_upload_base = 0u;
static uint32_t g_upload_expected = 0u;
static uint32_t g_upload_received = 0u;

static uint8_t g_partial[4];
static uint8_t g_partial_n = 0u;
static uint32_t g_write_address = 0u;

int cute_model_upload_begin(
    uint32_t total_bytes,
    char *target_slot)
{
    if (total_bytes < CUTE_MODEL_BASE_BYTES ||
        total_bytes > CUTE_SLOT_BYTES) {
        return -1;
    }

    char target;

    if (g_active_slot == 'A') {
        target = 'B';
    } else if (g_active_slot == 'B') {
        target = 'A';
    } else {
        target = 'A';
    }

    const uint32_t base = base_for_slot(target);

    const uint32_t erase_bytes =
        (total_bytes + CUTE_FLASH_PAGE - 1u) &
        ~(CUTE_FLASH_PAGE - 1u);

    for (uint32_t off = 0u;
         off < erase_bytes;
         off += CUTE_FLASH_PAGE) {

        if (nvmc_erase_page(base + off) != 0) {
            return -2;
        }
    }

    g_upload_slot = target;
    g_upload_base = base;
    g_upload_expected = total_bytes;
    g_upload_received = 0u;
    g_partial_n = 0u;
    g_write_address = base;

    if (target_slot) {
        *target_slot = target;
    }

    return 0;
}

static int flush_word(const uint8_t bytes[4])
{
    const uint32_t word =
        ((uint32_t)bytes[0]) |
        ((uint32_t)bytes[1] << 8) |
        ((uint32_t)bytes[2] << 16) |
        ((uint32_t)bytes[3] << 24);

    if (nvmc_write_word(
            g_write_address,
            word) != 0) {
        return -1;
    }

    g_write_address += 4u;
    return 0;
}

int cute_model_upload_write(
    const uint8_t *data,
    uint32_t n)
{
    if (!data || n == 0u) return -1;

    if (g_upload_base == 0u ||
        g_upload_received + n >
            g_upload_expected) {
        return -2;
    }

    for (uint32_t i = 0u; i < n; ++i) {
        g_partial[g_partial_n++] = data[i];
        ++g_upload_received;

        if (g_partial_n == 4u) {
            if (flush_word(g_partial) != 0) {
                return -3;
            }

            g_partial_n = 0u;
        }
    }

    return 0;
}

int cute_model_upload_end(char *activated_slot)
{
    if (g_upload_base == 0u) return -1;

    if (g_upload_received != g_upload_expected) {
        return -2;
    }

    if (g_partial_n != 0u) {
        while (g_partial_n < 4u) {
            g_partial[g_partial_n++] = 0xFFu;
        }

        if (flush_word(g_partial) != 0) {
            return -3;
        }

        g_partial_n = 0u;
    }

    if (!validate_model_at(g_upload_base)) {
        return -4;
    }

    if (metadata_append(g_upload_slot) != 0) {
        return -5;
    }

    activate_slot(g_upload_slot);

    if (activated_slot) {
        *activated_slot = g_upload_slot;
    }

    g_upload_slot = '\0';
    g_upload_base = 0u;
    g_upload_expected = 0u;
    g_upload_received = 0u;

    return 0;
}

void cute_model_upload_abort(void)
{
    g_upload_slot = '\0';
    g_upload_base = 0u;
    g_upload_expected = 0u;
    g_upload_received = 0u;
    g_partial_n = 0u;
    g_write_address = 0u;
}

uint32_t cute_model_upload_received(void)
{
    return g_upload_received;
}

uint32_t cute_model_upload_expected(void)
{
    return g_upload_expected;
}
