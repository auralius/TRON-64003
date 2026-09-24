/*
 * Native Waveshare 1.8" LCD for micro:bit V2
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
 * Rendering path:
 *   - nRF52833 SPIM2, SPI mode 0
 *   - 1 Mbit/s during cold-start/controller initialization
 *   - 8 Mbit/s for normal framebuffer/overlay rendering
 *   - EasyDMA for command/data transfers
 *   - one 256-byte RGB565 line/chunk buffer (128 pixels)
 *   - direct panel rendering; the Waveshare 23LC1024 framebuffer is unused
 *
 * Waveshare's MakeCode driver uses hardware SPI and requests 18 MHz. SPIM2 is
 * intentionally run at its supported 8 Mbit/s ceiling here for a conservative
 * first native/RTOS implementation. This is still far faster and more stable
 * than the previous GPIO bit-banged transport.
 */

#include <tk/tkernel.h>
#include <stdint.h>
#include "cute_lcd.h"

/* ------------------------------------------------------------------------- */
/* nRF52833 GPIO                                                              */
/* ------------------------------------------------------------------------- */
#define GPIO0_BASE      0x50000000u
#define GPIO1_BASE      0x50000300u
#define LCD_GPIO_OUTSET     0x508u
#define LCD_GPIO_OUTCLR     0x50Cu
#define LCD_GPIO_DIRSET     0x518u

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
    REG32(GPIO0_BASE + LCD_GPIO_OUTSET) = (1u << pin);
}

static inline void p0_lo(uint32_t pin)
{
    REG32(GPIO0_BASE + LCD_GPIO_OUTCLR) = (1u << pin);
}

static inline void p1_hi(uint32_t pin)
{
    REG32(GPIO1_BASE + LCD_GPIO_OUTSET) = (1u << pin);
}

static inline void p1_lo(uint32_t pin)
{
    REG32(GPIO1_BASE + LCD_GPIO_OUTCLR) = (1u << pin);
}

static inline void lcd_cs_hi(void) { p1_hi(PIN_LCDCS_P1); }
static inline void lcd_cs_lo(void) { p1_lo(PIN_LCDCS_P1); }
static inline void lcd_dc_hi(void) { p0_hi(PIN_DC_P0); }
static inline void lcd_dc_lo(void) { p0_lo(PIN_DC_P0); }
static inline void lcd_rst_hi(void) { p0_hi(PIN_RST_P0); }
static inline void lcd_rst_lo(void) { p0_lo(PIN_RST_P0); }

/* ------------------------------------------------------------------------- */
/* nRF52833 SPIM2 + EasyDMA                                                  */
/* ------------------------------------------------------------------------- */
#define SPIM2_BASE              0x40023000u
#define SPIM_TASKS_START        0x010u
#define SPIM_EVENTS_END         0x118u
#define SPIM_ENABLE             0x500u
#define SPIM_PSEL_SCK           0x508u
#define SPIM_PSEL_MOSI          0x50Cu
#define SPIM_PSEL_MISO          0x510u
#define SPIM_FREQUENCY          0x524u
#define SPIM_RXD_PTR            0x534u
#define SPIM_RXD_MAXCNT         0x538u
#define SPIM_TXD_PTR            0x544u
#define SPIM_TXD_MAXCNT         0x548u
#define SPIM_CONFIG             0x554u
#define SPIM_ORC                0x5C0u

#define SPIM_ENABLE_DISABLED    0u
#define SPIM_ENABLE_ENABLED     7u
#define SPIM_FREQUENCY_1M       0x10000000u
#define SPIM_FREQUENCY_8M       0x80000000u
#define SPIM_PIN_DISCONNECTED   0xFFFFFFFFu

/* True USB cold-power start: let the LCD module supply settle while the
   ST7735 reset input is actively held low. */
#define LCD_COLD_POWER_SETTLE_MS 2000u
#define LCD_RESET_RELEASE_MS      120u

/* 128 RGB565 pixels. EasyDMA source memory must remain in RAM until END. */
static uint8_t g_line_buffer[256u] __attribute__((aligned(4)));

