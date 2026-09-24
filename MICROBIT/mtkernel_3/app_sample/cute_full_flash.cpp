#include <stdint.h>
#include <string.h>

#include "noodle.h"
#include "noodle_int8.h"

#include "cute_model_flash.h"

#define INPUT_W      128u
#define INPUT_C      1u

#define STEM1_W      64u
#define STEM1_C      8u
#define STEM2_W      32u
#define STEM2_C      16u
#define STEM3_W      16u
#define STEM3_C      32u

#define HYB_W        16u
#define HYB_C        32u
#define HYB_DW_C     32u
#define HYB_PW_C     16u

#define HEAD_C       5u
#define HEAD_W       16u
#define HEAD_SIZE    (HEAD_C * HEAD_W * HEAD_W)

static NoodleTensor g_feat_a;
static NoodleTensor g_feat_b;

static NoodleTensor g_split_a;
static NoodleTensor g_split_b;

/* Reused by every sequential dual-DW/PW hybrid. */
static NoodleTensor g_branch_a;  /* retained PWA output, 16x16x16 */
static NoodleTensor g_dw;        /* temporary DW output, 32x16x16 */
static NoodleTensor g_pw;        /* PWB output, 16x16x16 */

static NoodleTensor g_head;

static int g_initialized = 0;

static void reset_all(void)
{
    if (g_initialized) {
        noodle_tensor_free(&g_feat_a);
        noodle_tensor_free(&g_feat_b);
        noodle_tensor_free(&g_split_a);
        noodle_tensor_free(&g_split_b);
        noodle_tensor_free(&g_branch_a);
        noodle_tensor_free(&g_dw);
        noodle_tensor_free(&g_pw);
        noodle_tensor_free(&g_head);
    }

    noodle_tensor_init(&g_feat_a);
    noodle_tensor_init(&g_feat_b);
    noodle_tensor_init(&g_split_a);
    noodle_tensor_init(&g_split_b);
    noodle_tensor_init(&g_branch_a);
    noodle_tensor_init(&g_dw);
    noodle_tensor_init(&g_pw);
    noodle_tensor_init(&g_head);

    g_initialized = 1;
}

static ConvMem make_conv(
    const CuteLayerRecord *r,
    uint16_t id,
    uint16_t k,
    uint16_t p,
    uint16_t stride,
    int relu)
{
    ConvMem conv = {};

    conv.K = k;
    conv.P = p;
    conv.S = stride;
    conv.OP = 0;
    conv.O = (uint16_t)r->bias_count;

    conv.weight = cute_model_weight(id);
    conv.bias = cute_model_bias(id);
    conv.multiplier = cute_model_multiplier(id);
    conv.shift = cute_model_shift(id);

    conv.input_scale = r->input_scale;
    conv.input_zero_point = r->input_zero_point;
    conv.output_scale = r->output_scale;
    conv.output_zero_point = r->output_zero_point;

    conv.activation_min =
        relu ? r->output_zero_point : -128;
    conv.activation_max = 127;

    conv.depth_multiplier = 1;
    conv.act = relu ? ACT_RELU : ACT_NONE;

    return conv;
}

static void make_external_input(
    NoodleTensor *t,
    int8_t *data,
    float scale,
    int32_t zp)
{
    memset(t, 0, sizeof(*t));

    t->buffer.data = data;
    t->buffer.capacity = INPUT_W * INPUT_W;

    t->C = INPUT_C;
    t->W = INPUT_W;
    t->rank = NOODLE_TENSOR_2D;
    t->scale = scale;
    t->zero_point = zp;
}

/*
 * Stem 1 differs from the later stems because the 16-KiB serial image
 * buffer is external to Noodle's arena.
 *
 * Noodle's packed arena may relocate buffers after tensor sizing calls, so
 * raw buffer pointers are always re-read immediately before each memcpy.
 */
