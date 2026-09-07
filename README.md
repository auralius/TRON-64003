# CUTE-YOLO on micro:bit V2

**Configurable Ultra-lightweight Tiny Embedded YOLO**  
**TRON competition project — μT-Kernel 3.0 + Noodle INT8**

CUTE-YOLO is a compact, single-class object detector designed for embedded systems with limited memory. This project demonstrates a complete face-detection pipeline on a micro:bit V2: a host sends an image, the microcontroller executes the neural network, and the result is drawn on an attached LCD and returned to the host.

The contribution is not simply running a small neural network. It is the integration of **flash-backed model parameters, Noodle's managed activation memory, and an RTOS-coordinated image-processing pipeline** on a constrained microcontroller.

The current micro:bit release uses a **fixed five-hybrid-block model**. “Configurable” is the CUTE family name and the direction of the separate N-layer ESP32 project; this firmware does not yet load arbitrary network topologies.

## What the demonstration does

The host prepares a 128 × 128 grayscale image and sends it through USB serial. The micro:bit displays the image, runs INT8 inference, decodes the detection head, performs non-maximum suppression (NMS), draws the resulting boxes, and returns their coordinates and confidence values.

```text
Host / CUTE-YOLO Studio
        │
        │ 128 × 128 grayscale image
        ▼
micro:bit V2 — μT-Kernel 3.0
        │
        ├── RX/COMM ──► LCD image
        │                  │
        │                  ▼
        ├────────────► Noodle INT8 inference
        │                  │
        │                  ▼
        │              Decode + NMS
        │                  │
        ├────────────► LCD detection boxes
        │
        └────────────► Serial detection response
```

The host supplies the image; the micro:bit performs the detector computation. This release does not claim that the micro:bit captures live camera frames itself.

## The neural network

The current model contains three downsampling stems, five hybrid blocks, and a detection head.

| Stage | Output shape | Operation |
|---|---|---|
| Input | 128 × 128 × 1 | Grayscale image |
| Stem 1 | 64 × 64 × 8 | Split 3 × 3 convolution |
| Stem 2 | 32 × 32 × 16 | Split 3 × 3 convolution |
| Stem 3 | 16 × 16 × 32 | Split 3 × 3 convolution |
| Hybrid blocks H1–H5 | 16 × 16 × 32 | 8-channel conventional branch + 24-channel depthwise/pointwise branch |
| Detection head | 16 × 16 × 5 | 1 × 1 convolution |

Each hybrid block combines a conventional 3 × 3 branch with a depthwise 3 × 3 → pointwise 1 × 1 branch. Their 8 and 24 output channels are concatenated to restore 32 channels. The detector has **22 convolution records** in total: six stem convolutions, fifteen hybrid-branch convolutions, and one head convolution.

The five head values per grid cell represent objectness and box geometry. The decoder applies the model's quantization information, confidence threshold, and NMS settings to produce detections.

**The branches are architectural parallelism, not simultaneous execution on two CPU cores.** The micro:bit implementation executes the convolution operations sequentially and reuses activation storage.

## RTOS structure

The application has three long-lived μT-Kernel tasks. A smaller numeric priority means a higher scheduling priority.

| Task | Priority | Stack allocation | Responsibility |
|---|---:|---:|---|
| RX/COMM | 8 | 2 KiB | Serial commands, image reception, orchestration, responses |
| LCD | 9 | 1.5 KiB | Runtime image and box drawing |
| INFERENCE | 10 | 3 KiB | Noodle inference and decode/NMS |

All three tasks use a `for (;;)` service loop. **An infinite task lifetime does not mean continuous CPU use.** RX normally blocks while waiting for serial data; the workers block on RTOS event flags until requested. The UART driver uses interrupts and wakes a waiting task when data arrives.

For a normal `CUTE` request, the execution order is:

