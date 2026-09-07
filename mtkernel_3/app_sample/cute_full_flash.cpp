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
#define HYB_A_C      8u
#define HYB_PW_C     24u

#define HEAD_C       5u
#define HEAD_W       16u
#define HEAD_SIZE    (HEAD_C * HEAD_W * HEAD_W)

static NoodleTensor g_feat_a;
static NoodleTensor g_feat_b;

static NoodleTensor g_split_a;
static NoodleTensor g_split_b;

static NoodleTensor g_branch_a;
static NoodleTensor g_dw;
static NoodleTensor g_pw;

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
    CuteLayerId id,
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
 * IMPORTANT:
 * Noodle's packed arena can relocate buffers after ANY tensor sizing call.
 * Therefore do not retain the raw pointer returned for `output` across
 * either split convolution. Re-read output->buffer.data immediately before
 * each memcpy.
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
    CuteLayerId ida,
    CuteLayerId idb,
    uint16_t out_c,
    uint16_t out_w)
{
    const CuteLayerRecord *ra =
        cute_model_layer(ida);
    const CuteLayerRecord *rb =
        cute_model_layer(idb);

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
        dst,
        g_split_a.buffer.data,
        a_bytes);

    memcpy(
        dst + a_bytes,
        g_split_b.buffer.data,
        b_bytes);

    noodle_tensor_free(&g_split_a);
    noodle_tensor_free(&g_split_b);

    return 0;
}

static int run_hybrid(
    NoodleTensor *input,
    NoodleTensor *output,
    CuteLayerId ida,
    CuteLayerId iddw,
    CuteLayerId idpw)
{
    const CuteLayerRecord *ra =
        cute_model_layer(ida);
    const CuteLayerRecord *rdw =
        cute_model_layer(iddw);
    const CuteLayerRecord *rpw =
        cute_model_layer(idpw);

    if (!ra || !rdw || !rpw) return -1;

    noodle_tensor_free(output);
    noodle_tensor_free(&g_branch_a);
    noodle_tensor_free(&g_dw);
    noodle_tensor_free(&g_pw);

    const ConvMem a = make_conv(
        ra, ida,
        3, 1, 1, 1);

    if (noodle_i8_conv2d(
            input,
            a,
            &g_branch_a) != HYB_W) {
        return -2;
    }

    ConvMem dw = make_conv(
        rdw, iddw,
        3, 1, 1, 1);

    dw.depth_multiplier = 1;

    if (noodle_i8_dwconv2d(
            input,
            dw,
            &g_dw) != HYB_W) {
        return -3;
    }

    noodle_tensor_free(input);

    const ConvMem pw = make_conv(
        rpw, idpw,
        1, 0, 1, 1);

    if (noodle_i8_conv2d(
            &g_dw,
            pw,
            &g_pw) != HYB_W) {
        return -4;
    }

    noodle_tensor_free(&g_dw);

    int8_t *dst =
        noodle_i8_tensor_require_2d(
            output,
            HYB_C,
            HYB_W,
            rpw->output_scale,
            rpw->output_zero_point);

    if (!dst) return -5;

    const uint32_t plane =
        HYB_W * HYB_W;

    const uint32_t a_bytes =
        HYB_A_C * plane;

    const uint32_t pw_bytes =
        HYB_PW_C * plane;

    memcpy(
        dst,
        g_branch_a.buffer.data,
        a_bytes);

    memcpy(
        dst + a_bytes,
        g_pw.buffer.data,
        pw_bytes);

    noodle_tensor_free(&g_branch_a);
    noodle_tensor_free(&g_pw);

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

    const CuteModelHeader *header =
        cute_model_header();

    if (!header) return -101;

    reset_all();

    if (header->input_zero_point != -128) {
        return -102;
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

    const CuteLayerId blocks[5][3] = {
        { CUTE_LAYER_H1A, CUTE_LAYER_H1DW, CUTE_LAYER_H1PW },
        { CUTE_LAYER_H2A, CUTE_LAYER_H2DW, CUTE_LAYER_H2PW },
        { CUTE_LAYER_H3A, CUTE_LAYER_H3DW, CUTE_LAYER_H3PW },
        { CUTE_LAYER_H4A, CUTE_LAYER_H4DW, CUTE_LAYER_H4PW },
        { CUTE_LAYER_H5A, CUTE_LAYER_H5DW, CUTE_LAYER_H5PW }
    };

    NoodleTensor *in = &g_feat_a;
    NoodleTensor *out = &g_feat_b;

    for (int block = 0; block < 5; ++block) {
        rc = run_hybrid(
            in,
            out,
            blocks[block][0],
            blocks[block][1],
            blocks[block][2]);

        if (rc) {
            return -200 - block * 10 + rc;
        }

        NoodleTensor *tmp = in;
        in = out;
        out = tmp;
    }

    const CuteLayerRecord *rh =
        cute_model_layer(CUTE_LAYER_HEAD);

    if (!rh) return -300;

    const ConvMem head =
        make_conv(
            rh,
            CUTE_LAYER_HEAD,
            1, 0, 1, 0);

    if (noodle_i8_conv2d(
            in,
            head,
            &g_head) != HEAD_W ||
        !g_head.buffer.data) {
        return -301;
    }

    noodle_tensor_free(in);

    *output_data = g_head.buffer.data;
    *output_size = HEAD_SIZE;

    return 0;
}