static int run_split_stem1(
    NoodleTensor *input,
    NoodleTensor *output)
{
    const CuteLayerRecord *ra =
        cute_model_layer(CUTE_LAYER_S1A);
    const CuteLayerRecord *rb =
        cute_model_layer(CUTE_LAYER_S1B);

    if (!ra || !rb) return -1;

    if (!noodle_i8_tensor_require_2d(
            output,
            STEM1_C,
            STEM1_W,
            ra->output_scale,
            ra->output_zero_point)) {
        return -2;
    }

    const ConvMem a = make_conv(
        ra, CUTE_LAYER_S1A,
        3, NOODLE_SAME_PADDING, 2, 1);

    if (noodle_i8_conv2d(
            input, a, &g_split_a) != STEM1_W) {
        return -3;
    }

    const uint32_t half_bytes =
        4u * STEM1_W * STEM1_W;

    if (!output->buffer.data ||
        !g_split_a.buffer.data) {
        return -4;
    }

    memcpy(
        output->buffer.data,
        g_split_a.buffer.data,
        half_bytes);

    noodle_tensor_free(&g_split_a);

    const ConvMem b = make_conv(
        rb, CUTE_LAYER_S1B,
        3, NOODLE_SAME_PADDING, 2, 1);

    if (noodle_i8_conv2d(
            input, b, &g_split_b) != STEM1_W) {
        return -5;
    }

    if (!output->buffer.data ||
        !g_split_b.buffer.data) {
        return -6;
    }

    memcpy(
        output->buffer.data + half_bytes,
        g_split_b.buffer.data,
        half_bytes);

    noodle_tensor_free(&g_split_b);

    return 0;
}

static int run_split_stem(
    NoodleTensor *input,
    NoodleTensor *output,
    uint16_t ida,
    uint16_t idb,
    uint16_t out_c,
    uint16_t out_w)
{
    const CuteLayerRecord *ra = cute_model_layer(ida);
    const CuteLayerRecord *rb = cute_model_layer(idb);

    if (!ra || !rb) return -1;

    const ConvMem a = make_conv(
        ra, ida,
        3, NOODLE_SAME_PADDING, 2, 1);

    const ConvMem b = make_conv(
        rb, idb,
        3, NOODLE_SAME_PADDING, 2, 1);

    if (noodle_i8_conv2d(
            input, a, &g_split_a) != out_w) {
        return -2;
    }

    if (noodle_i8_conv2d(
            input, b, &g_split_b) != out_w) {
        return -3;
    }

    noodle_tensor_free(input);

    int8_t *dst =
        noodle_i8_tensor_require_2d(
            output,
            out_c,
            out_w,
            ra->output_scale,
            ra->output_zero_point);

    if (!dst) return -4;

    const uint32_t plane =
        (uint32_t)out_w * out_w;

    const uint32_t a_bytes =
        ra->bias_count * plane;

    const uint32_t b_bytes =
        rb->bias_count * plane;

    memcpy(
        output->buffer.data,
        g_split_a.buffer.data,
        a_bytes);

    memcpy(
        output->buffer.data + a_bytes,
        g_split_b.buffer.data,
        b_bytes);

    noodle_tensor_free(&g_split_a);
    noodle_tensor_free(&g_split_b);

    return 0;
}

static uint16_t hybrid_layer_index(
    uint8_t block_index,
    uint8_t component_offset)
{
    return (uint16_t)(
        CUTE_LAYER_HYBRID_BASE +
        4u * (uint16_t)block_index +
        component_offset
    );
}

/*
 * micro:bit is single-core, so the two v2 hybrid branches are executed
 * sequentially while preserving the exact same graph and quantization as the
 * ESP32-S3 package:
 *
 *   A: DWA 32->32 -> PWA 32->16
 *   B: DWB 32->32 -> PWB 32->16
 *   concat 16+16 -> 32
 *
 * H changes only the number of repeated blocks; activation geometry stays
 * fixed at 32x16x16 and the same Noodle tensors are reused for every block.
 */
