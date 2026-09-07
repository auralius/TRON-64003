#ifndef CUTE_MODEL_FLASH_H
#define CUTE_MODEL_FLASH_H

#include <stdint.h>

#include "cute_model_format.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Boot-time model selection. Validates metadata and both slots. */
int cute_model_flash_init(void);

int cute_model_ready(void);
char cute_model_active_slot(void);
const CuteModelHeader *cute_model_header(void);
const CuteLayerRecord *cute_model_layer(CuteLayerId id);

const int8_t *cute_model_weight(CuteLayerId id);
const int32_t *cute_model_bias(CuteLayerId id);
const int32_t *cute_model_multiplier(CuteLayerId id);
const int32_t *cute_model_shift(CuteLayerId id);

const char *cute_model_label(void);
float cute_model_confidence_threshold(void);
float cute_model_nms_iou_threshold(void);
float cute_model_min_box_w(void);
float cute_model_min_box_h(void);
uint8_t cute_model_max_detections(void);

/* Transport-neutral A/B updater.
 *
 * The current test transport is USB serial. The later S113 BLE endpoint
 * will call these exact same functions from its CTRL/DATA handlers.
 */
int cute_model_upload_begin(uint32_t total_bytes, char *target_slot);
int cute_model_upload_write(const uint8_t *data, uint32_t n);
int cute_model_upload_end(char *activated_slot);
void cute_model_upload_abort(void);
uint32_t cute_model_upload_received(void);
uint32_t cute_model_upload_expected(void);

#ifdef __cplusplus
}
#endif

#endif
