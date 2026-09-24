#pragma once

#include "cute_model_manager.h"

#define CUTE_YOLO_DUAL_DWPW_W32 1
#define CUTE_YOLO_N_HYBRID_DUAL_DWPW_32 1

#define CUTE_YOLO_INPUT_W 128
#define CUTE_YOLO_INPUT_H 128
#define CUTE_YOLO_INPUT_C 1
#define CUTE_YOLO_GRID_W 16
#define CUTE_YOLO_OUTPUT_C 5

#define CUTE_YOLO_STEM1_BRANCH_A_OUT 4
#define CUTE_YOLO_STEM1_BRANCH_B_OUT 4
#define CUTE_YOLO_STEM2_BRANCH_A_OUT 8
#define CUTE_YOLO_STEM2_BRANCH_B_OUT 8
#define CUTE_YOLO_STEM3_BRANCH_A_OUT 16
#define CUTE_YOLO_STEM3_BRANCH_B_OUT 16

#define CUTE_YOLO_HYBRID_CHANNELS 32
#define CUTE_YOLO_HYBRID_DW_BRANCH_CHANNELS 32
#define CUTE_YOLO_HYBRID_PW_BRANCH_OUT 16
#define CUTE_YOLO_HYBRID_CONCAT_CHANNELS 32
#define CUTE_YOLO_HEAD_HIDDEN_CHANNELS 32
#define CUTE_YOLO_STEM3_OUTPUT_CHANNELS 32

#define CUTE_YOLO_INPUT_SCALE              (cute_model_ready() ? cute_model_header()->input_scale : (1.0f / 255.0f))
#define CUTE_YOLO_INPUT_ZERO_POINT         (cute_model_ready() ? cute_model_header()->input_zero_point : -128)
#define CUTE_YOLO_OUTPUT_SCALE             (cute_model_ready() ? cute_model_header()->output_scale : 1.0f)
#define CUTE_YOLO_OUTPUT_ZERO_POINT        (cute_model_ready() ? cute_model_header()->output_zero_point : 0)

#define CUTE_YOLO_STEM1_OUTPUT_SCALE       (cute_model_layer(CUTE_LAYER_S1A)->output_scale)
#define CUTE_YOLO_STEM1_OUTPUT_ZERO_POINT  (cute_model_layer(CUTE_LAYER_S1A)->output_zero_point)
#define CUTE_YOLO_STEM2_OUTPUT_SCALE       (cute_model_layer(CUTE_LAYER_S2A)->output_scale)
#define CUTE_YOLO_STEM2_OUTPUT_ZERO_POINT  (cute_model_layer(CUTE_LAYER_S2A)->output_zero_point)
#define CUTE_YOLO_STEM3_OUTPUT_SCALE       (cute_model_layer(CUTE_LAYER_S3A)->output_scale)
#define CUTE_YOLO_STEM3_OUTPUT_ZERO_POINT  (cute_model_layer(CUTE_LAYER_S3A)->output_zero_point)

#define w01a   (cute_model_weight(CUTE_LAYER_S1A))
#define b01a   (cute_model_bias(CUTE_LAYER_S1A))
#define m01a   (cute_model_multiplier(CUTE_LAYER_S1A))
#define s01a   (cute_model_shift(CUTE_LAYER_S1A))
#define w01b   (cute_model_weight(CUTE_LAYER_S1B))
#define b01b   (cute_model_bias(CUTE_LAYER_S1B))
#define m01b   (cute_model_multiplier(CUTE_LAYER_S1B))
#define s01b   (cute_model_shift(CUTE_LAYER_S1B))
#define w02a   (cute_model_weight(CUTE_LAYER_S2A))
#define b02a   (cute_model_bias(CUTE_LAYER_S2A))
#define m02a   (cute_model_multiplier(CUTE_LAYER_S2A))
#define s02a   (cute_model_shift(CUTE_LAYER_S2A))
#define w02b   (cute_model_weight(CUTE_LAYER_S2B))
#define b02b   (cute_model_bias(CUTE_LAYER_S2B))
#define m02b   (cute_model_multiplier(CUTE_LAYER_S2B))
#define s02b   (cute_model_shift(CUTE_LAYER_S2B))
#define w03a   (cute_model_weight(CUTE_LAYER_S3A))
#define b03a   (cute_model_bias(CUTE_LAYER_S3A))
#define m03a   (cute_model_multiplier(CUTE_LAYER_S3A))
#define s03a   (cute_model_shift(CUTE_LAYER_S3A))
#define w03b   (cute_model_weight(CUTE_LAYER_S3B))
#define b03b   (cute_model_bias(CUTE_LAYER_S3B))
#define m03b   (cute_model_multiplier(CUTE_LAYER_S3B))
#define s03b   (cute_model_shift(CUTE_LAYER_S3B))
