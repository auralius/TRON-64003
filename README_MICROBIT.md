# CUTE-YOLO on micro:bit V2

[Project overview](README.md) · [STM32 live-camera guide](README_STM32_LIVE_CAMERA.md)

**Configurable Ultra-lightweight Tiny Embedded YOLO**  
**μT-Kernel 3.0 + Noodle INT8 · configurable H/M firmware**

CUTE-YOLO is a compact, single-class detector for constrained microcontrollers. This repository implements the detector on the BBC micro:bit V2 (nRF52833) using μT-Kernel 3.0 and the Noodle inference framework. The host sends a 128 × 128 grayscale frame over USB serial; the micro:bit executes the network, decodes detections, and returns boxes and confidence values. The attached LCD can also show the image and boxes.

The model is a separate `.cute` package stored in flash. **H** selects the number of hybrid blocks and **M** selects the number of hidden 1 × 1 head layers. The current CUTE v2 firmware is configurable within its supported H/M limits; **H6/M1** is the recommended model for this demonstration. The firmware does not implement arbitrary neural-network operators or geometries.

This micro:bit project is distinct from the camera-based [CUTE-YOLO ESP32 project](https://github.com/Student-Embedded-Control-and-AI-Fest/Cute-YOLO) and the STM32H743 live-camera demonstration.

## Quick start with a prepared micro:bit

The [micro:bit tester walkthrough (Google Slides)](https://docs.google.com/presentation/d/1PmYaakbtI3gBRAG-kowh5rV8UwPY0BTDbEdfOld-lHo/edit?slide=id.g40a268b6705_0_97#slide=id.g40a268b6705_0_97) shows the tester workflow alongside these instructions.

1. Connect the micro:bit V2 running the H/M μT-Kernel firmware using a USB data cable.
2. From `tools/`, install the requirements in a Python environment and run `python cute_yolo_tester.py` (or `python3` on systems where appropriate). The tester README gives the full environment setup.
3. Select the micro:bit serial port and click **Read active model**. It reports the label, slot, and H/M values through `MIN2`.
4. To change models, choose a compatible CUTE v2 `.cute` file on **Deploy via Serial**, then click **Upload and verify model**. The tester checks the package before transfer and verifies the active model after activation. No training is needed to run a supplied model.
5. On **Test on Device**, open an image or start the host webcam, then click **Capture and detect on device**. The tester sends one prepared frame and shows the returned boxes and timing. The micro:bit does not capture from a camera attached to itself.
6. Use the **Log** tab for upload output, active-model queries, test summaries, and errors.

The tester and firmware are separate: installing the Python packages does not flash firmware, and copying the firmware HEX does not install a `.cute` model. No deployable model is included in the firmware archive supplied for this documentation update.

## Model and inference path

The input is one 128 × 128 grayscale image (16,384 bytes). The output is a 16 × 16 grid with five INT8 values per cell (1,280 bytes), followed by on-device decode and non-maximum suppression (NMS).

| Stage | Output | CUTE v2 operation |
| --- | --- | --- |
| Input | 128 × 128 × 1 | Grayscale image |
| Stem 1 | 64 × 64 × 8 | Two split 3 × 3 branches |
| Stem 2 | 32 × 32 × 16 | Two split 3 × 3 branches |
| Stem 3 | 16 × 16 × 32 | Two split 3 × 3 branches |
| H1…H | 16 × 16 × 32 | Per block: DW 3 × 3 (32→32) + PW 1 × 1 (32→16) in each of two branches; concatenate 16+16 |
| Head M | 16 × 16 × 32 | M hidden 1 × 1 layers (32→32) for the split MLP head |
| Output | 16 × 16 × 5 | Split objectness (32→1) and box (32→4) layers |

The two hybrid branches are executed **sequentially** on the single-core micro:bit. For H6/M1, the package has **33 layer records**: six stem records, 24 hybrid records, one hidden-head record, and two output records. The loader also supports other compatible H/M depths within the device's reported limits and the 40 KiB model-slot size.

## μT-Kernel tasks and memory

The current `app_main.c` defines these priorities and stack allocations (a smaller number is a higher priority):

| Task | Priority | Allocated stack | Role |
| --- | ---: | ---: | --- |
| RX/COMM | 8 | 2 KiB | USB serial commands, image reception, model upload, responses |
| LED | 9 | 1 KiB | Heartbeat while the LCD initializes and during operation |
| LCD | 10 | 1.5 KiB | LCD initialization, image and box drawing |
| INFERENCE | 11 | 3 KiB | Noodle INT8 inference and decode/NMS |

The LCD initializes in its own task. For a normal request, RX receives the frame, asks LCD to draw the unmodified image when ready, asks INFERENCE to run, optionally asks LCD to draw boxes, then returns the serial response. Event flags coordinate these steps. Inference modifies the shared 16 KiB frame buffer in place, so the image is drawn first when the LCD is ready. These are allocated task-stack sizes, not measured high-water marks.

Noodle supplies tensor and buffer management with a packed activation arena. The μT-Kernel build sets `NOODLE_BUFFER_ARENA_INITIAL_BYTES` to **48 KiB**, avoiding repeated arena growth during inference. Model parameters are read from flash rather than copied into a per-layer SRAM weight array. The arena, task stacks, and shared input buffer are distinct memory consumers.

## CUTE v2 packages and deployment

The current architecture ID is `0xB3E82A71` (`cute_yolo_1_nhybrid_dualdwpw_32`). CUTE v2 has a 128-byte base header and 40-byte records for each layer; the full header is dynamically sized and aligned to 16 bytes. It stores H/M, quantization, class label, deployment thresholds, package length, and CRC checks. The loader validates structure, layer geometry, quantization boundaries, payload ranges, CRCs, and slot size before activation.

| Flash region | Address | Size |
| --- | --- | --- |
| Metadata | `0x00063000` | 4 KiB |
| Slot A | `0x00064000` | 40 KiB |
| Slot B | `0x0006E000` | 40 KiB |

A known H6/M1 face package is **26,128 bytes** and has label `face`; package size and label depend on the selected model. The firmware capability query (`MCAP`) reports supported version, H/M limits, slot size, and architecture before upload. The transfer uses `MBEG`/`MRED`, repeated `MDAT`/`MACK`, and `MEND`/`MACT`. Finally, `MIN2` reads back the active slot, H/M, layer count, flags, size, and label. Use the tester or `tools/upload_cute_serial.py` to deploy; check the actual package before uploading.

## Build the micro:bit firmware

From the `mtkernel_3` source tree with an ARM cross-toolchain installed:

```bash
make -C build_make -B -j2 all
arm-none-eabi-size build_make/mtkernel_3.elf
arm-none-eabi-objcopy -O ihex \
  build_make/mtkernel_3.elf build_make/mtkernel_3.hex
```

Copy `build_make/mtkernel_3.hex` to the mounted `MICROBIT` USB drive. Avoid a mass erase if you intend to keep the installed model slots. Firmware rebuilds and model deployment are separate operations. See `mtkernel_3/app_sample/README.md` for the CUTE v2 serial command descriptions; the task priority macros in `app_main.c` reflect the current build.

## Test and verification status

A normal request is `CUTE` plus 16,384 bytes; `DTOK` returns packed detections. `STAT` (or `RTIM`) returns the last transaction's timing. `LOGT` returns the 1,280-byte raw INT8 head for direct host/device comparison.

The earlier fixed-H5 micro:bit build achieved byte-identical head output against its host reference on a regression input. That historical result **does not establish byte identity for H6/M1**. For the current H/M firmware, confirm `MIN2` reports the intended H/M and package, compare all 1,280 `LOGT` head bytes against the matching host model on the same input, then measure repeated inference and detection runs. The 5,000 ms firmware deadline is provisional, not a worst-case execution-time guarantee. Timing from the STM32H743 or ESP32-S3 is not a micro:bit timing result.

## Known LCD behavior

The attached LCD may stay white after power-up despite startup mitigations. Briefly press and release Reset and wait a few seconds; if necessary, try one or two more short presses. A **long press on micro:bit V2 enters sleep**, so do not use it as the normal reset procedure. If the LCD remains blank but the tester receives boxes and timing, continue the demonstration on the computer display. A serial timeout or device error is a separate issue and should be checked in the Log tab.
