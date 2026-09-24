# CUTE-YOLO H/M on micro:bit V2

**Target:** BBC micro:bit V2 (nRF52833), μT-Kernel 3.0, Noodle full-INT8  
**Model format:** CUTE v2, `cute_yolo_1_nhybrid_dualdwpw_32`

## 1. Current detector contract

The micro:bit firmware now consumes the same configurable CUTE-YOLO v2 packages produced by the current Studio and used by the ESP32-S3 implementation. The detector input is 128×128 grayscale and the output is a 5×16×16 anchor-free head (1,280 INT8 bytes).

The topology is parameterized by:

- **H**: number of 32-channel hybrid blocks.
- **M**: number of 32→32 hidden 1×1 head layers for the `mlp32_split` head.

Each hybrid block is:

```text
input 32×16×16
  ├─ DWA 3×3 depthwise 32→32 ─ PWA 1×1 32→16 ┐
  └─ DWB 3×3 depthwise 32→32 ─ PWB 1×1 32→16 ┘
                                      concat → 32×16×16
```

The ESP32-S3 executes the two branches concurrently. The micro:bit is single-core, so it executes branch A and branch B sequentially while preserving the same graph, quantization boundaries, and `.cute` package. H and M therefore change weight storage and execution time but do not require a different peak activation geometry; the same Noodle tensors are reused at every repeated block/head stage.

The current accuracy/latency study recommends **H6/M1**. H6/M2 is also structurally supported and is useful as an ablation model, subject to the 40-KiB slot-size limit.

## 2. μT-Kernel application

The application uses three tasks:

| Task | Priority | Responsibility |
| --- | ---: | --- |
| RX/COMM | 8 | Serial protocol and pipeline orchestration |
| LCD | 9 | Image and detection-box drawing |
| INFERENCE | 10 | Noodle inference and decode/NMS |

A shared 16-KiB image buffer is handed from RX to LCD to inference. Event flags enforce the pipeline order. `NOODLE_BUFFER_ARENA_INITIAL_BYTES` is set to the proven **48 KiB** initial allocation for this μT-Kernel build so the packed arena does not repeatedly grow through `Krealloc()` during inference.

Important application sources are `app_main.c`, `cute_full_flash.cpp`, `cute_detect_flash.c/.h`, `cute_model_flash.c/.h`, `cute_model_format.h`, `cute_lcd.c/.h`, and the Noodle runtime files.

## 3. CUTE v2 flash layout

The package has a stable 128-byte base header followed by 40-byte layer records. The header records H, M, layer count, package size, CRCs, quantization, class label, and deployment thresholds. The complete header size is dynamic and 16-byte aligned.

The raw nRF52833 model regions remain:

| Region | Address |
| --- | --- |
| Metadata | `0x00063000` |
| Slot A | `0x00064000` |
| Slot B | `0x0006E000` |
| Slot size | `0x0000A000` (40 KiB) |

The current H6/M1 package is about 26 KiB and H6/M2 about 28 KiB, so both fit. The host and firmware validate the actual package size before activation.

## 4. Serial model deployment

Use the Studio **Deploy via Serial** tab or the updated standalone uploader. Close picocom and every other process using `/dev/ttyACM0` first.

The updated protocol is:

```text
MCAP
  → device capability reply
MBEG + total_bytes
  → MRED + inactive target slot
MDAT + chunk_length + chunk
  → MACK + cumulative byte count
  ... repeat ...
MEND
  → MACT + newly active slot
MIN2
  → detailed active-model verification
```

`MCAP` is intentionally performed **before erasing/writing a slot**. It reports the supported CUTE version, H/M limits, slot size, and architecture ID. This prevents a current H/M package from being streamed to an older fixed-H5 firmware.

`MIN2` verifies the active slot, CUTE version, H, M, layer count, flags, total package bytes, and label after activation. The legacy 32-byte `MINF` command is retained for old host utilities.

Standalone deployment example:

```bash
python3 upload_cute_serial_hm.py \
  face_adapted_wider_tuned.cute \
  --port /dev/ttyACM0 \
  --chunk 128
```

Package-only validation can be done without a board:

```bash
python3 upload_cute_serial_hm.py model.cute --inspect-only
```

Before serial transfer the host verifies CUTE magic/version, architecture ID, H/M topology, dynamic header size, layer shapes, CRCs, payload ranges/alignment, the 40-KiB slot limit, and all raw-INT8 quantization boundaries used for split concatenation. The firmware repeats these structural and quantization checks before a slot is accepted.

## 5. Detection and regression commands

Normal detection uses `CUTE`, followed by exactly 16,384 grayscale bytes. The device runs inference, decode/NMS, LCD handling, and returns `DTOK` plus detection records. Each detection record is:

```text
confidence_u8, x1, y1, x2, y2
```

`LOGT` returns `FLOK`, a 32-bit output length, and the raw 1,280-byte INT8 detector head. It remains the preferred host/device numerical-regression path because it bypasses decode/NMS and LCD box drawing.

`STAT` (alias `RTIM`) returns the latest task timing record. The provisional `CUTE_DEADLINE_MS` remains 5,000 ms; it is an experimental deadline, not a formal WCET guarantee.

## 6. Build and flash

From the μT-Kernel tree:

```bash
cd ~/works/utkernel_microbit/mtkernel_3
make -C build_make -B -j2 all
arm-none-eabi-size build_make/mtkernel_3.elf
arm-none-eabi-objcopy -O ihex \
  build_make/mtkernel_3.elf build_make/mtkernel_3.hex
```

Then copy the HEX to the `MICROBIT` USB drive and reset the board. A normal application update should preserve the dedicated CUTE model sectors, but avoid mass-erase operations unless the model will be uploaded again.

The source update should be cross-built with the project's ARM toolchain before flashing. Host-side C/C++ syntax checks are useful, but they do not replace an nRF52833 link/build or hardware regression test.

## 7. First validation after this H/M update

For the first board test, use the current **H6/M1** package. After upload, verify `MIN2` reports H=6 and M=1, then run `LOGT` with the same regression image used on the host and compare all 1,280 head bytes. Only after byte-level consistency is confirmed should timing and detection measurements be collected for the new H6/M1 micro:bit result.
