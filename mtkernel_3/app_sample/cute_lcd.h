#ifndef CUTE_LCD_H
#define CUTE_LCD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CUTE_LCD_WIDTH   160u
#define CUTE_LCD_HEIGHT  128u

void cute_lcd_init(void);
void cute_lcd_clear(uint16_t rgb565);

/* Draw a 128x128 uint8 grayscale image centered horizontally:
 * logical LCD area x=16..143, y=0..127.
 */
void cute_lcd_draw_gray128(const uint8_t *gray);

/* Small helpers for the next Cute-YOLO step. */
void cute_lcd_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t rgb565);
void cute_lcd_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t rgb565);
void cute_lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rgb565);

#ifdef __cplusplus
}
#endif

#endif
