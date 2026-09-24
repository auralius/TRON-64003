#ifndef __LCD_H
#define __LCD_H

#include "main.h"
#include "st7789.h"
#include <stdint.h>

#define LCD_WIDTH  ST7789_WIDTH
#define LCD_HEIGHT ST7789_HEIGHT

#define WHITE      ST7789_WHITE
#define BLACK      ST7789_BLACK
#define BLUE       ST7789_BLUE
#define RED        ST7789_RED
#define MAGENTA    ST7789_MAGENTA
#define GREEN      ST7789_GREEN
#define CYAN       ST7789_CYAN
#define YELLOW     ST7789_YELLOW

/* Set to 0 after the panel geometry has been verified. */
#ifndef LCD_BOOT_TEST_PATTERN
#define LCD_BOOT_TEST_PATTERN 0
#endif

int32_t LCD_Init(void);
void LCD_Test(void);
void LCD_SetBrightness(uint32_t brightness);
uint32_t LCD_GetBrightness(void);
void LCD_SoftPWMEnable(uint8_t enable);
uint8_t LCD_SoftPWMIsEnable(void);
void LCD_SoftPWMCtrlInit(void);
void LCD_SoftPWMCtrlDeInit(void);
void LCD_SoftPWMCtrlRun(void);
void LCD_FillScreen(uint16_t color);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void LCD_DrawRGB565(uint16_t x,
                    uint16_t y,
                    const uint16_t *pixels,
                    uint16_t w,
                    uint16_t h);

/*
 * Small ASCII compatibility layer used by the CUTE result/status display.
 * The API matches the old WeAct ST7735 helper so application code does not
 * need a separate font implementation.
 */
extern uint16_t POINT_COLOR;
extern uint16_t BACK_COLOR;

void LCD_ShowChar(uint16_t x,
                  uint16_t y,
                  uint8_t num,
                  uint8_t size,
                  uint8_t mode);

void LCD_ShowString(uint16_t x,
                    uint16_t y,
                    uint16_t width,
                    uint16_t height,
                    uint8_t size,
                    uint8_t *p);

#endif /* __LCD_H */
