#include <stdint.h>
#include <stddef.h>

#include "noodle.h"
#include "noodle_int8.h"
#include "cute_stem12_weights.h"

static NoodleTensor g_input;
static NoodleTensor g_s1;
static NoodleTensor g_s2;
static int g_initialized = 0;

static void reset_tensors(void)
{
    if (g_initialized) {
        noodle_tensor_free(&g_input);
        noodle_tensor_free(&g_s1);
        noodle_tensor_free(&g_s2);
    }

    noodle_tensor_init(&g_input);
    noodle_tensor_init(&g_s1);
    noodle_tensor_init(&g_s2);
    g_initialized = 1;
}

static ConvMem make_conv(
    uint16_t in_c,
    uint16_t out_c,
    const int8_t *w,
    const int32_t *b,
    const int32_t *m,
    const int32_t *s,
    float in_scale,
    int32_t in_zp,
    float out_scale,
    int32_t out_zp)
{
    (void)in_c;

    ConvMem conv = {};
    conv.K = 3;
    conv.P = 1;
    conv.S = 2;
    conv.OP = 0;
    conv.O = out_c;

    conv.weight = w;
    conv.bias = b;
    conv.multiplier = m;
    conv.shift = s;

    conv.input_scale = in_scale;
    conv.input_zero_point = in_zp;
    conv.output_scale = out_scale;
    conv.output_zero_point = out_zp;

    /* Both stem layers are Conv + ReLU. Their output zero-point is -128,
       so real zero is the INT8 lower bound. */
    conv.activation_min = out_zp;
    conv.activation_max = 127;

    conv.depth_multiplier = 1;
    conv.act = ACT_RELU;
    return conv;
}

extern "C" int cute_stem12_run(
    const uint8_t *gray_u8,
    uint32_t n,
    const int8_t **output_data,
    uint32_t *output_size)
{
    if (!gray_u8 || !output_data || !output_size) return -1;
    if (n != (CUTE_INPUT_W * CUTE_INPUT_W)) return -2;

    reset_tensors();

    int8_t *x = noodle_i8_tensor_require_2d(
        &g_input,
        CUTE_INPUT_C,
        CUTE_INPUT_W,
        CUTE_INPUT_SCALE,
        CUTE_INPUT_ZERO_POINT);

    if (!x) return -3;

    for (uint32_t i = 0; i < n; ++i) {
        x[i] = (int8_t)((int32_t)gray_u8[i] - 128);
    }

    const ConvMem stem1 = make_conv(
        CUTE_INPUT_C, CUTE_STEM1_C,
        cute_w1, cute_b1, cute_m1, cute_s1,
        CUTE_INPUT_SCALE, CUTE_INPUT_ZERO_POINT,
        CUTE_STEM1_SCALE, CUTE_STEM1_ZERO_POINT);

    const uint16_t w1 =
        noodle_i8_conv2d(&g_input, stem1, &g_s1);

    if (w1 != CUTE_STEM1_W || !g_s1.buffer.data) return -4;

    /* The original 128x128 input tensor is no longer needed.
       Freeing it compacts the Noodle arena before allocating Stem 2,
       keeping the peak activation arena at 48 KiB rather than 64 KiB. */
    noodle_tensor_free(&g_input);

    const ConvMem stem2 = make_conv(
        CUTE_STEM1_C, CUTE_STEM2_C,
        cute_w2, cute_b2, cute_m2, cute_s2,
        CUTE_STEM1_SCALE, CUTE_STEM1_ZERO_POINT,
        CUTE_STEM2_SCALE, CUTE_STEM2_ZERO_POINT);

    const uint16_t w2 =
        noodle_i8_conv2d(&g_s1, stem2, &g_s2);

    if (w2 != CUTE_STEM2_W || !g_s2.buffer.data) return -5;

    /* Stem 1 output is now dead. Remove it; Noodle refreshes g_s2's pointer
       after arena compaction. At return, only the 16 KiB Stem-2 tensor remains. */
    noodle_tensor_free(&g_s1);

    *output_data = g_s2.buffer.data;
    *output_size = (uint32_t)CUTE_STEM2_SIZE;
    return 0;
}
