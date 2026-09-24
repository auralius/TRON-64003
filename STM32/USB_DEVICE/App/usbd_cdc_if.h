#ifndef __USBD_CDC_IF_H__
#define __USBD_CDC_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "usbd_cdc.h"

extern USBD_CDC_ItfTypeDef USBD_CDC_fops;

/* Non-blocking: returns USBD_OK, USBD_BUSY, or USBD_FAIL. */
uint8_t CDC_Transmit_FS(uint8_t *buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CDC_IF_H__ */
