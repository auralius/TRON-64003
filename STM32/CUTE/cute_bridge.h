#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Model package probe / status. */
int cute_model_probe(void);
const char *cute_model_status_c(void);

/* Synchronous single-core STM32 Noodle runner. */
int cute_runtime_begin(void);
int cute_runtime_ready(void);
int cute_runtime_run_gray8(const uint8_t *gray_128x128);
const char *cute_runtime_error_c(void);

const int8_t *cute_runtime_head_data(void);
uint32_t cute_runtime_head_bytes(void);
float cute_runtime_head_scale(void);
int32_t cute_runtime_head_zero_point(void);
uint32_t cute_runtime_head_crc32(void);

size_t cute_runtime_arena_used_bytes(void);
size_t cute_runtime_arena_capacity_bytes(void);

#ifdef __cplusplus
}
#endif
