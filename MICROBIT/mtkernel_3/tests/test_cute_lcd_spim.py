from pathlib import Path
import re

SRC = Path(__file__).resolve().parents[1] / 'app_sample' / 'cute_lcd.c'
text = SRC.read_text(encoding='utf-8')


def test_hardware_spim_easydma_is_used():
    assert 'SPIM2_BASE' in text
    assert 'SPIM_TXD_PTR' in text
    assert 'SPIM_TXD_MAXCNT' in text
    assert 'SPIM_FREQUENCY_8M' in text
    assert 'SPIM_ENABLE_ENABLED' in text


def test_gpio_bitbang_loop_is_gone():
    assert 'for (int bit = 7; bit >= 0; --bit)' not in text
    assert 'mosi_hi()' not in text
    assert 'mosi_lo()' not in text


def test_gray_frame_uses_256_byte_line_buffer():
    assert re.search(r'static\s+uint8_t\s+g_line_buffer\s*\[\s*256u?\s*\]', text)
    assert 'spim_write(g_line_buffer, sizeof(g_line_buffer))' in text


def test_display_is_enabled_only_after_black_gram_prefill():
    init = text[text.index('void cute_lcd_init(void)'):]
    sleep = init.index('lcd_cmd(0x11)')
    black = init.index('lcd_fill_screen_raw(0x0000u)')
    display_on = init.index('lcd_cmd(0x29)')
    backlight_on = init.index('p0_hi(PIN_BL_P0)')
    assert sleep < black < display_on < backlight_on


def test_backlight_is_off_during_initialization():
    gpio = text[text.index('static void lcd_gpio_init(void)'):text.index('static void lcd_hw_reset(void)')]
    assert 'p0_lo(PIN_BL_P0)' in gpio


def test_cold_start_controller_phase_uses_1mhz():
    assert '#define SPIM_FREQUENCY_1M' in text
    init_start = text.index('static void spim_init(void)')
    init_end = text.index('static void spim_write(', init_start)
    spim_init = text[init_start:init_end]
    assert 'SPIM_FREQUENCY_1M' in spim_init
    assert 'SPIM_FREQUENCY_8M' not in spim_init


def test_switch_to_8mhz_only_after_display_init_finishes():
    init = text[text.index('void cute_lcd_init(void)'):]
    display_on = init.index('lcd_cmd(0x29)')
    fast_switch = init.index('spim_set_frequency(SPIM_FREQUENCY_8M)')
    backlight_on = init.index('p0_hi(PIN_BL_P0)')
    assert display_on < fast_switch < backlight_on


def test_full_gray_frame_reissues_display_on_after_pixel_transfer():
    start = text.index('void cute_lcd_draw_gray128(const uint8_t *gray)')
    end = text.index('void cute_lcd_draw_hline(', start)
    draw = text[start:end]
    end_pixels = draw.index('lcd_end_pixels();')
    display_on = draw.index('lcd_cmd(0x29)')
    assert end_pixels < display_on


def test_true_cold_boot_holds_panel_reset_low_during_power_settle():
    # A real USB power cycle powers the LCD and MCU at the same time.  Keep the
    # ST7735 reset asserted while the module supply settles; only then release
    # reset and begin controller commands.
    gpio_start = text.index('static void lcd_gpio_init(void)')
    reset_start = text.index('static void lcd_hw_reset(void)')
    gpio = text[gpio_start:reset_start]
    assert 'lcd_rst_lo();' in gpio
    assert 'lcd_rst_hi();' not in gpio

    reset_end = text.index('static void lcd_init_regs(void)', reset_start)
    reset = text[reset_start:reset_end]
    assert '#define LCD_COLD_POWER_SETTLE_MS' in text
    assert '#define LCD_RESET_RELEASE_MS' in text
    hold = reset.index('tk_dly_tsk(LCD_COLD_POWER_SETTLE_MS)')
    release = reset.index('lcd_rst_hi();')
    release_wait = reset.index('tk_dly_tsk(LCD_RESET_RELEASE_MS)')
    assert hold < release < release_wait

    settle_match = re.search(r'#define\s+LCD_COLD_POWER_SETTLE_MS\s+(\d+)u', text)
    release_match = re.search(r'#define\s+LCD_RESET_RELEASE_MS\s+(\d+)u', text)
    assert settle_match and int(settle_match.group(1)) >= 6000
    assert release_match and int(release_match.group(1)) >= 120
