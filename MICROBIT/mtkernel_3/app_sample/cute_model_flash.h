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
const CuteModelHeaderBase *cute_model_header(void);
const CuteLayerRecord *cute_model_layer(uint16_t index);

const int8_t *cute_model_weight(uint16_t index);
const int32_t *cute_model_bias(uint16_t index);
const int32_t *cute_model_multiplier(uint16_t index);
const int32_t *cute_model_shift(uint16_t index);

uint16_t cute_model_format_version(void);
uint16_t cute_model_header_bytes(void);
uint16_t cute_model_layer_count(void);
uint32_t cute_model_total_bytes(void);
uint8_t cute_model_hybrid_blocks(void);
int cute_model_has_mlp_head(void);
uint8_t cute_model_head_hidden_layers(void);
uint16_t cute_model_head_layer_index(void);
uint16_t cute_model_head_hidden_layer_index(uint8_t hidden_index);
uint16_t cute_model_head_objectness_layer_index(void);
uint16_t cute_model_head_box_layer_index(void);

const char *cute_model_label(void);
float cute_model_confidence_threshold(void);
float cute_model_nms_iou_threshold(void);
float cute_model_min_box_w(void);
float cute_model_min_box_h(void);
uint8_t cute_model_max_detections(void);

/* Transport-neutral A/B updater.
 * USB serial currently calls these directly from RX/COMM.  A future BLE
 * transport can reuse the same storage functions.
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
