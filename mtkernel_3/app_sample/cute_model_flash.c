#include "cute_model_flash.h"

#include <stdint.h>
#include <string.h>

/* ------------------------------------------------------------
 * Fixed architecture contract.
 * Same learned model as ESP32; only execution scheduling differs.
 * ------------------------------------------------------------ */
static const uint32_t EXPECTED_WEIGHT_COUNTS[CUTE_MODEL_LAYER_COUNT] = {
    36, 36, 576, 576, 2304, 2304,
    2304, 288, 768,
    2304, 288, 768,
    2304, 288, 768,
    2304, 288, 768,
    2304, 288, 768,
    160
};

static const uint32_t EXPECTED_BIAS_COUNTS[CUTE_MODEL_LAYER_COUNT] = {
    4, 4, 8, 8, 16, 16,
    8, 32, 24,
    8, 32, 24,
    8, 32, 24,
    8, 32, 24,
    8, 32, 24,
    5
};

/* ------------------------------------------------------------
 * nRF52833 NVMC backend.
 *
 * IMPORTANT: this backend is for the pre-SoftDevice validation stage.
 * Once S113 is enabled, the storage API remains the same but erase/write
 * must be implemented with sd_flash_page_erase()/sd_flash_write().
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

static int range_ok(
    uint32_t offset,
    uint32_t bytes,
    uint32_t total)
{
    if (offset < CUTE_MODEL_HEADER_BYTES) return 0;
    if (offset > total) return 0;
    if (bytes > total - offset) return 0;
    return 1;
}

static CuteModelHeader g_header_scratch;

static int validate_model_at(uint32_t base)
{
    if (base != CUTE_SLOT_A_ADDR &&
        base != CUTE_SLOT_B_ADDR) {
        return 0;
    }

    memcpy(
        &g_header_scratch,
        (const void *)(uintptr_t)base,
        sizeof(g_header_scratch)
    );

    CuteModelHeader *h = &g_header_scratch;

    if (memcmp(h->magic, "CUTE", 4) != 0) return 0;

    if (h->version != CUTE_MODEL_FORMAT_VERSION ||
        h->header_bytes != CUTE_MODEL_HEADER_BYTES ||
        h->architecture_id != CUTE_MODEL_ARCH_ID ||
        h->layer_count != CUTE_MODEL_LAYER_COUNT) {
        return 0;
    }

    if (h->total_bytes < CUTE_MODEL_HEADER_BYTES ||
        h->total_bytes > CUTE_SLOT_BYTES) {
        return 0;
    }

    const uint32_t wanted_header_crc = h->header_crc32;
    h->header_crc32 = 0;

    const uint32_t got_header_crc =
        crc32_memory(
            (const uint8_t *)h,
            sizeof(*h)
        );

    if (got_header_crc != wanted_header_crc) {
        return 0;
    }

    h->header_crc32 = wanted_header_crc;

    if (h->label[23] != '\0') return 0;

    if (!(h->confidence_threshold >= 0.0f &&
          h->confidence_threshold <= 1.0f)) return 0;

    if (!(h->nms_iou_threshold >= 0.0f &&
          h->nms_iou_threshold <= 1.0f)) return 0;

    if (!(h->min_box_w >= 0.0f &&
          h->min_box_w <= 1.0f)) return 0;

    if (!(h->min_box_h >= 0.0f &&
          h->min_box_h <= 1.0f)) return 0;

    if (h->max_detections == 0 ||
        h->max_detections > 32) return 0;

    for (uint32_t i = 0;
         i < CUTE_MODEL_LAYER_COUNT;
         ++i) {

        const CuteLayerRecord *r = &h->layers[i];

        if (r->weight_count != EXPECTED_WEIGHT_COUNTS[i] ||
            r->bias_count != EXPECTED_BIAS_COUNTS[i]) {
            return 0;
        }

        if (!range_ok(
                r->weight_offset,
                r->weight_count,
                h->total_bytes) ||
            !range_ok(
                r->bias_offset,
                r->bias_count * 4u,
                h->total_bytes) ||
            !range_ok(
                r->multiplier_offset,
                r->bias_count * 4u,
                h->total_bytes) ||
            !range_ok(
                r->shift_offset,
                r->bias_count * 4u,
                h->total_bytes)) {
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

    const uint8_t *payload =
        (const uint8_t *)(uintptr_t)(
            base + CUTE_MODEL_HEADER_BYTES);

    const uint32_t payload_n =
        h->total_bytes - CUTE_MODEL_HEADER_BYTES;

    const uint32_t got_payload_crc =
        crc32_memory(payload, payload_n);

    if (got_payload_crc != h->payload_crc32) {
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
                12
            );

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
            12
        );

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

    for (uint32_t i = 0; i < 4; ++i) {
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
            &seq
        );

    g_meta_sequence =
        has_meta ? seq : 0;

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
    g_active_base = 0;

    return 0;
}

int cute_model_ready(void)
{
    return g_active_base != 0;
}

char cute_model_active_slot(void)
{
    return g_active_slot;
}

const CuteModelHeader *cute_model_header(void)
{
    if (!cute_model_ready()) return 0;

    return
        (const CuteModelHeader *)
        (uintptr_t)g_active_base;
}

const CuteLayerRecord *cute_model_layer(CuteLayerId id)
{
    const CuteModelHeader *h =
        cute_model_header();

    if (!h ||
        (uint32_t)id >= CUTE_MODEL_LAYER_COUNT) {
        return 0;
    }

    return &h->layers[(uint32_t)id];
}

const int8_t *cute_model_weight(CuteLayerId id)
{
    const CuteLayerRecord *r = cute_model_layer(id);

    if (!r) return 0;

    return
        (const int8_t *)
        (uintptr_t)(
            g_active_base + r->weight_offset
        );
}

const int32_t *cute_model_bias(CuteLayerId id)
{
    const CuteLayerRecord *r = cute_model_layer(id);

    if (!r) return 0;

    return
        (const int32_t *)
        (uintptr_t)(
            g_active_base + r->bias_offset
        );
}

const int32_t *cute_model_multiplier(CuteLayerId id)
{
    const CuteLayerRecord *r = cute_model_layer(id);

    if (!r) return 0;

    return
        (const int32_t *)
        (uintptr_t)(
            g_active_base + r->multiplier_offset
        );
}

const int32_t *cute_model_shift(CuteLayerId id)
{
    const CuteLayerRecord *r = cute_model_layer(id);

    if (!r) return 0;

    return
        (const int32_t *)
        (uintptr_t)(
            g_active_base + r->shift_offset
        );
}

const char *cute_model_label(void)
{
    const CuteModelHeader *h = cute_model_header();
    return h ? h->label : "none";
}

float cute_model_confidence_threshold(void)
{
    const CuteModelHeader *h = cute_model_header();
    return h ? h->confidence_threshold : 0.50f;
}

float cute_model_nms_iou_threshold(void)
{
    const CuteModelHeader *h = cute_model_header();
    return h ? h->nms_iou_threshold : 0.35f;
}

float cute_model_min_box_w(void)
{
    const CuteModelHeader *h = cute_model_header();
    return h ? h->min_box_w : 0.05f;
}

float cute_model_min_box_h(void)
{
    const CuteModelHeader *h = cute_model_header();
    return h ? h->min_box_h : 0.05f;
}

uint8_t cute_model_max_detections(void)
{
    const CuteModelHeader *h = cute_model_header();
    return h ? h->max_detections : 16u;
}

/* ------------------------------------------------------------
 * Streaming uploader.
 * ------------------------------------------------------------ */
