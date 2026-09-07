# Release v0.1.0-beta — Initial Beta Release

Initial beta release of **CUTE-YOLO on micro:bit V2**, integrating μT-Kernel 3.0 and the Noodle INT8 inference framework.

## 🎯 Overview

CUTE-YOLO (**Configurable Ultra-lightweight Tiny Embedded YOLO**) demonstrates a complete embedded object-detection pipeline on a resource-constrained microcontroller. A host supplies a 128 × 128 grayscale image, while the micro:bit performs neural-network inference, detection decoding, and LCD visualization.

This release preserves the working micro:bit implementation as the baseline for the TRON competition. The separate N-layer ESP32 project is not part of this release.

## ✨ Features

* **Complete INT8 detector:** Three downsampling stems, five hybrid blocks, and a 16 × 16 × 5 detection head.
* **μT-Kernel task architecture:** Separate RX/COMM, LCD, and INFERENCE tasks coordinated through event flags.
* **Blocking synchronization:** Serial reception and worker-task waits allow the scheduler to execute other ready tasks instead of continuously busy polling.
* **Noodle memory management:** Managed activation storage using the tensor–buffer–arena hierarchy.
* **Flash-backed model parameters:** A validated `.cute` package can be deployed separately from the firmware.
* **Serial deployment:** Model upload, validation, and activation through the existing host tools.
* **Detection visualization:** Display the input image and resulting bounding boxes on the attached LCD.
* **Timing instrumentation:** Records UART reception, LCD drawing, inference, decode/NMS, and total transaction time.

## 🛠️ Technical Details

| Item                | Value                            |
| ------------------- | -------------------------------- |
| MCU                 | micro:bit V2 / nRF52833          |
| OS                  | μT-Kernel 3.0                    |
| Inference framework | Noodle INT8                      |
| Input               | 128 × 128 × 1                    |
| Detection head      | 16 × 16 × 5                      |
| Hybrid architecture | Five fixed 8+24 blocks           |
| Application tasks   | RX/COMM, LCD, INFERENCE          |
| Model package       | `.cute` format version 1         |
| Model storage       | Internal flash, two 40 KiB slots |

## 📦 What is a `.cute` file?

A `.cute` file is a deployable model package containing the INT8 parameters, quantization information, architecture identifier, and detection settings. The current loader validates the package before activating it from internal flash.

The firmware and model are separate: **changing the deployed model does not require recompiling the entire firmware**, provided the package matches the supported architecture.

## 🚀 Getting Started

See the repository's **README.md** for the project overview and `mtkernel_3/app_sample/README.txt` for build instructions, model deployment, and serial protocol details.

## ✅ Verified Functionality

The working baseline has demonstrated full-model inference, serial model deployment, LCD drawing, detection decoding, and RTOS task orchestration.

A regression comparison between the original firmware and an experimental traced build produced identical raw inference-head bytes for the same model and test image. This is a single-input numerical regression result, not a general accuracy benchmark.

## ⚠️ Known Limitations

* The current micro:bit model has a **fixed five-hybrid-block topology**; arbitrary N-layer loading is not implemented in this release.
* The hybrid branches are executed sequentially on the single-core microcontroller, not simultaneously on multiple CPU cores.
* The 5,000 ms deadline setting is provisional and is **not a demonstrated worst-case execution-time guarantee**.
* Formal accuracy, latency, and memory-usage results should be reported from the finalized evaluation measurements.
* The optional CD Task Tracer integration has not produced a trustworthy loss-free trace and is not included as validated scheduling evidence.

## 🏆 TRON Competition

This release documents the μT-Kernel + Noodle + CUTE-YOLO implementation for the TRON competition. The project demonstrates how RTOS task coordination and managed neural-network memory can be combined on a constrained embedded platform.