```text
RX receives image
       │
       ▼
RX signals LCD_IMAGE ──► LCD draws image
RX waits for LCD_DONE ◄── LCD signals completion
       │
       ▼
RX signals INFER_RUN ──► INFERENCE runs Noodle + decode/NMS
RX waits for INFER_DONE ◄── INFERENCE signals completion
       │
       ▼
RX signals LCD_BOXES ──► LCD draws boxes
RX waits for LCD_DONE ◄── LCD signals completion
       │
       ▼
RX sends detections and waits for the next command
```

The image is drawn before inference because inference quantizes the shared input in place. Event flags enforce ownership so the same 16 KiB image buffer can be reused safely. This is **preemptive, event-driven scheduling on a single-core MCU**, not three computations running simultaneously.

## What is Noodle?

**Noodle** is the project's C/C++ inference framework. It provides the tensor operations and memory-management machinery used by the detector.

The relevant memory hierarchy is:

- **NoodleTensor:** logical shape, type, quantization information, and buffer association.
- **NoodleBuffer:** retained capacity that can grow as needed.
- **NoodleArena:** physical placement of activation buffers, including packed storage and relocation when the arena expands.

This lets the implementation reuse intermediate storage rather than allocating a separate full-sized activation array for every network layer. The micro:bit port connects Noodle's allocator to the μT-Kernel allocation functions.

The **3 KiB INFERENCE stack is not the whole neural-network memory budget**. The shared image is static SRAM storage, activation memory is managed separately by Noodle, and model parameters reside in flash. The task-stack sizes are configured allocations, not measured peak stack usage.

## What is a `.cute` file?

A `.cute` file is the project's **deployable, self-describing model package**. It is neither an ordinary `.tflite` file nor a complete firmware image.

The current format contains a 1,024-byte header followed by model data. Its header includes the format version, architecture identifier, layer records, quantization information, detection settings, and CRC32 checksums. The payload contains INT8 weights and associated INT32 bias/requantization data.

The current micro:bit loader supports **format version 1 and a fixed 22-record architecture**. It validates the package before activation; the file is not a general-purpose arbitrary-topology interpreter.

```text
Training / adaptation in CUTE-YOLO Studio
                  │
                  ▼
           INT8 model export
                  │
                  ▼
          .cute model package
                  │
          USB serial deployment
                  ▼
       micro:bit internal flash
                  │
        Validate and activate
                  ▼
         Noodle INT8 inference
```

The known-good face package is `X_adapted.cute` (28,412 bytes). The current flash layout has two 40 KiB model slots and a metadata page. Model deployment can therefore be performed separately from rebuilding the firmware.

## Running the demonstration

The original working firmware is under `mtkernel_3/`. Use the existing CUTE-YOLO host tools and the original kernel build, not the separate experimental tracer tree.

The normal serial request is `CUTE` followed by 16,384 image bytes. The response begins with `DTOK` and contains the detection records. The `LOGT` command returns the raw 1,280-byte INT8 head for regression testing; `STAT` returns the last transaction's timing measurements.

For exact build commands, model deployment, protocol layouts, flash addresses, and troubleshooting, see **[the micro:bit application README](mtkernel_3/app_sample/README.txt)**.

## What has been demonstrated?

The working baseline establishes full-model inference, serial model deployment, image display, detection decoding, and RTOS task orchestration. A regression comparison between the original firmware and the experimental traced build produced identical raw head bytes for the same model and test image.

The firmware also records UART reception, LCD drawing, inference, decode/NMS, and end-to-end transaction times. **The 5,000 ms deadline setting is provisional; it is not a measured worst-case execution time or a hard-real-time guarantee.** Formal timing statistics must be based on repeated measurements.

The optional CD Task Tracer experiment is separate. Its sample application worked, but the Cute-YOLO integration did not produce a trustworthy loss-free trace. Those experimental captures are not presented as final scheduling evidence.

## Project scope

This repository is the **micro:bit μT-Kernel competition implementation**. The N-layer CUTE-YOLO project on ESP32 is a separate continuation focused on configurable network depth and hybrid topology export.

The micro:bit release is preserved as the reference implementation rather than modified to accommodate the next project.