static void spim_init(void)
{
    /* PSEL registers are only writable while SPIM is disabled. */
    REG32(SPIM2_BASE + SPIM_ENABLE) = SPIM_ENABLE_DISABLED;

    /* P0 pins use plain pin numbers in PSEL (PORT bit = 0). */
    REG32(SPIM2_BASE + SPIM_PSEL_SCK) = PIN_SCK_P0;
    REG32(SPIM2_BASE + SPIM_PSEL_MOSI) = PIN_MOSI_P0;
    REG32(SPIM2_BASE + SPIM_PSEL_MISO) = SPIM_PIN_DISCONNECTED;

    /* Mode 0, MSB first: ORDER=0, CPHA=0, CPOL=0. */
    REG32(SPIM2_BASE + SPIM_CONFIG) = 0u;
    /* Keep cold-start register traffic conservative. Once the ST7735S is
       awake and DISPON has completed, cute_lcd_init() raises the bus to 8M. */
    REG32(SPIM2_BASE + SPIM_FREQUENCY) = SPIM_FREQUENCY_1M;
    REG32(SPIM2_BASE + SPIM_ORC) = 0xFFu;

    /* TX-only transfers. */
    REG32(SPIM2_BASE + SPIM_RXD_PTR) = 0u;
    REG32(SPIM2_BASE + SPIM_RXD_MAXCNT) = 0u;
    REG32(SPIM2_BASE + SPIM_EVENTS_END) = 0u;

    REG32(SPIM2_BASE + SPIM_ENABLE) = SPIM_ENABLE_ENABLED;
}

static void spim_set_frequency(uint32_t frequency)
{
    /* All public LCD operations are synchronous, so this is only called when
       no EasyDMA transfer is active. FREQUENCY may then be changed directly. */
    REG32(SPIM2_BASE + SPIM_FREQUENCY) = frequency;
    __asm volatile("dmb" ::: "memory");
}

static void spim_write(const uint8_t *data, uint32_t count)
{
    if (data == 0 || count == 0u) {
        return;
    }

    REG32(SPIM2_BASE + SPIM_EVENTS_END) = 0u;
    REG32(SPIM2_BASE + SPIM_TXD_PTR) = (uint32_t)(uintptr_t)data;
    REG32(SPIM2_BASE + SPIM_TXD_MAXCNT) = count;

    /* Ensure RAM writes are visible to EasyDMA before START. */
    __asm volatile("dmb" ::: "memory");
    REG32(SPIM2_BASE + SPIM_TASKS_START) = 1u;

    /* Synchronous API for now; SPIM continues autonomously if this task is
       preempted while the transfer is in progress. */
    while (REG32(SPIM2_BASE + SPIM_EVENTS_END) == 0u) {
        /* intentionally empty */
    }

    __asm volatile("dmb" ::: "memory");
}

/* ------------------------------------------------------------------------- */
/* ST7735 transport                                                          */
/* ------------------------------------------------------------------------- */
static void lcd_cmd(uint8_t cmd)
{
    lcd_dc_lo();
    lcd_cs_lo();
    spim_write(&cmd, 1u);
    lcd_cs_hi();
}

static void lcd_data8(uint8_t data)
{
    lcd_dc_hi();
    lcd_cs_lo();
    spim_write(&data, 1u);
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
    REG32(GPIO0_BASE + LCD_GPIO_DIRSET) =
        (1u << PIN_BL_P0) |
        (1u << PIN_RAMCS_P0) |
        (1u << PIN_RST_P0) |
        (1u << PIN_DC_P0) |
        (1u << PIN_MOSI_P0) |
        (1u << PIN_SCK_P0);

    REG32(GPIO1_BASE + LCD_GPIO_DIRSET) = (1u << PIN_LCDCS_P1);

    /* Safe power-on state. Keep the panel dark until black GRAM is ready. */
    lcd_cs_hi();
    p0_hi(PIN_RAMCS_P0);  /* Deselect onboard 23LC1024 permanently. */
    lcd_dc_hi();
    lcd_rst_lo();      /* Hold controller in reset throughout cold-power settle. */
    p0_lo(PIN_SCK_P0);    /* SPI mode 0 idle state before SPIM takes the pin. */
    p0_lo(PIN_MOSI_P0);
    p0_lo(PIN_BL_P0);     /* Backlight OFF during controller bring-up. */
}

static void lcd_hw_reset(void)
{
    /* On a true USB power cycle the LCD module and MCU ramp together.  The
       μT-Kernel firmware reaches this driver much earlier than MakeCode, so
       keep the ST7735 reset asserted until the module supply has settled. */
    lcd_rst_lo();
    tk_dly_tsk(LCD_COLD_POWER_SETTLE_MS);
    lcd_rst_hi();
    tk_dly_tsk(LCD_RESET_RELEASE_MS);
}

/* This register sequence follows Waveshare's WSLCD1in8/PXT driver. */
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

static void lcd_begin_pixels(void)
{
    lcd_dc_hi();
    lcd_cs_lo();
}

static void lcd_end_pixels(void)
{
    lcd_cs_hi();
}

