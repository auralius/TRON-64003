# μT-Kernel background LCD init + heartbeat LED

This revision moves the long Waveshare LCD cold-start sequence out of `usermain()` and into the LCD RT task.

## What changed

- `cute_lcd_init()` is now called inside `lcd_task()`.
- RX/COMM no longer waits for LCD readiness before opening `sera`.
- If the LCD is not yet ready, `CUTE` still receives the image, runs inference, and returns detections; only LCD drawing is skipped.
- Added a `led_task()` that blinks one pixel of the built-in 5x5 LED matrix:
  - fast blink while LCD is still cold-starting
  - slow heartbeat after the LCD becomes ready

## Built-in status LED mapping

This uses one matrix pixel directly:

- Row 1: `P0.21`
- Col 1: `P0.28`

A matrix LED is lit by driving the row high and the column low.

## Main code file changed

- `app_sample/app_main.c`

## Notes

- `cute_lcd.c` is unchanged in this revision.
- Display timing reported by `STAT`/`RTIM` is `0 ms` for image/box drawing if a transaction happened before the LCD became ready.
- This is intended to demonstrate a more meaningful μT-Kernel use case: other tasks continue to run while the LCD spends several seconds in cold-power settle.
