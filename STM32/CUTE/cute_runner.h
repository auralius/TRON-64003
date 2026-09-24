#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Single-core STM32H743 CUTE-YOLO runner.
 *
 * This is deliberately NOT the muT-Kernel integration yet. It is a synchronous
 * runner called from the existing Cube/HAL super-loop. The topology and model
 * parameters are read from the active memory-mapped .cute package.
 */

int cute_runner_begin(void);
int cute_runner_ready(void);
int cute_runner_run_gray8(const uint8_t *gray_128x128);

const char *cute_runner_last_error(void);
const int8_t *cute_runner_head_data(void);
uint32_t cute_runner_head_bytes(void);
float cute_runner_head_scale(void);
int32_t cute_runner_head_zero_point(void);
uint32_t cute_runner_head_crc32(void);

size_t cute_runner_arena_used_bytes(void);
size_t cute_runner_arena_capacity_bytes(void);

#ifdef __cplusplus
}
#endif
