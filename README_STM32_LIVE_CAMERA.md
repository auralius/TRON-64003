# STM32H743 CUTE-YOLO live-camera demo

[Project overview](README.md) · [Micro:bit setup and testing](README_MICROBIT.md)

This guide explains how to operate the **WeAct Studio MiniSTM32H7xx (STM32H743VIT6)** CUTE-YOLO demonstration. The board captures an image with its **OV5640 V1** camera, runs the Noodle INT8 detector under **μT-Kernel 3.0**, and shows the result on a **135 × 240 portrait ST7789** LCD. The model is a separate CUTE v2 `.cute` package in internal flash.

The micro:bit demonstration sends images from a computer. **This STM32 demonstration captures its own live camera frames.** A computer is needed only when changing the model over USB serial or collecting diagnostics.

## What the audience sees

1. **Power on or reset:** The board starts μT-Kernel automatically. K1 is **not** needed to start the application. Once the OV5640 initializes, the LCD shows a live portrait camera preview; the PE3 LED provides a heartbeat.
2. **Aim the camera:** Put a face in view when a face model is installed, or present the object class named by the active `.cute` package. The OV5640 supplies 160 × 120 RGB565 frames. The preview is rotated for the portrait LCD; inference uses the camera's landscape coordinates.
3. **Press and release K1 once:** The control task waits for the **next completed camera frame**, copies and freezes it, and requests inference. Camera DMA continues in the background; the frozen copy stays unchanged while inference runs. Give the board time to finish.
4. **Read the result:** The board crops a centered 120 × 120 square, converts it to 128 × 128 grayscale, runs CUTE-YOLO, then decodes and filters detections. The LCD displays the frozen result, boxes when detections exist, and `T=...ms` above the image. If there are no detections, it displays the frozen frame without boxes.
5. **Press and release K1 again:** Once inference has finished, K1 returns the display to live preview. Repeat from step 2 for another scene. Pressing K1 during an active inference does not start a second inference.

`T=...ms` covers **preprocessing, Noodle inference, and decode/NMS**. It excludes camera capture and LCD rendering, so it is not a full button-to-display latency measurement. In an earlier 480 MHz H6/M1 hardware run, the total detector path was around **804 ms**; timing depends on the installed model and build.

## Hardware and firmware used here

| Component | Demonstration configuration |
| --- | --- |
| Board | WeAct Studio MiniSTM32H7xx, STM32H743VIT6 at 480 MHz |
| Camera | OV5640 V1, 160 × 120 RGB565; the firmware is locked to this sensor |
| Display | ST7789, 135 × 240 portrait; 120 × 160 centered preview |
| Input | K1 button: freeze/infer in LIVE; return to LIVE in FROZEN |
| Connection | USB CDC serial, commonly `/dev/ttyACM0` on Linux, 115200 baud at the host |
| Model storage | CUTE v2 package in the reserved last 128 KiB of internal flash, beginning at `0x081E0000` |
| Serial upload limit | 32 KiB staging buffer reported by `MCAP`; the selected `.cute` must fit |

The firmware uses μT-Kernel tasks for heartbeat, camera control, LCD rendering, inference, and serial communication. The LCD task owns all ST7789 drawing; K1 and camera frame handoff live in the control task; a dedicated inference task runs preprocessing and the detector. The LCD rotation does not change inference coordinates or the stored model.

## Changing the `.cute` model over USB serial

The STM32 uses the **same CUTE v2 serial upload sequence** as the micro:bit tool: `MCAP` checks compatibility, `MBEG` starts an upload, `MDAT` transfers chunks, `MEND` validates and activates the package, and `MIN2` confirms the installed H/M configuration. You can use **Deploy via Serial** in `cute_yolo_tester.py` or the standalone uploader packaged with that tool. The STM32's single flash model slot is reported as slot **A**. A known H6/M1 face package is **26,128 bytes**, within the STM32 serial limit.

Typical command from a directory containing the tester's `tools/` folder:

```bash
python3 tools/upload_cute_serial.py /path/to/face_adapted_wider_tuned.cute \
  --port /dev/ttyACM0 --chunk 128
```

Select the actual port on your computer. Close serial monitors and other programs holding it before upload, and perform the upload while the board is not running inference. Wait for `ACTIVE slot: A` and the final `Verified: ... H6/M1 ...` response. **Then press the board's hardware RESET button once (not K1)**: this firmware probes the flash package and initializes its inference runtime at boot. The upload replaces the current model in flash; it does not rebuild the STM32 firmware. If you use the tester GUI, use its **Deploy via Serial** tab for this board. Its **Test on Device** tab sends a computer image using the micro:bit `CUTE` command and is **not the STM32 live-camera control interface**.

The serial `MIN2` response in this firmware is prepared during a successful upload. After a power cycle, **Read active model** in the tester may show an unknown model until another upload. Test with K1 instead; `NO RUNTIME` on the LCD indicates that the inference runtime is not ready.

## Operating notes

- **No live preview:** Check camera seating and power, then reset. The firmware probes the selected OV5640; if identification fails, the LCD may say `OV5640 FAIL` and PE3 blinks rapidly (about every 120 ms). A DCMI start failure uses a different rapid blink (about every 350 ms). The working camera is the original OV5640 V1; the M12 OV5640 has strong distortion and the tested OV2640 module did not initialize correctly in this setup.
- **No model result:** An empty or invalid model slot can show `NO RUNTIME`. Upload a compatible CUTE v2 package and check the uploader's final verification. `INFER ERR`, `DECODE ERR`, and `SEM ERROR` identify distinct runtime failures on the LCD.
- **No boxes:** Aim at the class of the installed model, improve illumination, and press K1 again from LIVE. A frozen image with `T=...ms` but no boxes is still a completed inference with zero accepted detections.
- **Serial port missing or busy:** Reconnect the USB CDC cable, use the newly enumerated port, and close any other serial application. The LED and live preview can run without a host computer after a model has been installed.

## Source and scope

The attached project is `08-DCMI2LCD_CubeIDE/`. The live-camera and K1 state machine are in `Src/main.c`, the μT-Kernel tasks and model-upload parser in `mtk3_src/usermain.c`, and the camera and display drivers in `Drivers/BSP/Camera/` and `Drivers/BSP/ST7789/`. `README_MIGRATION.md` describes the ST7789 portrait mapping; `README_OV5640_LOCK.md` describes the OV5640 startup path.