static int run_hybrid(
    NoodleTensor *input,
    NoodleTensor *output,
    uint8_t block_index)
{
    const uint16_t dwa_id = hybrid_layer_index(block_index, 0u);
    const uint16_t pwa_id = hybrid_layer_index(block_index, 1u);
    const uint16_t dwb_id = hybrid_layer_index(block_index, 2u);
    const uint16_t pwb_id = hybrid_layer_index(block_index, 3u);

    const CuteLayerRecord *rdwa = cute_model_layer(dwa_id);
    const CuteLayerRecord *rpwa = cute_model_layer(pwa_id);
    const CuteLayerRecord *rdwb = cute_model_layer(dwb_id);
    const CuteLayerRecord *rpwb = cute_model_layer(pwb_id);

    if (!rdwa || !rpwa || !rdwb || !rpwb) return -1;

    noodle_tensor_free(output);
    noodle_tensor_free(&g_branch_a);
    noodle_tensor_free(&g_dw);
    noodle_tensor_free(&g_pw);

    /* Branch A. */
    ConvMem dwa = make_conv(
        rdwa, dwa_id,
        3, 1, 1, 1);
    dwa.depth_multiplier = 1;

    if (noodle_i8_dwconv2d(
            input,
            dwa,
            &g_dw) != HYB_W) {
        return -2;
    }

    const ConvMem pwa = make_conv(
        rpwa, pwa_id,
        1, 0, 1, 1);

    if (noodle_i8_conv2d(
            &g_dw,
            pwa,
            &g_branch_a) != HYB_W) {
        return -3;
    }

    noodle_tensor_free(&g_dw);

    /* Branch B. */
    ConvMem dwb = make_conv(
        rdwb, dwb_id,
        3, 1, 1, 1);
    dwb.depth_multiplier = 1;

    if (noodle_i8_dwconv2d(
            input,
            dwb,
            &g_dw) != HYB_W) {
        return -4;
    }

    /* No later operation needs the block input once DWB has completed. */
    noodle_tensor_free(input);

    const ConvMem pwb = make_conv(
        rpwb, pwb_id,
        1, 0, 1, 1);

    if (noodle_i8_conv2d(
            &g_dw,
            pwb,
            &g_pw) != HYB_W) {
        return -5;
    }

    noodle_tensor_free(&g_dw);

    int8_t *dst =
        noodle_i8_tensor_require_2d(
            output,
            HYB_C,
            HYB_W,
            rpwa->output_scale,
            rpwa->output_zero_point);

    if (!dst) return -6;

    const uint32_t plane = HYB_W * HYB_W;
    const uint32_t branch_bytes = HYB_PW_C * plane;

    if (!output->buffer.data ||
        !g_branch_a.buffer.data ||
        !g_pw.buffer.data) {
        return -7;
    }

    memcpy(
        output->buffer.data,
        g_branch_a.buffer.data,
        branch_bytes);

    memcpy(
        output->buffer.data + branch_bytes,
        g_pw.buffer.data,
        branch_bytes);

    noodle_tensor_free(&g_branch_a);
    noodle_tensor_free(&g_pw);

    return 0;
}

static int run_hidden_head_layer(
    NoodleTensor *input,
    NoodleTensor *output,
    uint16_t id)
{
    const CuteLayerRecord *r = cute_model_layer(id);
    if (!r) return -1;

    const ConvMem hidden = make_conv(
        r, id,
        1, 0, 1, 1);

    if (noodle_i8_conv2d(
            input,
            hidden,
            output) != HEAD_W) {
        return -2;
    }

    noodle_tensor_free(input);
    return 0;
}

static int run_split_head_output(
    NoodleTensor *input,
    uint16_t obj_id,
    uint16_t box_id)
{
    const CuteLayerRecord *ro = cute_model_layer(obj_id);
    const CuteLayerRecord *rb = cute_model_layer(box_id);

    if (!ro || !rb) return -1;

    noodle_tensor_free(&g_split_a);
    noodle_tensor_free(&g_split_b);
    noodle_tensor_free(&g_head);

    const ConvMem obj = make_conv(
        ro, obj_id,
        1, 0, 1, 0);

    const ConvMem box = make_conv(
        rb, box_id,
        1, 0, 1, 0);

    if (noodle_i8_conv2d(
            input,
            obj,
            &g_split_a) != HEAD_W) {
        return -2;
    }

    if (noodle_i8_conv2d(
            input,
            box,
            &g_split_b) != HEAD_W) {
        return -3;
    }

    noodle_tensor_free(input);

    int8_t *dst =
        noodle_i8_tensor_require_2d(
            &g_head,
            HEAD_C,
            HEAD_W,
            ro->output_scale,
            ro->output_zero_point);

    if (!dst) return -4;

    const uint32_t plane = HEAD_W * HEAD_W;

    if (!g_head.buffer.data ||
        !g_split_a.buffer.data ||
        !g_split_b.buffer.data) {
        return -5;
    }

    memcpy(
        g_head.buffer.data,
        g_split_a.buffer.data,
        plane);

    memcpy(
        g_head.buffer.data + plane,
        g_split_b.buffer.data,
        4u * plane);

    noodle_tensor_free(&g_split_a);
    noodle_tensor_free(&g_split_b);

    return 0;
}

