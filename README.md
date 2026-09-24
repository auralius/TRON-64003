# CUTE-YOLO

**Configurable Ultra-lightweight Tiny Embedded YOLO**

CUTE-YOLO is a compact single-class object detector for constrained microcontrollers. These demonstrations run Noodle INT8 inference under μT-Kernel 3.0. A model is packaged as a separate CUTE v2 `.cute` file and deployed over USB serial; training is not needed to try a prepared model. In the model name, **H** is the number of hybrid blocks and **M** is the number of hidden head layers. **H6/M1** is the model used for the current demonstration.

## Choose your demonstration

| Board | Image source | How to try it | Guide |
| --- | --- | --- | --- |
| BBC micro:bit V2 | Image or webcam frame sent from a computer over USB serial | Use the CUTE-YOLO tester to deploy a model and send images for detection | [Micro:bit setup and testing](README_MICROBIT.md) |
| STM32H743 with OV5640 | Live camera on the board | Preview live video, press K1 to freeze and infer, then press K1 again to resume | [STM32 live-camera demo](README_STM32_LIVE_CAMERA.md) |

Both boards accept compatible `.cute` packages through the serial deployment workflow. Check each board's reported capability and size limit before uploading. The micro:bit tester's **Test on Device** tab sends computer images to the micro:bit; the STM32 uses its own camera and K1 button instead.

The detector originated in the [CUTE-YOLO ESP32 project](https://github.com/Student-Embedded-Control-and-AI-Fest/Cute-YOLO). The guides above describe these μT-Kernel demonstrations and their board-specific instructions.

## Descriptive Poster

![](https://github.com/auralius/TRON-64003/blob/main/CUTE_YOLO_detailed_poster.png)
