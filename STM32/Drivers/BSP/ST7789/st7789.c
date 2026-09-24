#include "st7789.h"
#include "spi.h"

#include <stddef.h>

/* Existing WeAct LCD connector signals. */
#define LCD_CS_LOW()   HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define LCD_CS_HIGH()  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)
#define LCD_DC_CMD()   HAL_GPIO_WritePin(LCD_WR_RS_GPIO_Port, LCD_WR_RS_Pin, GPIO_PIN_RESET)
#define LCD_DC_DATA()  HAL_GPIO_WritePin(LCD_WR_RS_GPIO_Port, LCD_WR_RS_Pin, GPIO_PIN_SET)

#define ST7789_SWRESET 0x01U
#define ST7789_SLPOUT  0x11U
#define ST7789_NORON   0x13U
#define ST7789_INVOFF  0x20U
#define ST7789_INVON   0x21U
#define ST7789_DISPOFF 0x28U
#define ST7789_DISPON  0x29U
#define ST7789_CASET   0x2AU
#define ST7789_RASET   0x2BU
#define ST7789_RAMWR   0x2CU
#define ST7789_MADCTL  0x36U
#define ST7789_COLMOD  0x3AU

#define ST7789_PORCTRL 0xB2U
#define ST7789_GCTRL   0xB7U
#define ST7789_VCOMS   0xBBU
#define ST7789_LCMCTRL 0xC0U
#define ST7789_VDVVRHEN 0xC2U
#define ST7789_VRHS    0xC3U
#define ST7789_VDVS    0xC4U
#define ST7789_FRCTRL2 0xC6U
#define ST7789_PWCTRL1 0xD0U
#define ST7789_PVGAMCTRL 0xE0U
#define ST7789_NVGAMCTRL 0xE1U

static int32_t st7789_tx(const uint8_t *data, uint32_t len, uint8_t data_mode)
{
    if ((data == NULL) || (len == 0U))
    {
        return 0;
    }

    LCD_CS_LOW();
    if (data_mode != 0U)
        LCD_DC_DATA();
    else
        LCD_DC_CMD();

    while (len > 0U)
    {
        const uint16_t chunk = (len > 32768U) ? 32768U : (uint16_t)len;
        if (HAL_SPI_Transmit(&hspi4, (uint8_t *)data, chunk, 1000U) != HAL_OK)
        {
            LCD_CS_HIGH();
            return -1;
        }
        data += chunk;
        len -= chunk;
    }

    LCD_CS_HIGH();
    return 0;
}

static int32_t st7789_cmd(uint8_t cmd)
{
    return st7789_tx(&cmd, 1U, 0U);
}

static int32_t st7789_data(const uint8_t *data, uint32_t len)
{
    return st7789_tx(data, len, 1U);
}

static int32_t st7789_cmd_data(uint8_t cmd, const uint8_t *data, uint32_t len)
{
    if (st7789_cmd(cmd) != 0)
        return -1;
    if ((data != NULL) && (len != 0U))
        return st7789_data(data, len);
    return 0;
}

static int32_t st7789_set_window(uint16_t x,
                                 uint16_t y,
                                 uint16_t w,
                                 uint16_t h)
{
    if ((w == 0U) || (h == 0U) || (x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT))
        return -1;

    if ((uint32_t)x + w > ST7789_WIDTH)
        w = (uint16_t)(ST7789_WIDTH - x);
    if ((uint32_t)y + h > ST7789_HEIGHT)
        h = (uint16_t)(ST7789_HEIGHT - y);

    const uint16_t x0 = (uint16_t)(x + ST7789_XSTART);
    const uint16_t x1 = (uint16_t)(x + w - 1U + ST7789_XSTART);
    const uint16_t y0 = (uint16_t)(y + ST7789_YSTART);
    const uint16_t y1 = (uint16_t)(y + h - 1U + ST7789_YSTART);

    const uint8_t xa[4] = {
        (uint8_t)(x0 >> 8), (uint8_t)x0,
        (uint8_t)(x1 >> 8), (uint8_t)x1
    };
    const uint8_t ya[4] = {
        (uint8_t)(y0 >> 8), (uint8_t)y0,
        (uint8_t)(y1 >> 8), (uint8_t)y1
    };

    if (st7789_cmd_data(ST7789_CASET, xa, sizeof(xa)) != 0)
        return -1;
    if (st7789_cmd_data(ST7789_RASET, ya, sizeof(ya)) != 0)
        return -1;
    return st7789_cmd(ST7789_RAMWR);
}

