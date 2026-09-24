#include "cute_bridge.h"
#include "cute_model_manager.h"
#include "cute_runner.h"

extern "C" int cute_model_probe(void)
{
    return cute_model_begin() ? 1 : 0;
}

extern "C" const char *cute_model_status_c(void)
{
    return cute_model_status();
}

extern "C" int cute_runtime_begin(void)
{
    return cute_runner_begin();
}

extern "C" int cute_runtime_ready(void)
{
    return cute_runner_ready();
}

extern "C" int cute_runtime_run_gray8(const uint8_t *gray_128x128)
{
    return cute_runner_run_gray8(gray_128x128);
}

extern "C" const char *cute_runtime_error_c(void)
{
    return cute_runner_last_error();
}

extern "C" const int8_t *cute_runtime_head_data(void)
{
    return cute_runner_head_data();
}

extern "C" uint32_t cute_runtime_head_bytes(void)
{
    return cute_runner_head_bytes();
}

extern "C" float cute_runtime_head_scale(void)
{
    return cute_runner_head_scale();
}

extern "C" int32_t cute_runtime_head_zero_point(void)
{
    return cute_runner_head_zero_point();
}

extern "C" uint32_t cute_runtime_head_crc32(void)
{
    return cute_runner_head_crc32();
}

extern "C" size_t cute_runtime_arena_used_bytes(void)
{
    return cute_runner_arena_used_bytes();
}

extern "C" size_t cute_runtime_arena_capacity_bytes(void)
{
    return cute_runner_arena_capacity_bytes();
}
