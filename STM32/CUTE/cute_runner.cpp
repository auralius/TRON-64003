#include "cute_runner.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "noodle.h"
#include "cute_model_runtime.h"

#if !defined(NOODLE_USE_INT8)
#error "STM32 CUTE runner requires NOODLE_USE_INT8"
#endif

#if !defined(NOODLE_USE_NONE)
#error "STM32 CUTE runner currently requires NOODLE_USE_NONE"
#endif

namespace {

static constexpr uint16_t CUTE_TFLITE_SAME_PADDING = 65535u;
static constexpr uint16_t IMG_W = CUTE_YOLO_INPUT_W;
static constexpr uint16_t IMG_H = CUTE_YOLO_INPUT_H;
static constexpr uint32_t HEAD_BYTES =
    (uint32_t)CUTE_YOLO_OUTPUT_C * CUTE_YOLO_GRID_W * CUTE_YOLO_GRID_W;

/*
 * Single-core retained tensors.
 *
 * X     : 1 x 128 x 128 = 16384 B
 * A     : 8 x  64 x  64 = 32768 B
 * B     :16 x  32 x  32 = 16384 B
 * BR0   : 4 x  64 x  64 = 16384 B
 * BR1   : 4 x  64 x  64 = 16384 B
 * DWTMP :32 x  16 x  16 =  8192 B
 * ---------------------------------
 * total retained capacity          =106496 B
 *
 * Unlike the ESP32-S3 implementation, the two branches execute sequentially,
 * so a single DW temporary tensor is sufficient.
 */
static NoodleTensor X;
static NoodleTensor A;
static NoodleTensor B;
static NoodleTensor BR0;
static NoodleTensor BR1;
static NoodleTensor DWTMP;

static bool g_initialized = false;
static bool g_ready = false;
static NoodleTensor *g_head = nullptr;
static char g_error[48] = "NOT INIT";

struct HybridConvSet {
    ConvMem dw_a;
    ConvMem pw_a;
    ConvMem dw_b;
    ConvMem pw_b;
};

static void set_error(const char *s)
{
    if (!s) s = "ERR";
    snprintf(g_error, sizeof(g_error), "%s", s);
}

static Pool no_pool()
{
    Pool p;
    p.M = 1;
    p.T = 1;
    return p;
}

static void make_conv(
    ConvMem &conv,
    uint16_t K,
    uint16_t P,
    uint16_t S,
    uint16_t O,
    const NoodleWeight *weight,
    const NoodleBias *bias,
    const int32_t *multiplier,
    const int32_t *shift,
    float input_scale,
    int32_t input_zero_point,
    float output_scale,
    int32_t output_zero_point,
    Activation activation)
{
    conv.K = K;
    conv.P = P;
    conv.S = S;
    conv.OP = 0;
    conv.O = O;
    conv.weight = weight;
    conv.bias = bias;
    conv.multiplier = multiplier;
    conv.shift = shift;
    conv.input_scale = input_scale;
    conv.input_zero_point = input_zero_point;
    conv.output_scale = output_scale;
    conv.output_zero_point = output_zero_point;
    conv.activation_min = -128;
    conv.activation_max = 127;
    conv.depth_multiplier = 1;
    conv.act = activation;
}

static uint16_t hybrid_layer_index(uint8_t block_index, uint8_t component_offset)
{
    return static_cast<uint16_t>(
        CUTE_LAYER_HYBRID_BASE +
        static_cast<uint16_t>(4u * block_index) +
        component_offset);
}

static bool configure_hybrid_block(uint8_t block_index, HybridConvSet &block)
{
    const uint8_t H = cute_model_hybrid_blocks();
    if (block_index >= H) return false;

    const uint16_t dwa_id = hybrid_layer_index(block_index, 0);
    const uint16_t pwa_id = hybrid_layer_index(block_index, 1);
    const uint16_t dwb_id = hybrid_layer_index(block_index, 2);
    const uint16_t pwb_id = hybrid_layer_index(block_index, 3);

    const CuteLayerRecord *dwa = cute_model_layer(dwa_id);
    const CuteLayerRecord *pwa = cute_model_layer(pwa_id);
    const CuteLayerRecord *dwb = cute_model_layer(dwb_id);
    const CuteLayerRecord *pwb = cute_model_layer(pwb_id);
    if (!dwa || !pwa || !dwb || !pwb) return false;

    make_conv(block.dw_a, 3, 1, 1, CUTE_YOLO_HYBRID_DW_BRANCH_CHANNELS,
              cute_model_weight(dwa_id), cute_model_bias(dwa_id),
              cute_model_multiplier(dwa_id), cute_model_shift(dwa_id),
              dwa->input_scale, dwa->input_zero_point,
              dwa->output_scale, dwa->output_zero_point,
              ACT_RELU);
    block.dw_a.depth_multiplier = 1;

    make_conv(block.pw_a, 1, 0, 1, CUTE_YOLO_HYBRID_PW_BRANCH_OUT,
              cute_model_weight(pwa_id), cute_model_bias(pwa_id),
              cute_model_multiplier(pwa_id), cute_model_shift(pwa_id),
              pwa->input_scale, pwa->input_zero_point,
              pwa->output_scale, pwa->output_zero_point,
              ACT_RELU);

    make_conv(block.dw_b, 3, 1, 1, CUTE_YOLO_HYBRID_DW_BRANCH_CHANNELS,
              cute_model_weight(dwb_id), cute_model_bias(dwb_id),
              cute_model_multiplier(dwb_id), cute_model_shift(dwb_id),
              dwb->input_scale, dwb->input_zero_point,
              dwb->output_scale, dwb->output_zero_point,
              ACT_RELU);
    block.dw_b.depth_multiplier = 1;

    make_conv(block.pw_b, 1, 0, 1, CUTE_YOLO_HYBRID_PW_BRANCH_OUT,
              cute_model_weight(pwb_id), cute_model_bias(pwb_id),
              cute_model_multiplier(pwb_id), cute_model_shift(pwb_id),
              pwb->input_scale, pwb->input_zero_point,
              pwb->output_scale, pwb->output_zero_point,
              ACT_RELU);

    return true;
}

static bool concat_branches(NoodleTensor *output, uint16_t expected_c)
{
    const uint16_t c = noodle_concat(&BR0, &BR1, output);
    return c == expected_c;
}

static bool run_split_conv(
    NoodleTensor *input,
    NoodleTensor *output,
    const ConvMem &conv_a,
    const ConvMem &conv_b,
    const char *stage)
{
    const Pool p = no_pool();

    if (!noodle_conv2d(input, &BR0, conv_a, p)) {
        char b[40]; snprintf(b, sizeof(b), "%s-A", stage); set_error(b); return false;
    }
    if (!noodle_conv2d(input, &BR1, conv_b, p)) {
        char b[40]; snprintf(b, sizeof(b), "%s-B", stage); set_error(b); return false;
    }
    if (!concat_branches(output, static_cast<uint16_t>(conv_a.O + conv_b.O))) {
        char b[40]; snprintf(b, sizeof(b), "%s-CAT", stage); set_error(b); return false;
    }
    return true;
}

static bool run_hybrid(
    uint8_t block_index,
    NoodleTensor *input,
    NoodleTensor *output,
    const HybridConvSet &h)
{
    const Pool p = no_pool();
    char stage[24];

    if (!noodle_dwconv2d(input, &DWTMP, h.dw_a, p)) {
        snprintf(stage, sizeof(stage), "H%u-DWA", (unsigned)(block_index + 1));
        set_error(stage); return false;
    }
    if (!noodle_conv2d(&DWTMP, &BR0, h.pw_a, p)) {
        snprintf(stage, sizeof(stage), "H%u-PWA", (unsigned)(block_index + 1));
        set_error(stage); return false;
    }

    /* Reuse the same DW temporary now that branch A's PW result is in BR0. */
    if (!noodle_dwconv2d(input, &DWTMP, h.dw_b, p)) {
        snprintf(stage, sizeof(stage), "H%u-DWB", (unsigned)(block_index + 1));
        set_error(stage); return false;
    }
    if (!noodle_conv2d(&DWTMP, &BR1, h.pw_b, p)) {
        snprintf(stage, sizeof(stage), "H%u-PWB", (unsigned)(block_index + 1));
        set_error(stage); return false;
    }

    if (!concat_branches(output, CUTE_YOLO_HYBRID_CONCAT_CHANNELS)) {
        snprintf(stage, sizeof(stage), "H%u-CAT", (unsigned)(block_index + 1));
        set_error(stage); return false;
    }

    return true;
}

static bool fill_input_from_gray8(const uint8_t *gray)
{
    if (!gray) return false;

    noodle_tensor_set_quantization(
        &X,
        CUTE_YOLO_INPUT_SCALE,
        CUTE_YOLO_INPUT_ZERO_POINT);

    NoodleData *x = noodle_tensor_require_2d(&X, 1, IMG_W);
    if (!x) return false;

    for (uint32_t i = 0; i < (uint32_t)IMG_W * IMG_H; ++i) {
        const float value01 = (float)gray[i] * (1.0f / 255.0f);
        x[i] = (NoodleData)noodle_quantize_float(
            value01,
            CUTE_YOLO_INPUT_SCALE,
            CUTE_YOLO_INPUT_ZERO_POINT);
    }
    return true;
}

static bool run_network()
{
    if (!cute_model_ready()) { set_error("NO MODEL"); return false; }

    const uint8_t H = cute_model_hybrid_blocks();
    if (H < CUTE_MODEL_MIN_HYBRID_BLOCKS || H > CUTE_MODEL_MAX_HYBRID_BLOCKS) {
        set_error("BAD H"); return false;
    }

    const Pool p = no_pool();

    ConvMem s1a, s1b, s2a, s2b, s3a, s3b;
    HybridConvSet hybrid;
    ConvMem head_linear, head_hidden, head_obj, head_box;

    make_conv(s1a, 3, CUTE_TFLITE_SAME_PADDING, 2,
              CUTE_YOLO_STEM1_BRANCH_A_OUT,
              w01a, b01a, m01a, s01a,
              CUTE_YOLO_INPUT_SCALE, CUTE_YOLO_INPUT_ZERO_POINT,
              CUTE_YOLO_STEM1_OUTPUT_SCALE, CUTE_YOLO_STEM1_OUTPUT_ZERO_POINT,
              ACT_RELU);
    make_conv(s1b, 3, CUTE_TFLITE_SAME_PADDING, 2,
              CUTE_YOLO_STEM1_BRANCH_B_OUT,
              w01b, b01b, m01b, s01b,
              CUTE_YOLO_INPUT_SCALE, CUTE_YOLO_INPUT_ZERO_POINT,
              CUTE_YOLO_STEM1_OUTPUT_SCALE, CUTE_YOLO_STEM1_OUTPUT_ZERO_POINT,
              ACT_RELU);

    make_conv(s2a, 3, CUTE_TFLITE_SAME_PADDING, 2,
              CUTE_YOLO_STEM2_BRANCH_A_OUT,
              w02a, b02a, m02a, s02a,
              CUTE_YOLO_STEM1_OUTPUT_SCALE, CUTE_YOLO_STEM1_OUTPUT_ZERO_POINT,
              CUTE_YOLO_STEM2_OUTPUT_SCALE, CUTE_YOLO_STEM2_OUTPUT_ZERO_POINT,
              ACT_RELU);
    make_conv(s2b, 3, CUTE_TFLITE_SAME_PADDING, 2,
              CUTE_YOLO_STEM2_BRANCH_B_OUT,
              w02b, b02b, m02b, s02b,
              CUTE_YOLO_STEM1_OUTPUT_SCALE, CUTE_YOLO_STEM1_OUTPUT_ZERO_POINT,
              CUTE_YOLO_STEM2_OUTPUT_SCALE, CUTE_YOLO_STEM2_OUTPUT_ZERO_POINT,
              ACT_RELU);

    make_conv(s3a, 3, CUTE_TFLITE_SAME_PADDING, 2,
              CUTE_YOLO_STEM3_BRANCH_A_OUT,
              w03a, b03a, m03a, s03a,
              CUTE_YOLO_STEM2_OUTPUT_SCALE, CUTE_YOLO_STEM2_OUTPUT_ZERO_POINT,
              CUTE_YOLO_STEM3_OUTPUT_SCALE, CUTE_YOLO_STEM3_OUTPUT_ZERO_POINT,
              ACT_RELU);
    make_conv(s3b, 3, CUTE_TFLITE_SAME_PADDING, 2,
              CUTE_YOLO_STEM3_BRANCH_B_OUT,
              w03b, b03b, m03b, s03b,
              CUTE_YOLO_STEM2_OUTPUT_SCALE, CUTE_YOLO_STEM2_OUTPUT_ZERO_POINT,
              CUTE_YOLO_STEM3_OUTPUT_SCALE, CUTE_YOLO_STEM3_OUTPUT_ZERO_POINT,
              ACT_RELU);

    if (!run_split_conv(&X, &A, s1a, s1b, "S1")) return false;
    if (!run_split_conv(&A, &B, s2a, s2b, "S2")) return false;
    if (!run_split_conv(&B, &A, s3a, s3b, "S3")) return false;

    NoodleTensor *current = &A;
    NoodleTensor *next = &B;

    for (uint8_t block = 0; block < H; ++block) {
        if (!configure_hybrid_block(block, hybrid)) {
            char b[24]; snprintf(b, sizeof(b), "H%u-CFG", (unsigned)(block + 1));
            set_error(b); return false;
        }
        if (!run_hybrid(block, current, next, hybrid)) return false;
        NoodleTensor *tmp = current; current = next; next = tmp;
    }

    const bool mlp_head = cute_model_has_mlp_head();
    const uint8_t M = mlp_head ? cute_model_head_hidden_layers() : 0u;
    const uint16_t head_id = cute_model_head_layer_index();
    const CuteLayerRecord *head_record = cute_model_layer(head_id);
    if (!head_record) { set_error("HEAD-CFG"); return false; }

    if (mlp_head) {
        if (M < CUTE_MODEL_MIN_HEAD_HIDDEN_LAYERS ||
            M > CUTE_MODEL_MAX_HEAD_HIDDEN_LAYERS) {
            set_error("BAD M"); return false;
        }

        for (uint8_t hidden = 0; hidden < M; ++hidden) {
            const uint16_t id = cute_model_head_hidden_layer_index(hidden);
            const CuteLayerRecord *r = cute_model_layer(id);
            if (!r) { set_error("M-CFG"); return false; }

            make_conv(head_hidden, 1, 0, 1, CUTE_YOLO_HEAD_HIDDEN_CHANNELS,
                      cute_model_weight(id), cute_model_bias(id),
                      cute_model_multiplier(id), cute_model_shift(id),
                      r->input_scale, r->input_zero_point,
                      r->output_scale, r->output_zero_point,
                      ACT_RELU);

            if (!noodle_conv2d(current, next, head_hidden, p)) {
                char b[24]; snprintf(b, sizeof(b), "M%u", (unsigned)(hidden + 1));
                set_error(b); return false;
            }
            NoodleTensor *tmp = current; current = next; next = tmp;
        }

        const uint16_t obj_id = cute_model_head_objectness_layer_index();
        const uint16_t box_id = cute_model_head_box_layer_index();
        const CuteLayerRecord *obj = cute_model_layer(obj_id);
        const CuteLayerRecord *box = cute_model_layer(box_id);
        if (!obj || !box) { set_error("OUT-CFG"); return false; }

        make_conv(head_obj, 1, 0, 1, 1,
                  cute_model_weight(obj_id), cute_model_bias(obj_id),
                  cute_model_multiplier(obj_id), cute_model_shift(obj_id),
                  obj->input_scale, obj->input_zero_point,
                  obj->output_scale, obj->output_zero_point,
                  ACT_NONE);
        make_conv(head_box, 1, 0, 1, 4,
                  cute_model_weight(box_id), cute_model_bias(box_id),
                  cute_model_multiplier(box_id), cute_model_shift(box_id),
                  box->input_scale, box->input_zero_point,
                  box->output_scale, box->output_zero_point,
                  ACT_NONE);

        if (!run_split_conv(current, next, head_obj, head_box, "OUT")) return false;
        g_head = next;
    } else {
        make_conv(head_linear, 1, 0, 1, 5,
                  cute_model_weight(head_id), cute_model_bias(head_id),
                  cute_model_multiplier(head_id), cute_model_shift(head_id),
                  head_record->input_scale, head_record->input_zero_point,
                  head_record->output_scale, head_record->output_zero_point,
                  ACT_NONE);

        if (!noodle_conv2d(current, next, head_linear, p)) {
            set_error("HEAD"); return false;
        }
        g_head = next;
    }

    if (!g_head || g_head->C != CUTE_YOLO_OUTPUT_C ||
        g_head->W != CUTE_YOLO_GRID_W ||
        noodle_tensor_size(g_head) != HEAD_BYTES) {
        set_error("HEAD-SHAPE");
        g_head = nullptr;
        return false;
    }

    set_error("OK");
    return true;
}

static uint32_t crc32_bytes(const uint8_t *data, size_t n)
{
    uint32_t crc = 0xFFFFFFFFu;
    while (n--) {
        crc ^= *data++;
        for (uint8_t k = 0; k < 8; ++k) {
            const uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

} // namespace

extern "C" int cute_runner_begin(void)
{
    if (g_ready) return 1;
    if (!cute_model_ready()) {
        set_error("NO MODEL");
        return 0;
    }

    if (!g_initialized) {
        noodle_tensor_init(&X);
        noodle_tensor_init(&A);
        noodle_tensor_init(&B);
        noodle_tensor_init(&BR0);
        noodle_tensor_init(&BR1);
        noodle_tensor_init(&DWTMP);
        g_initialized = true;
    }

    noodle_tensor_set_quantization(&X,
                                   CUTE_YOLO_INPUT_SCALE,
                                   CUTE_YOLO_INPUT_ZERO_POINT);

    bool ok = true;
    ok &= noodle_tensor_require_2d(&X, 1, 128) != nullptr;
    ok &= noodle_tensor_require_2d(&A, 8, 64) != nullptr;
    ok &= noodle_tensor_require_2d(&B, 16, 32) != nullptr;
    ok &= noodle_tensor_require_2d(&BR0, 4, 64) != nullptr;
    ok &= noodle_tensor_require_2d(&BR1, 4, 64) != nullptr;
    ok &= noodle_tensor_require_2d(&DWTMP, 32, 16) != nullptr;

    if (!ok) {
        set_error("ARENA ALLOC");
        g_ready = false;
        return 0;
    }

    g_head = nullptr;
    g_ready = true;
    set_error("READY");
    return 1;
}

extern "C" int cute_runner_ready(void)
{
    return g_ready ? 1 : 0;
}

extern "C" int cute_runner_run_gray8(const uint8_t *gray_128x128)
{
    if (!g_ready && !cute_runner_begin()) return 0;
    g_head = nullptr;

    if (!fill_input_from_gray8(gray_128x128)) {
        set_error("INPUT");
        return 0;
    }

    return run_network() ? 1 : 0;
}

extern "C" const char *cute_runner_last_error(void)
{
    return g_error;
}

extern "C" const int8_t *cute_runner_head_data(void)
{
    return g_head ? reinterpret_cast<const int8_t *>(noodle_tensor_const_data(g_head)) : nullptr;
}

extern "C" uint32_t cute_runner_head_bytes(void)
{
    return g_head ? static_cast<uint32_t>(noodle_tensor_size(g_head)) : 0u;
}

extern "C" float cute_runner_head_scale(void)
{
    return g_head ? g_head->scale : 0.0f;
}

extern "C" int32_t cute_runner_head_zero_point(void)
{
    return g_head ? g_head->zero_point : 0;
}

extern "C" uint32_t cute_runner_head_crc32(void)
{
    const int8_t *p = cute_runner_head_data();
    const uint32_t n = cute_runner_head_bytes();
    return (p && n) ? crc32_bytes(reinterpret_cast<const uint8_t *>(p), n) : 0u;
}

extern "C" size_t cute_runner_arena_used_bytes(void)
{
    return noodle_buffer_arena_used_bytes();
}

extern "C" size_t cute_runner_arena_capacity_bytes(void)
{
    return noodle_buffer_arena_capacity_bytes();
}