int32_t ST7789_Init(void)
{
    uint8_t d[16];

    LCD_CS_HIGH();
    LCD_DC_DATA();
    HAL_Delay(10U);

    if (st7789_cmd(ST7789_SWRESET) != 0)
        return -1;
    HAL_Delay(150U);

    if (st7789_cmd(ST7789_SLPOUT) != 0)
        return -1;
    HAL_Delay(120U);

    d[0] = 0x55U; /* RGB565 */
    if (st7789_cmd_data(ST7789_COLMOD, d, 1U) != 0)
        return -1;
    HAL_Delay(10U);

    d[0] = ST7789_MADCTL_VALUE;
    if (st7789_cmd_data(ST7789_MADCTL, d, 1U) != 0)
        return -1;

    /* Common ST7789V 1.14-inch IPS initialization sequence. */
    d[0] = 0x0CU; d[1] = 0x0CU; d[2] = 0x00U; d[3] = 0x33U; d[4] = 0x33U;
    if (st7789_cmd_data(ST7789_PORCTRL, d, 5U) != 0)
        return -1;

    d[0] = 0x35U;
    if (st7789_cmd_data(ST7789_GCTRL, d, 1U) != 0)
        return -1;

    d[0] = 0x19U;
    if (st7789_cmd_data(ST7789_VCOMS, d, 1U) != 0)
        return -1;

    d[0] = 0x2CU;
    if (st7789_cmd_data(ST7789_LCMCTRL, d, 1U) != 0)
        return -1;

    d[0] = 0x01U;
    if (st7789_cmd_data(ST7789_VDVVRHEN, d, 1U) != 0)
        return -1;

    d[0] = 0x12U;
    if (st7789_cmd_data(ST7789_VRHS, d, 1U) != 0)
        return -1;

    d[0] = 0x20U;
    if (st7789_cmd_data(ST7789_VDVS, d, 1U) != 0)
        return -1;

    d[0] = 0x0FU;
    if (st7789_cmd_data(ST7789_FRCTRL2, d, 1U) != 0)
        return -1;

    d[0] = 0xA4U; d[1] = 0xA1U;
    if (st7789_cmd_data(ST7789_PWCTRL1, d, 2U) != 0)
        return -1;

    {
        const uint8_t gamma_pos[14] = {
            0xD0U, 0x04U, 0x0DU, 0x11U, 0x13U, 0x2BU, 0x3FU,
            0x54U, 0x4CU, 0x18U, 0x0DU, 0x0BU, 0x1FU, 0x23U
        };
        if (st7789_cmd_data(ST7789_PVGAMCTRL, gamma_pos, sizeof(gamma_pos)) != 0)
            return -1;
    }

    {
        const uint8_t gamma_neg[14] = {
            0xD0U, 0x04U, 0x0CU, 0x11U, 0x13U, 0x2CU, 0x3FU,
            0x44U, 0x51U, 0x2FU, 0x1FU, 0x1FU, 0x20U, 0x23U
        };
        if (st7789_cmd_data(ST7789_NVGAMCTRL, gamma_neg, sizeof(gamma_neg)) != 0)
            return -1;
    }

    /* The 1.14-inch IPS family normally uses display inversion. */
    if (st7789_cmd(ST7789_INVON) != 0)
        return -1;

    if (st7789_cmd(ST7789_NORON) != 0)
        return -1;
    HAL_Delay(10U);

    if (st7789_cmd(ST7789_DISPON) != 0)
        return -1;
    HAL_Delay(120U);

    return 0;
}

void ST7789_FillRect(uint16_t x,
                     uint16_t y,
                     uint16_t w,
                     uint16_t h,
                     uint16_t color)
{
    if (st7789_set_window(x, y, w, h) != 0)
        return;

    uint8_t block[256];
    const uint8_t hi = (uint8_t)(color >> 8);
    const uint8_t lo = (uint8_t)color;
    for (uint32_t i = 0U; i < sizeof(block); i += 2U)
    {
        block[i] = hi;
        block[i + 1U] = lo;
    }

    uint32_t bytes = (uint32_t)w * (uint32_t)h * 2U;
    LCD_CS_LOW();
    LCD_DC_DATA();
    while (bytes > 0U)
    {
        const uint16_t n = (bytes > sizeof(block)) ? (uint16_t)sizeof(block) : (uint16_t)bytes;
        if (HAL_SPI_Transmit(&hspi4, block, n, 1000U) != HAL_OK)
            break;
        bytes -= n;
    }
    LCD_CS_HIGH();
}

void ST7789_FillScreen(uint16_t color)
{
    ST7789_FillRect(0U, 0U, ST7789_WIDTH, ST7789_HEIGHT, color);
}

void ST7789_DrawRGB565(uint16_t x,
                       uint16_t y,
                       const uint8_t *rgb565_be,
                       uint16_t w,
                       uint16_t h)
{
    if ((rgb565_be == NULL) || (w == 0U) || (h == 0U))
        return;

    if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT))
        return;

    if ((uint32_t)x + w > ST7789_WIDTH)
        w = (uint16_t)(ST7789_WIDTH - x);
    if ((uint32_t)y + h > ST7789_HEIGHT)
        h = (uint16_t)(ST7789_HEIGHT - y);

    if (st7789_set_window(x, y, w, h) != 0)
        return;

    (void)st7789_tx(rgb565_be, (uint32_t)w * (uint32_t)h * 2U, 1U);
}