/* Stream count RGB565 pixels using the shared 128-pixel EasyDMA buffer. */
static void lcd_stream_solid_pixels(uint16_t rgb565, uint32_t count)
{
    const uint8_t hi = (uint8_t)(rgb565 >> 8);
    const uint8_t lo = (uint8_t)(rgb565 & 0xffu);

    for (uint32_t i = 0; i < 128u; ++i) {
        g_line_buffer[2u * i] = hi;
        g_line_buffer[2u * i + 1u] = lo;
    }

    while (count != 0u) {
        const uint32_t pixels = (count > 128u) ? 128u : count;
        spim_write(g_line_buffer, pixels * 2u);
        count -= pixels;
    }
}

/* Internal full-panel fill used during startup before DISPON/backlight. */
static void lcd_fill_screen_raw(uint16_t rgb565)
{
    lcd_set_window_xyxy(0u, 0u, CUTE_LCD_WIDTH - 1u, CUTE_LCD_HEIGHT - 1u);
    lcd_begin_pixels();
    lcd_stream_solid_pixels(
        rgb565,
        (uint32_t)CUTE_LCD_WIDTH * (uint32_t)CUTE_LCD_HEIGHT);
    lcd_end_pixels();
}

void cute_lcd_init(void)
{
    lcd_gpio_init();
    spim_init();
    lcd_hw_reset();
    lcd_init_regs();

    /* Match the useful behavior of Waveshare's reference: keep the display
       off while waking the controller and preparing GRAM, then reveal a
       known black frame. This avoids exposing white/undefined cold-start RAM. */
    lcd_cmd(0x11); /* Sleep out */
    tk_dly_tsk(120);

    lcd_fill_screen_raw(0x0000u);

    lcd_cmd(0x29); /* Display on */
    tk_dly_tsk(20);

    /* Initialization/register traffic above deliberately ran at 1 Mbit/s.
       From here onward use the nRF52833 SPIM2 maximum for image and box
       rendering. */
    spim_set_frequency(SPIM_FREQUENCY_8M);

    p0_hi(PIN_BL_P0); /* Backlight ON only after the first black frame exists. */
}

void cute_lcd_clear(uint16_t rgb565)
{
    lcd_fill_screen_raw(rgb565);
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
    if (!gray) {
        return;
    }

    /* Center the exact 128x128 Cute-YOLO image in the 160x128 panel. */
    const uint16_t x0 = 16u;
    const uint16_t y0 = 0u;

    lcd_set_window_xyxy(x0, y0, x0 + 127u, y0 + 127u);
    lcd_begin_pixels();

    for (uint32_t y = 0u; y < 128u; ++y) {
        const uint8_t *src = gray + y * 128u;

        for (uint32_t x = 0u; x < 128u; ++x) {
            const uint16_t c = gray8_to_rgb565(src[x]);
            g_line_buffer[2u * x] = (uint8_t)(c >> 8);
            g_line_buffer[2u * x + 1u] = (uint8_t)(c & 0xffu);
        }

        spim_write(g_line_buffer, sizeof(g_line_buffer));
    }

    lcd_end_pixels();

    /* Waveshare's reference LCD_Display() reissues DISPON after a complete
       framebuffer transfer. Keep the bus at the normal 8 Mbit/s here so this
       experiment changes only the post-frame controller command behavior. */
    lcd_cmd(0x29);
}

void cute_lcd_draw_hline(
    uint16_t x, uint16_t y, uint16_t w, uint16_t rgb565)
{
    if (w == 0u || x >= CUTE_LCD_WIDTH || y >= CUTE_LCD_HEIGHT) {
        return;
    }
    if (x + w > CUTE_LCD_WIDTH) {
        w = CUTE_LCD_WIDTH - x;
    }

    lcd_set_window_xyxy(x, y, x + w - 1u, y);
    lcd_begin_pixels();
    lcd_stream_solid_pixels(rgb565, w);
    lcd_end_pixels();
}

void cute_lcd_draw_vline(
    uint16_t x, uint16_t y, uint16_t h, uint16_t rgb565)
{
    if (h == 0u || x >= CUTE_LCD_WIDTH || y >= CUTE_LCD_HEIGHT) {
        return;
    }
    if (y + h > CUTE_LCD_HEIGHT) {
        h = CUTE_LCD_HEIGHT - y;
    }

    lcd_set_window_xyxy(x, y, x, y + h - 1u);
    lcd_begin_pixels();
    lcd_stream_solid_pixels(rgb565, h);
    lcd_end_pixels();
}

void cute_lcd_draw_rect(
    uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t rgb565)
{
    if (w == 0u || h == 0u) {
        return;
    }

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