extern "C" int cute_full_flash_run(
    uint8_t *gray_u8,
    uint32_t n,
    const int8_t **output_data,
    uint32_t *output_size)
{
    if (!cute_model_ready()) return -100;

    if (!gray_u8 ||
        !output_data ||
        !output_size) {
        return -1;
    }

    if (n != INPUT_W * INPUT_W) {
        return -2;
    }

    const CuteModelHeaderBase *header =
        cute_model_header();

    if (!header) return -101;

    if (header->version != CUTE_MODEL_FORMAT_VERSION_V2) {
        return -102;
    }

    const uint8_t hybrid_blocks =
        cute_model_hybrid_blocks();

    if (hybrid_blocks < CUTE_MODEL_MIN_HYBRID_BLOCKS ||
        hybrid_blocks > CUTE_MODEL_MAX_HYBRID_BLOCKS) {
        return -103;
    }

    reset_all();

    if (header->input_zero_point != -128) {
        return -104;
    }

    for (uint32_t i = 0; i < n; ++i) {
        gray_u8[i] =
            (uint8_t)(
                (int32_t)gray_u8[i] - 128
            );
    }

    NoodleTensor external_input;

    make_external_input(
        &external_input,
        (int8_t *)gray_u8,
        header->input_scale,
        header->input_zero_point);

    int rc =
        run_split_stem1(
            &external_input,
            &g_feat_a);

    if (rc) return -110 + rc;

    rc =
        run_split_stem(
            &g_feat_a,
            &g_feat_b,
            CUTE_LAYER_S2A,
            CUTE_LAYER_S2B,
            STEM2_C,
            STEM2_W);

    if (rc) return -120 + rc;

    rc =
        run_split_stem(
            &g_feat_b,
            &g_feat_a,
            CUTE_LAYER_S3A,
            CUTE_LAYER_S3B,
            STEM3_C,
            STEM3_W);

    if (rc) return -130 + rc;

    NoodleTensor *in = &g_feat_a;
    NoodleTensor *out = &g_feat_b;

    for (uint8_t block = 0u;
         block < hybrid_blocks;
         ++block) {

        rc = run_hybrid(
            in,
            out,
            block);

        if (rc) {
            return -200 - (int)block * 10 + rc;
        }

        NoodleTensor *tmp = in;
        in = out;
        out = tmp;
    }

    if (cute_model_has_mlp_head()) {
        const uint8_t m =
            cute_model_head_hidden_layers();

        if (m < CUTE_MODEL_MIN_HEAD_HIDDEN_LAYERS ||
            m > CUTE_MODEL_MAX_HEAD_HIDDEN_LAYERS) {
            return -300;
        }

        for (uint8_t hidden = 0u;
             hidden < m;
             ++hidden) {

            const uint16_t hidden_id =
                cute_model_head_hidden_layer_index(hidden);

            if (hidden_id == 0xFFFFu) return -301;

            rc = run_hidden_head_layer(
                in,
                out,
                hidden_id);

            if (rc) {
                return -310 - (int)hidden * 10 + rc;
            }

            NoodleTensor *tmp = in;
            in = out;
            out = tmp;
        }

        const uint16_t obj_id =
            cute_model_head_objectness_layer_index();
        const uint16_t box_id =
            cute_model_head_box_layer_index();

        if (obj_id == 0xFFFFu ||
            box_id == 0xFFFFu) {
            return -350;
        }

        rc = run_split_head_output(
            in,
            obj_id,
            box_id);

        if (rc) return -360 + rc;
    }
    else {
        const uint16_t head_id =
            cute_model_head_layer_index();

        const CuteLayerRecord *rh =
            cute_model_layer(head_id);

        if (!rh) return -370;

        const ConvMem head = make_conv(
            rh,
            head_id,
            1, 0, 1, 0);

        if (noodle_i8_conv2d(
                in,
                head,
                &g_head) != HEAD_W ||
            !g_head.buffer.data) {
            return -371;
        }

        noodle_tensor_free(in);
    }

    if (!g_head.buffer.data ||
        g_head.C != HEAD_C ||
        g_head.W != HEAD_W) {
        return -380;
    }

    *output_data = g_head.buffer.data;
    *output_size = HEAD_SIZE;

    return 0;
}
