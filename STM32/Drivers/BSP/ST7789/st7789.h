#ifndef __ST7789_H
#define __ST7789_H

#include "main.h"
#include <stdint.h>

/*
 * 1.14-inch 135x240 IPS panel.
 *
 * The controller GRAM is larger than the visible area, so this panel needs
 * offsets.  The previous working build used landscape rotation 3:
 *
 *     240x135, XSTART=40, YSTART=53, MADCTL=0x68
 *
 * This build deliberately changes to portrait rotation 0:
 *
 *     135x240, XSTART=53, YSTART=40, MADCTL=0xC8
 *
 * 0xC8 = MX | MY | BGR.  This is the 90-degree partner of the already-working
 * 0x68 landscape orientation, so the electrical/init path stays unchanged.
 *
 * If the complete portrait image is upside-down, use 0x08 instead of 0xC8.
 */
#define ST7789_WIDTH        135U
#define ST7789_HEIGHT       240U
#define ST7789_XSTART        53U
#define ST7789_YSTART        40U
#define ST7789_MADCTL_VALUE 0xC8U

#define ST7789_BLACK       0x0000U
#define ST7789_BLUE        0x001FU
#define ST7789_RED         0xF800U
#define ST7789_GREEN       0x07E0U
#define ST7789_CYAN        0x07FFU
#define ST7789_MAGENTA     0xF81FU
#define ST7789_YELLOW      0xFFE0U
#define ST7789_WHITE       0xFFFFU

int32_t ST7789_Init(void);
void ST7789_FillScreen(uint16_t color);
void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_DrawRGB565(uint16_t x,
                       uint16_t y,
                       const uint8_t *rgb565_be,
                       uint16_t w,
                       uint16_t h);

#endif /* __ST7789_H */
