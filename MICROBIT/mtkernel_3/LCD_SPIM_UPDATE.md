# Waveshare 1.8-inch LCD SPIM update

This revision replaces the original GPIO bit-banged LCD transport with the nRF52833 hardware SPIM2 peripheral and EasyDMA.

## What changed

- SPI mode 0 on P13/SCK (P0.17) and P15/MOSI (P0.13).
- SPIM2 runs at **1 Mbit/s during cold-start/controller initialization**, then switches to **8 Mbit/s after `DISPON`** for normal image/box rendering.
- The onboard 23LC1024 SRAM remains deselected and unused.
- `cute_lcd_draw_gray128()` converts one 128-pixel row at a time into a 256-byte RGB565 RAM buffer and sends it with EasyDMA.
- Solid clears and box lines reuse the same 256-byte buffer in chunks.
- Backlight stays off during controller startup.
- On a true USB cold-power start, LCD RESET is held low for **6 s** before release, then the driver waits another **120 ms** before sending ST7735 commands. This matches the observed requirement that the module must be allowed to settle before firmware initialization begins.
- After `SLPOUT`, the driver writes a black frame to GRAM before sending `DISPON`, then enables the backlight. This follows the useful ordering observed in Waveshare's PXT driver and avoids exposing undefined/white GRAM during cold startup.
- Public `cute_lcd.h` API is unchanged.

## Build

```bash
cd ~/works/utkernel_microbit/mtkernel_3
make -C build_make clean
make -C build_make -B -j2 all
arm-none-eabi-size build_make/mtkernel_3.elf
arm-none-eabi-objcopy -O ihex build_make/mtkernel_3.elf build_make/mtkernel_3.hex
cp build_make/mtkernel_3.hex /run/media/auralius/MICROBIT/
sync
```

## Hardware validation

1. Cold power-cycle the micro:bit + LCD.
2. Confirm the display goes black during boot rather than remaining white.
3. Send one image from the Studio without pressing RESET first.
4. Confirm the first image is visible.
5. Record `LCD image` from the existing `STAT/STOK` timing display and compare it with the previous ~760 ms bit-banged result.
6. Repeat several inferences and resets.

The earlier 1 MHz-init experiment showed that lowering SPI command speed alone did not remove the intermittent white screen. A delayed manual RESET after USB power-up did. This revision therefore targets the observed root cause: the firmware was reaching LCD initialization before the module had settled on a true cold start.

Perform at least 10 full USB unplug/replug cycles and confirm that the display initializes without a manual RESET. Keep the existing 1 MHz-init / 8 MHz-render split and post-frame `DISPON`; those paths are already validated by the ~50 ms image-render timing.
