/*
 * Minimal native Waveshare 1.8" LCD for micro:bit driver
 *
 * Panel/controller:
 *   ST7735S / ST7735R-compatible
 *   160x128 logical landscape
 *   RGB565
 *
 * Waveshare micro:bit edge pins:
 *   P13 SCK      -> nRF52833 P0.17
 *   P15 MOSI     -> nRF52833 P0.13
 *   P16 LCD_CS   -> nRF52833 P1.02
 *   P2  RAM_CS   -> nRF52833 P0.04   (held high; onboard 23LC1024 unused)
 *   P12 DC       -> nRF52833 P0.12
 *   P8  RESET    -> nRF52833 P0.10
 *   P1  BL       -> nRF52833 P0.03
 *
 * This first port deliberately bit-bangs SPI mode 0.  It avoids adding a
 * second RTOS device driver while we validate the display.  The official
 * Waveshare library also uses a modest 1 MHz SPI clock, so this is adequate
 * for the first serial->micro:bit->LCD test.
 */

#include <tk/tkernel.h>
#include <stdint.h>
#include "cute_lcd.h"

/* nRF52833 GPIO register blocks. */
#define GPIO0_BASE      0x50000000u
#define GPIO1_BASE      0x50000300u
#define GPIO_OUTSET     0x508u
#define GPIO_OUTCLR     0x50Cu
#define GPIO_DIRSET     0x518u

#define REG32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))

/* Edge connector -> nRF52833 GPIO mapping. */
#define PIN_BL_P0       3u
#define PIN_RAMCS_P0    4u
#define PIN_RST_P0      10u
#define PIN_DC_P0       12u
#define PIN_MOSI_P0     13u
#define PIN_SCK_P0      17u

#define PIN_LCDCS_P1    2u

static inline void p0_hi(uint32_t pin)
{
    REG32(GPIO0_BASE + GPIO_OUTSET) = (1u << pin);
}

static inline void p0_lo(uint32_t pin)
{
    REG32(GPIO0_BASE + GPIO_OUTCLR) = (1u << pin);
}

static inline void p1_hi(uint32_t pin)
{
    REG32(GPIO1_BASE + GPIO_OUTSET) = (1u << pin);
}

static inline void p1_lo(uint32_t pin)
{
    REG32(GPIO1_BASE + GPIO_OUTCLR) = (1u << pin);
}

static inline void lcd_cs_hi(void) { p1_hi(PIN_LCDCS_P1); }
static inline void lcd_cs_lo(void) { p1_lo(PIN_LCDCS_P1); }
static inline void lcd_dc_hi(void) { p0_hi(PIN_DC_P0); }
static inline void lcd_dc_lo(void) { p0_lo(PIN_DC_P0); }
static inline void lcd_rst_hi(void) { p0_hi(PIN_RST_P0); }
static inline void lcd_rst_lo(void) { p0_lo(PIN_RST_P0); }
static inline void sck_hi(void) { p0_hi(PIN_SCK_P0); }
static inline void sck_lo(void) { p0_lo(PIN_SCK_P0); }
static inline void mosi_hi(void) { p0_hi(PIN_MOSI_P0); }
static inline void mosi_lo(void) { p0_lo(PIN_MOSI_P0); }

/* SPI mode 0: idle low; MSB first. */
static inline void spi_write8(uint8_t v)
{
    for (int bit = 7; bit >= 0; --bit) {
        sck_lo();

        if (v & (1u << bit)) {
            mosi_hi();
        } else {
            mosi_lo();
        }

        sck_hi();
    }

    sck_lo();
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_dc_lo();
    lcd_cs_lo();
    spi_write8(cmd);
    lcd_cs_hi();
}

static void lcd_data8(uint8_t data)
{
    lcd_dc_hi();
    lcd_cs_lo();
    spi_write8(data);
    lcd_cs_hi();
}

