# Cute-YOLO on micro:bit V2

**Status:** Original working full-inference firmware
**Target:** micro:bit V2 (nRF52833), μT-Kernel 3.0, Noodle INT8

This README replaces the obsolete standalone Stem-1 test instructions.

## 1. Overview

Cute-YOLO runs a complete, single-class INT8 object detector on the micro:bit. The PC supplies a 128 × 128 grayscale image; the micro:bit performs inference and decode/NMS, draws the image and detection boxes on the attached LCD, and returns the detections over serial.

The deployed network uses a 16 × 16 grid with five values per cell. Its raw INT8 head contains 1,280 bytes. Network parameters are read from a validated `.cute` package in internal flash rather than compiled as a large weight array.

The application uses three μT-Kernel tasks:

| Task      | Priority | Responsibility                             |
| --------- | -------: | ------------------------------------------ |
| RX/COMM   |        8 | Serial protocol and pipeline orchestration |
| LCD       |        9 | Image and detection-box drawing            |
| INFERENCE |       10 | Noodle inference and decode/NMS            |

A shared 16 KiB image buffer is handed from RX to LCD to inference. Event flags enforce the order. Noodle uses its retained, grow-only activation arena and the μT-Kernel allocation backend.

## 2. Source files

Application directory:

```text
~/works/utkernel_microbit/mtkernel_3/app_sample/
```

The current application includes:

| File                        | Purpose                              |
| --------------------------- | ------------------------------------ |
| `app_main.c`                | RTOS tasks, serial protocol, timing  |
| `cute_full_flash.cpp`       | Full flash-backed detector execution |
| `cute_stem12.cpp`           | Stem execution support               |
| `cute_stem12_weights.h`     | Stem parameters                      |
| `cute_detect_flash.c/.h`    | Decode and NMS                       |
| `cute_model_flash.c/.h`     | Model discovery, upload, validation  |
| `cute_model_format.h`       | `.cute` format and flash layout      |
| `cute_lcd.c/.h`             | LCD driver and drawing               |
| `noodle_*.cpp / noodle_*.h` | Noodle runtime and configuration     |

The original build uses the C/C++ source wildcard in `build_make/mtkernel_3/app_sample/subdir.mk`. Only the active `.c` and `.cpp` sources are needed for the normal build. Old `*.bak` files and `send_stem1_test.py` are historical diagnostics, not runtime components.

Do not restore the old checksum, `L1OK`, or standalone Stem-1 application code. The full detector and its serial protocol are already present.

## 3. Build and flash

Use the original working kernel tree, not the separate CD tracer tree. The working build includes the C++17, INT8, and math-library settings.

```bash
cd ~/works/utkernel_microbit/mtkernel_3
make -C build_make -B -j2 all
arm-none-eabi-size build_make/mtkernel_3.elf
```

The explicit `all` target is intentional.

Close picocom, Studio, and other programs using the serial port. To flash through the micro:bit USB drive:

```bash
arm-none-eabi-objcopy -O ihex \
  build_make/mtkernel_3.elf build_make/mtkernel_3.hex

cp build_make/mtkernel_3.hex /run/media/auralius/MICROBIT/
sync
```

Wait for flashing to finish, then reset the board. Check the boot messages with:

```bash
picocom -b 115200 /dev/ttyACM0
```

Exit picocom with **Ctrl+A**, then **Ctrl+X**.

The model occupies separate flash sectors. A normal firmware update should not require another model upload, but verify model discovery afterward. Do not mass-erase the chip or overwrite the model sectors without a deliberate backup/redeployment plan.

## 4. Deploy the model

The known-good face model used in this project is `X_adapted.cute`. It is a deployable model package, not a firmware image.

From the Cute-YOLO host project root:

```bash
cd ~/works/Student-Embedded-Control-and-AI-Fest/Cute-YOLO

python3 tools/upload_cute_serial.py \
  tools/adapted_models/X_adapted.cute \
  --port /dev/ttyACM0
```

Alternatively, use the existing Studio **Deploy via Serial** tab. Keep other serial programs closed during upload.

The firmware supports `MBEG`, `MDAT`, `MEND`, and `MINF` for model management. The uploader transfers the package, validates it, and activates the selected flash slot. After a successful upload, reset the board and confirm that a valid `.cute` model is found.

