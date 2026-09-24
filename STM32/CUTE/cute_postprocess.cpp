#include "cute_postprocess.h"
#include "cute_bridge.h"
#include "cute_model_manager.h"

#include <math.h>
#include <stddef.h>

namespace {

static constexpr uint16_t GRID_W = 16U;
static constexpr uint32_t GRID_PIXELS = (uint32_t)GRID_W * GRID_W;
static constexpr uint32_t HEAD_BYTES = 5U * GRID_PIXELS;
static constexpr uint8_t MAX_CANDIDATES = 128U;
static constexpr float NMS_CONTAINMENT_THRESHOLD = 0.75f;

static CuteDetection candidates[MAX_CANDIDATES];

static inline float clamp01(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static inline float sigmoidf_local(float x)
{
    return 1.0f / (1.0f + expf(-x));
}

static inline float dequantize(int8_t q, float scale, int32_t zero_point)
{
    return ((float)((int32_t)q - zero_point)) * scale;
}

static float iou(const CuteDetection &a, const CuteDetection &b)
{
    const float x1 = fmaxf(a.x1, b.x1);
    const float y1 = fmaxf(a.y1, b.y1);
    const float x2 = fminf(a.x2, b.x2);
    const float y2 = fminf(a.y2, b.y2);

    const float inter =
        fmaxf(0.0f, x2 - x1) *
        fmaxf(0.0f, y2 - y1);

    const float area_a =
        fmaxf(0.0f, a.x2 - a.x1) *
        fmaxf(0.0f, a.y2 - a.y1);

    const float area_b =
        fmaxf(0.0f, b.x2 - b.x1) *
        fmaxf(0.0f, b.y2 - b.y1);

    return inter / (area_a + area_b - inter + 1e-9f);
}

static float intersection_over_smaller(const CuteDetection &a,
                                       const CuteDetection &b)
{
    const float x1 = fmaxf(a.x1, b.x1);
    const float y1 = fmaxf(a.y1, b.y1);
    const float x2 = fminf(a.x2, b.x2);
    const float y2 = fminf(a.y2, b.y2);

    const float inter =
        fmaxf(0.0f, x2 - x1) *
        fmaxf(0.0f, y2 - y1);

    const float area_a =
        fmaxf(0.0f, a.x2 - a.x1) *
        fmaxf(0.0f, a.y2 - a.y1);

    const float area_b =
        fmaxf(0.0f, b.x2 - b.x1) *
        fmaxf(0.0f, b.y2 - b.y1);

    const float smaller = fminf(area_a, area_b);
    if (smaller <= 1e-9f) return 0.0f;
    return inter / smaller;
}

static void add_candidate(const CuteDetection &d, uint8_t &count)
{
    if (count < MAX_CANDIDATES)
    {
        candidates[count++] = d;
        return;
    }

    uint8_t weakest = 0;
    for (uint8_t i = 1; i < count; ++i)
    {
        if (candidates[i].confidence < candidates[weakest].confidence)
            weakest = i;
    }

    if (d.confidence > candidates[weakest].confidence)
        candidates[weakest] = d;
}

static void sort_candidates(uint8_t count)
{
    for (uint8_t i = 1; i < count; ++i)
    {
        const CuteDetection key = candidates[i];
        int j = (int)i - 1;

        while (j >= 0 && candidates[j].confidence < key.confidence)
        {
            candidates[j + 1] = candidates[j];
            --j;
        }
        candidates[j + 1] = key;
    }
}

} // namespace

extern "C" int cute_postprocess_decode(CuteDetectionSet *result)
{
    if (!result) return 0;
    result->count = 0;

    const int8_t *out = cute_runtime_head_data();
    if (!out || cute_runtime_head_bytes() != HEAD_BYTES || !cute_model_ready())
        return 0;

    /*
     * Use the package's final-head quantization directly.  The objectness and
     * box projections in the CUTE v2 split head share this final scale/zp.
     * This also keeps the decoder independent of transient tensor metadata.
     */
    const CuteModelHeaderBase *header = cute_model_header();
    if (!header || !(header->output_scale > 0.0f))
        return 0;

    const float out_scale = header->output_scale;
    const int32_t out_zp = header->output_zero_point;
    const float conf_threshold = cute_model_confidence_threshold();
    const float min_w = cute_model_min_box_w();
    const float min_h = cute_model_min_box_h();

    uint8_t candidate_count = 0;

    for (uint16_t gy = 0; gy < GRID_W; ++gy)
    {
        for (uint16_t gx = 0; gx < GRID_W; ++gx)
        {
            const uint32_t i = (uint32_t)gy * GRID_W + gx;

            const float confidence = sigmoidf_local(
                dequantize(out[0U * GRID_PIXELS + i], out_scale, out_zp));

            if (confidence < conf_threshold)
                continue;

            const float dx = sigmoidf_local(
                dequantize(out[1U * GRID_PIXELS + i], out_scale, out_zp));
            const float dy = sigmoidf_local(
                dequantize(out[2U * GRID_PIXELS + i], out_scale, out_zp));
            const float w = sigmoidf_local(
                dequantize(out[3U * GRID_PIXELS + i], out_scale, out_zp));
            const float h = sigmoidf_local(
                dequantize(out[4U * GRID_PIXELS + i], out_scale, out_zp));

            if (w < min_w || h < min_h)
                continue;

            const float cx = ((float)gx + dx) / (float)GRID_W;
            const float cy = ((float)gy + dy) / (float)GRID_W;

            CuteDetection d;
            d.confidence = confidence;
            d.x1 = clamp01(cx - 0.5f * w);
            d.y1 = clamp01(cy - 0.5f * h);
            d.x2 = clamp01(cx + 0.5f * w);
            d.y2 = clamp01(cy + 0.5f * h);

            add_candidate(d, candidate_count);
        }
    }

    sort_candidates(candidate_count);

    uint8_t max_det = cute_model_max_detections();
    if (max_det > CUTE_POST_MAX_DETECTIONS)
        max_det = CUTE_POST_MAX_DETECTIONS;

    const float nms_iou_threshold = cute_model_nms_iou_threshold();

    for (uint8_t i = 0;
         i < candidate_count && result->count < max_det;
         ++i)
    {
        int suppress = 0;

        for (uint8_t j = 0; j < result->count; ++j)
        {
            const float pair_iou = iou(candidates[i], result->items[j]);
            const float pair_ios =
                intersection_over_smaller(candidates[i], result->items[j]);

            /* Match the ESP32 firmware: standard IoU NMS + nested-box IoS. */
            if (pair_iou >= nms_iou_threshold ||
                pair_ios >= NMS_CONTAINMENT_THRESHOLD)
            {
                suppress = 1;
                break;
            }
        }

        if (!suppress)
            result->items[result->count++] = candidates[i];
    }

    return 1;
}