/* Waveshare's logical display window has controller-RAM offsets +1,+2. */
static void lcd_set_window_xyxy(
    uint16_t x0, uint16_t y0,
    uint16_t x1, uint16_t y1)
{
    lcd_cmd(0x2A); /* CASET */
    lcd_data8((uint8_t)(((x0 + 1u) >> 8) & 0xffu));
    lcd_data8((uint8_t)((x0 + 1u) & 0xffu));
    lcd_data8((uint8_t)(((x1 + 1u) >> 8) & 0xffu));
    lcd_data8((uint8_t)((x1 + 1u) & 0xffu));

    lcd_cmd(0x2B); /* RASET */
    lcd_data8((uint8_t)(((y0 + 2u) >> 8) & 0xffu));
    lcd_data8((uint8_t)((y0 + 2u) & 0xffu));
    lcd_data8((uint8_t)(((y1 + 2u) >> 8) & 0xffu));
    lcd_data8((uint8_t)((y1 + 2u) & 0xffu));

    lcd_cmd(0x2C); /* RAMWR */
}

static void lcd_gpio_init(void)
{
    REG32(GPIO0_BASE + GPIO_DIRSET) =
        (1u << PIN_BL_P0) |
        (1u << PIN_RAMCS_P0) |
        (1u << PIN_RST_P0) |
        (1u << PIN_DC_P0) |
        (1u << PIN_MOSI_P0) |
        (1u << PIN_SCK_P0);

    REG32(GPIO1_BASE + GPIO_DIRSET) =
        (1u << PIN_LCDCS_P1);

    /* Safe idle state. */
    lcd_cs_hi();
    p0_hi(PIN_RAMCS_P0);  /* Always deselect onboard 23LC1024 for now. */
    lcd_dc_hi();
    lcd_rst_hi();
    sck_lo();
    mosi_lo();

    /* Full backlight for the first test. */
    p0_hi(PIN_BL_P0);
}

static void lcd_hw_reset(void)
{
    lcd_rst_hi();
    tk_dly_tsk(100);
    lcd_rst_lo();
    tk_dly_tsk(100);
    lcd_rst_hi();
    tk_dly_tsk(100);
}

/* This register sequence follows Waveshare's own WSLCD1in8 driver. */
static void lcd_init_regs(void)
{
    lcd_cmd(0xB1);
    lcd_data8(0x01); lcd_data8(0x2C); lcd_data8(0x2D);

    lcd_cmd(0xB2);
    lcd_data8(0x01); lcd_data8(0x2C); lcd_data8(0x2D);

    lcd_cmd(0xB3);
    lcd_data8(0x01); lcd_data8(0x2C); lcd_data8(0x2D);
    lcd_data8(0x01); lcd_data8(0x2C); lcd_data8(0x2D);

    lcd_cmd(0xB4);
    lcd_data8(0x07);

    lcd_cmd(0xC0);
    lcd_data8(0xA2); lcd_data8(0x02); lcd_data8(0x84);

    lcd_cmd(0xC1);
    lcd_data8(0xC5);

    lcd_cmd(0xC2);
    lcd_data8(0x0A); lcd_data8(0x00);

    lcd_cmd(0xC3);
    lcd_data8(0x8A); lcd_data8(0x2A);

    lcd_cmd(0xC4);
    lcd_data8(0x8A); lcd_data8(0xEE);

    lcd_cmd(0xC5);
    lcd_data8(0x0E);

    lcd_cmd(0xE0);
    lcd_data8(0x0F); lcd_data8(0x1A); lcd_data8(0x0F); lcd_data8(0x18);
    lcd_data8(0x2F); lcd_data8(0x28); lcd_data8(0x20); lcd_data8(0x22);
    lcd_data8(0x1F); lcd_data8(0x1B); lcd_data8(0x23); lcd_data8(0x37);
    lcd_data8(0x00); lcd_data8(0x07); lcd_data8(0x02); lcd_data8(0x10);

    lcd_cmd(0xE1);
    lcd_data8(0x0F); lcd_data8(0x1B); lcd_data8(0x0F); lcd_data8(0x17);
    lcd_data8(0x33); lcd_data8(0x2C); lcd_data8(0x29); lcd_data8(0x2E);
    lcd_data8(0x30); lcd_data8(0x30); lcd_data8(0x39); lcd_data8(0x3F);
    lcd_data8(0x00); lcd_data8(0x07); lcd_data8(0x03); lcd_data8(0x10);

    lcd_cmd(0xF0);
    lcd_data8(0x01);

    lcd_cmd(0xF6);
    lcd_data8(0x00);

    lcd_cmd(0x3A); /* RGB565 / 65K */
    lcd_data8(0x05);

    lcd_cmd(0x36); /* Landscape orientation used by Waveshare. */
    lcd_data8(0xA0);
}

