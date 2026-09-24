#include <stdint.h>
#include <math.h>

#include "cute_detect_flash.h"
#include "cute_model_flash.h"

#define GRID_W          16
#define GRID_PIXELS     (GRID_W * GRID_W)
#define MAX_CANDIDATES  64u

static CuteDetection g_candidates[MAX_CANDIDATES];

static float clamp01(float x)
{
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}

static float sigmoidf_cute(float x)
{
    return 1.0f / (1.0f + expf(-x));
}

static float box_iou(
    const CuteDetection *a,
    const CuteDetection *b)
{
    const float x1 = fmaxf(a->x1, b->x1);
    const float y1 = fmaxf(a->y1, b->y1);
    const float x2 = fminf(a->x2, b->x2);
    const float y2 = fminf(a->y2, b->y2);

    const float iw = fmaxf(0.0f, x2 - x1);
    const float ih = fmaxf(0.0f, y2 - y1);

    const float inter = iw * ih;

    const float aa =
        fmaxf(0.0f, a->x2 - a->x1) *
        fmaxf(0.0f, a->y2 - a->y1);

    const float ab =
        fmaxf(0.0f, b->x2 - b->x1) *
        fmaxf(0.0f, b->y2 - b->y1);

    return
        inter /
        (aa + ab - inter + 1.0e-9f);
}

static void add_candidate(
    const CuteDetection *d,
    uint8_t *count)
{
    if (*count < MAX_CANDIDATES) {
        g_candidates[*count] = *d;
        ++(*count);
        return;
    }

    uint8_t weakest = 0;

    for (uint8_t i = 1; i < *count; ++i) {
        if (g_candidates[i].confidence <
            g_candidates[weakest].confidence) {
            weakest = i;
        }
    }

    if (d->confidence >
        g_candidates[weakest].confidence) {
        g_candidates[weakest] = *d;
    }
}

static void sort_candidates(uint8_t count)
{
    for (uint8_t i = 1; i < count; ++i) {
        CuteDetection key = g_candidates[i];
        int j = (int)i - 1;

        while (j >= 0 &&
               g_candidates[j].confidence <
               key.confidence) {

            g_candidates[j + 1] =
                g_candidates[j];

            --j;
        }

        g_candidates[j + 1] = key;
    }
}

int cute_detect_flash_decode(
    const int8_t *head,
    uint32_t head_size,
    CuteDetectionSet *result)
{
    if (!head || !result) return -1;
    if (head_size != 5u * GRID_PIXELS) return -2;

    const CuteModelHeader *model =
        cute_model_header();

    if (!model) return -3;

    const float conf_threshold =
        model->confidence_threshold;

    const float nms_threshold =
        model->nms_iou_threshold;

    const float min_w =
        model->min_box_w;

    const float min_h =
        model->min_box_h;

    uint8_t max_det =
        model->max_detections;

    if (max_det >
        CUTE_DETECT_MAX_DETECTIONS) {
        max_det =
            CUTE_DETECT_MAX_DETECTIONS;
    }

    const float out_scale =
        model->output_scale;

    const int32_t out_zp =
        model->output_zero_point;

    uint8_t candidate_count = 0;

    for (int gy = 0; gy < GRID_W; ++gy) {
        for (int gx = 0; gx < GRID_W; ++gx) {
            const uint32_t i =
                (uint32_t)gy * GRID_W +
                (uint32_t)gx;

            const float obj_logit =
                ((float)(
                    (int32_t)
                    head[0 * GRID_PIXELS + i] -
                    out_zp
                )) * out_scale;

            const float confidence =
                sigmoidf_cute(obj_logit);

            if (confidence < conf_threshold) {
                continue;
            }

            float q[4];

            for (int c = 0; c < 4; ++c) {
                const float logit =
                    ((float)(
                        (int32_t)
                        head[(c + 1) *
                             GRID_PIXELS + i] -
                        out_zp
                    )) * out_scale;

                q[c] = sigmoidf_cute(logit);
            }

            const float dx = q[0];
            const float dy = q[1];
            const float w = q[2];
            const float h = q[3];

            if (w < min_w || h < min_h) {
                continue;
            }

            const float cx =
                ((float)gx + dx) /
                (float)GRID_W;

            const float cy =
                ((float)gy + dy) /
                (float)GRID_W;

            CuteDetection d;

            d.confidence = confidence;
            d.x1 = clamp01(cx - 0.5f * w);
            d.y1 = clamp01(cy - 0.5f * h);
            d.x2 = clamp01(cx + 0.5f * w);
            d.y2 = clamp01(cy + 0.5f * h);

            add_candidate(
                &d,
                &candidate_count);
        }
    }

    sort_candidates(candidate_count);

    result->count = 0;

    for (uint8_t i = 0;
         i < candidate_count &&
         result->count < max_det;
         ++i) {

        int suppress = 0;

        for (uint8_t j = 0;
             j < result->count;
             ++j) {

            if (box_iou(
                    &g_candidates[i],
                    &result->items[j]) >=
                nms_threshold) {

                suppress = 1;
                break;
            }
        }

        if (!suppress) {
            result->items[result->count++] =
                g_candidates[i];
        }
    }

    return 0;
}