Flash layout defined by `cute_model_format.h`:

| Region    | Address               |
| --------- | --------------------- |
| Metadata  | `0x00063000`          |
| Slot A    | `0x00064000`          |
| Slot B    | `0x0006E000`          |
| Slot size | `0x0000A000` (40 KiB) |

## 5. Run detection

The normal command is `CUTE`, followed by exactly 16,384 grayscale bytes. The host must use the established ESP32-style RGB565-to-grayscale and half-pixel bilinear preprocessing so the deployed input is consistent.

Use the existing Studio test interface or the known-good `send_webcam_detect.py` utility from the host bundle, for example:

```bash
python3 send_webcam_detect.py --image face1.jpg
```

Run that command from the directory containing the utility and image, or supply the appropriate image path.

**Normal response:** `DTOK` + 4-byte header remainder + `count × 5` detection bytes.

The eight-byte header contains `DTOK`, a one-byte count, and three reserved zero bytes. Each detection record contains:

```text
confidence_u8, x1, y1, x2, y2
```

Coordinates are image pixels; confidence is encoded on 0–255. `DTER` followed by a signed 32-bit error code reports a detector error.

`LOGT` is a separate regression command. It returns `FLOK`, a little-endian 32-bit output length, and the raw INT8 head (1,280 bytes for this model). It bypasses LCD drawing and decode/NMS; it is not the normal detection or display workflow.

Do not send arbitrary text to the binary protocol. T-Monitor and `sera` share the UART; application debug prints must not be mixed into active binary transfers.

## 6. Timing and reproducibility

The original firmware already records:

* UART receive time
* LCD image-drawing time
* Noodle inference time
* Decode + NMS time
* LCD box-overlay time
* End-to-end transaction time

Request `STAT` (or its `RTIM` alias) after a completed transaction. The 32-byte `STOK` response contains seven little-endian `uint32` values:

```text
frame_id, uart_rx_ms, lcd_image_ms, inference_ms,
decode_nms_ms, lcd_boxes_ms, total_ms
```

The current `CUTE_DEADLINE_MS` value is 5,000 ms. It is a provisional evaluation threshold, not a demonstrated worst-case execution time or a hard-real-time guarantee. Record repeated measurements under stated conditions before making timing claims.

For reproducible comparisons, keep the same firmware, `.cute` model, input image, preprocessing, and measurement procedure. The original and traced experimental builds previously produced identical 1,280-byte raw heads for the same face-model/image test. This is a single-input regression result, not a general accuracy or timing validation.

## 7. Noodle arena configuration

`NOODLE_BUFFER_ARENA_INITIAL_BYTES` is an initial allocation setting, not a maximum arena size. The source has a 64-byte fallback; an effective build-time override may change it. Preserve the configuration of the working build rather than changing the arena during unrelated tests.

A later integration experiment required a 48 KiB initial reservation to avoid repeated growth/allocation failure. That observation does not establish that the original working firmware had the same failure. If allocation is investigated again, record the effective build flags, initial capacity, peak arena usage, and actual failure point.

## 8. Task tracer experiment

The separate `tasktracer/` directory contains the CD tracer sample and experimental Cute-YOLO integration. It is not the original working firmware and is not required for normal inference, deployment, or the existing `STAT` measurements.

The CD sample tracer was demonstrated successfully. The later Cute-YOLO one-shot integration produced a matching raw inference head, but the detection response was truncated during the serial handoff. Captured tracer logs also reported lost events; an experimental UART drain patch was reversed after a HardFault. Those captures must not be presented as loss-free scheduling or timing evidence.

Keep the tracer experiment isolated. Do not copy its startup, driver, or one-shot trace changes into the original working project merely to build or run Cute-YOLO.

## 9. Historical note

The previous README documented only Stem 1:

```text
1 × 128 × 128 → 8 × 64 × 64 (32,768 INT8 outputs)
```

That was an earlier byte-exact kernel test. It has been superseded by the complete flash-backed detector and the current `CUTE`/`LOGT` protocol. The old `send_stem1_test.py` and `*.bak` files may be retained separately for historical debugging, but they are not required for this release.

