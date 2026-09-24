#include "lcd.h"
#include "tim.h"
#include "font.h"
#include <string.h>

static uint32_t lcd_brightness = 0U;
static uint8_t lcd_soft_pwm = 0U;

int32_t LCD_Init(void)
{
    /* Same proven WeAct backlight PWM path used by the original ST7735. */
    if (HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2) != HAL_OK)
        return -1;

    LCD_SetBrightness(0U);

    if (ST7789_Init() != 0)
        return -1;

    LCD_FillScreen(BLACK);
    LCD_SetBrightness(600U);
    return 0;
}

void LCD_Test(void)
{
    if (LCD_Init() != 0)
    {
        /* Leave the screen dark; heartbeat/CDC can still expose the failure. */
        return;
    }

#if LCD_BOOT_TEST_PATTERN
    /*
     * Portrait geometry/color test before μT-Kernel starts.
     *
     * Expected from TOP to BOTTOM:
     *   RED | GREEN | BLUE | WHITE
     *
     * A small MAGENTA marker is added at the logical top-left and a CYAN
     * marker at the logical bottom-right.  These make a 180-degree mistake
     * immediately obvious.
     */
    const uint16_t bar_h = LCD_HEIGHT / 4U;
    LCD_FillRect(0U, 0U,         LCD_WIDTH, bar_h,                  RED);
    LCD_FillRect(0U, bar_h,      LCD_WIDTH, bar_h,                  GREEN);
    LCD_FillRect(0U, bar_h * 2U, LCD_WIDTH, bar_h,                  BLUE);
    LCD_FillRect(0U, bar_h * 3U, LCD_WIDTH, LCD_HEIGHT-bar_h * 3U, WHITE);

    LCD_FillRect(0U, 0U, 12U, 12U, MAGENTA);
    LCD_FillRect(LCD_WIDTH - 12U, LCD_HEIGHT - 12U, 12U, 12U, CYAN);

    HAL_Delay(800U);
    LCD_FillScreen(BLACK);
#endif
}

void LCD_SetBrightness(uint32_t brightness)
{
    if (brightness > 999U)
        brightness = 999U;

    lcd_brightness = brightness;
    if (!lcd_soft_pwm)
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, brightness);
}

uint32_t LCD_GetBrightness(void)
{
    if (lcd_soft_pwm)
        return lcd_brightness;
    return __HAL_TIM_GET_COMPARE(&htim1, TIM_CHANNEL_2);
}

void LCD_SoftPWMEnable(uint8_t enable)
{
    lcd_soft_pwm = enable ? 1U : 0U;
    if (!lcd_soft_pwm)
        LCD_SetBrightness(lcd_brightness);
}

uint8_t LCD_SoftPWMIsEnable(void)
{
    return lcd_soft_pwm;
}

void LCD_SoftPWMCtrlInit(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOE_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    MX_TIM16_Init();
    HAL_TIM_Base_Start_IT(&htim16);
    LCD_SoftPWMEnable(1U);
}

void LCD_SoftPWMCtrlDeInit(void)
{
    HAL_TIM_Base_DeInit(&htim16);
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_10);
}

void LCD_SoftPWMCtrlRun(void)
{
    static uint32_t timecount = 0U;

    if (timecount > 1000U)
        timecount = 0U;
    else
        timecount += 10U;

    if (timecount >= lcd_brightness)
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_RESET);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM16)
        LCD_SoftPWMCtrlRun();
}

void LCD_FillScreen(uint16_t color)
{
    ST7789_FillScreen(color);
}

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ST7789_FillRect(x, y, w, h, color);
}

void LCD_DrawRGB565(uint16_t x,
                    uint16_t y,
                    const uint16_t *pixels,
                    uint16_t w,
                    uint16_t h)
{
    /*
     * The camera buffer is intentionally sent as raw bytes. The DCMI RGB565
     * stream is already stored in the byte order expected by the LCD SPI path.
     */
    ST7789_DrawRGB565(x, y, (const uint8_t *)pixels, w, h);
}


/* -------------------------------------------------------------------------
 * Minimal text compatibility layer
 * ------------------------------------------------------------------------- */

uint16_t POINT_COLOR = WHITE;
uint16_t BACK_COLOR  = BLACK;

static uint16_t lcd_wire565(uint16_t color)
{
    /*
     * LCD_DrawRGB565() sends memory bytes verbatim.  Convert an ordinary
     * host-endian RGB565 constant to a uint16_t whose in-memory bytes are
     * high-byte first.
     */
    return (uint16_t)((color << 8) | (color >> 8));
}

void LCD_ShowChar(uint16_t x,
                  uint16_t y,
                  uint8_t num,
                  uint8_t size,
                  uint8_t mode)
{
    (void)mode; /* CUTE uses opaque text only. */

    if ((num < (uint8_t)' ') || (num > (uint8_t)'~'))
        return;

    if ((size != 12U) && (size != 16U))
        return;

    const uint16_t glyph_w = (uint16_t)(size / 2U);

    if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT))
        return;

    if (((uint32_t)x + glyph_w > LCD_WIDTH) ||
        ((uint32_t)y + size > LCD_HEIGHT))
        return;

    uint16_t glyph[16U * 8U];
    const uint16_t fg = lcd_wire565(POINT_COLOR);
    const uint16_t bg = lcd_wire565(BACK_COLOR);

    for (uint32_t i = 0U; i < (uint32_t)glyph_w * size; ++i)
        glyph[i] = bg;

    const uint8_t index = (uint8_t)(num - (uint8_t)' ');
    uint16_t row = 0U;

    for (uint8_t t = 0U; t < size; ++t)
    {
        uint8_t bits =
            (size == 12U) ? asc2_1206[index][t]
                          : asc2_1608[index][t];

        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            if ((bits & 0x80U) != 0U)
                glyph[(uint32_t)row * glyph_w + (t / 2U)] = fg;

            bits <<= 1;
            row++;

            if (row >= size)
            {
                row = 0U;
                break;
            }
        }
    }

    LCD_DrawRGB565(x, y, glyph, glyph_w, size);
}

void LCD_ShowString(uint16_t x,
                    uint16_t y,
                    uint16_t width,
                    uint16_t height,
                    uint8_t size,
                    uint8_t *p)
{
    if (p == NULL)
        return;

    const uint16_t x0 = x;
    const uint16_t x_end = (uint16_t)(x + width);
    const uint16_t y_end = (uint16_t)(y + height);

    while ((*p >= (uint8_t)' ') && (*p <= (uint8_t)'~'))
    {
        if ((uint32_t)x + (size / 2U) > x_end)
        {
            x = x0;
            y = (uint16_t)(y + size);
        }

        if ((uint32_t)y + size > y_end)
            break;

        LCD_ShowChar(x, y, *p, size, 0U);
        x = (uint16_t)(x + size / 2U);
        ++p;
    }
}