void cute_lcd_init(void)
{
    lcd_gpio_init();
    lcd_hw_reset();
    lcd_init_regs();

    lcd_cmd(0x11); /* Sleep out */
    tk_dly_tsk(120);

    lcd_cmd(0x29); /* Display on */
    tk_dly_tsk(20);
}

static void lcd_begin_pixels(void)
{
    lcd_dc_hi();
    lcd_cs_lo();
}

static void lcd_end_pixels(void)
{
    lcd_cs_hi();
}

static inline void lcd_stream_rgb565(uint16_t c)
{
    spi_write8((uint8_t)(c >> 8));
    spi_write8((uint8_t)(c & 0xffu));
}

void cute_lcd_clear(uint16_t rgb565)
{
    lcd_set_window_xyxy(0, 0, CUTE_LCD_WIDTH - 1u, CUTE_LCD_HEIGHT - 1u);

    lcd_begin_pixels();

    for (uint32_t i = 0; i < CUTE_LCD_WIDTH * CUTE_LCD_HEIGHT; ++i) {
        lcd_stream_rgb565(rgb565);
    }

    lcd_end_pixels();
}

static inline uint16_t gray8_to_rgb565(uint8_t g)
{
    return (uint16_t)(
        ((uint16_t)(g >> 3) << 11) |
        ((uint16_t)(g >> 2) << 5)  |
        ((uint16_t)(g >> 3))
    );
}

void cute_lcd_draw_gray128(const uint8_t *gray)
{
    if (!gray) return;

    /* Center the exact 128x128 Cute-YOLO image in the 160x128 panel. */
    const uint16_t x0 = 16u;
    const uint16_t y0 = 0u;

    lcd_set_window_xyxy(x0, y0, x0 + 127u, y0 + 127u);

    lcd_begin_pixels();

    for (uint32_t i = 0; i < 128u * 128u; ++i) {
        lcd_stream_rgb565(gray8_to_rgb565(gray[i]));
    }

    lcd_end_pixels();
}

void cute_lcd_draw_hline(
    uint16_t x, uint16_t y, uint16_t w, uint16_t rgb565)
{
    if (w == 0u || x >= CUTE_LCD_WIDTH || y >= CUTE_LCD_HEIGHT) return;
    if (x + w > CUTE_LCD_WIDTH) w = CUTE_LCD_WIDTH - x;

    lcd_set_window_xyxy(x, y, x + w - 1u, y);

    lcd_begin_pixels();
    for (uint16_t i = 0; i < w; ++i) {
        lcd_stream_rgb565(rgb565);
    }
    lcd_end_pixels();
}

void cute_lcd_draw_vline(
    uint16_t x, uint16_t y, uint16_t h, uint16_t rgb565)
{
    if (h == 0u || x >= CUTE_LCD_WIDTH || y >= CUTE_LCD_HEIGHT) return;
    if (y + h > CUTE_LCD_HEIGHT) h = CUTE_LCD_HEIGHT - y;

    lcd_set_window_xyxy(x, y, x, y + h - 1u);

    lcd_begin_pixels();
    for (uint16_t i = 0; i < h; ++i) {
        lcd_stream_rgb565(rgb565);
    }
    lcd_end_pixels();
}

void cute_lcd_draw_rect(
    uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rgb565)
{
    if (w == 0u || h == 0u) return;

    cute_lcd_draw_hline(x, y, w, rgb565);

    if (h > 1u) {
        cute_lcd_draw_hline(x, y + h - 1u, w, rgb565);
    }

    if (h > 2u) {
        cute_lcd_draw_vline(x, y + 1u, h - 2u, rgb565);

        if (w > 1u) {
            cute_lcd_draw_vline(x + w - 1u, y + 1u, h - 2u, rgb565);
        }
    }
}