static char g_upload_slot = '\0';
static uint32_t g_upload_base = 0;
static uint32_t g_upload_expected = 0;
static uint32_t g_upload_received = 0;

static uint8_t g_partial[4];
static uint8_t g_partial_n = 0;
static uint32_t g_write_address = 0;

int cute_model_upload_begin(
    uint32_t total_bytes,
    char *target_slot)
{
    if (total_bytes < CUTE_MODEL_HEADER_BYTES ||
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

    for (uint32_t off = 0;
         off < erase_bytes;
         off += CUTE_FLASH_PAGE) {

        if (nvmc_erase_page(base + off) != 0) {
            return -2;
        }
    }

    g_upload_slot = target;
    g_upload_base = base;
    g_upload_expected = total_bytes;
    g_upload_received = 0;
    g_partial_n = 0;
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
    if (!data || n == 0) return -1;

    if (g_upload_base == 0 ||
        g_upload_received + n >
            g_upload_expected) {
        return -2;
    }

    for (uint32_t i = 0; i < n; ++i) {
        g_partial[g_partial_n++] = data[i];
        ++g_upload_received;

        if (g_partial_n == 4u) {
            if (flush_word(g_partial) != 0) {
                return -3;
            }

            g_partial_n = 0;
        }
    }

    return 0;
}

int cute_model_upload_end(char *activated_slot)
{
    if (g_upload_base == 0) return -1;

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

        g_partial_n = 0;
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
    g_upload_base = 0;
    g_upload_expected = 0;
    g_upload_received = 0;

    return 0;
}

void cute_model_upload_abort(void)
{
    g_upload_slot = '\0';
    g_upload_base = 0;
    g_upload_expected = 0;
    g_upload_received = 0;
    g_partial_n = 0;
    g_write_address = 0;
}

uint32_t cute_model_upload_received(void)
{
    return g_upload_received;
}

uint32_t cute_model_upload_expected(void)
{
    return g_upload_expected;
}
