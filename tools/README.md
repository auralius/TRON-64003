# CUTE-YOLO micro:bit tester

This application has **Deploy via Serial**, **Test on Device**, and **Log** tabs. It does not train models and does not need TensorFlow. It accepts compatible CUTE v2 `.cute` packages supplied separately.

## Start

Install Python 3.10+ and Tkinter. On Ubuntu/Debian, Tkinter is available through `sudo apt install python3-tk`. From this folder:

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -r requirements.txt
python cute_yolo_tester.py
```

On Windows, activate the environment with `.venv\Scripts\activate` and use `python` in place of `python3`. If another program has the serial port open, close it before using this application.

## Demonstration

1. Plug in the micro:bit V2 running the configurable CUTE v2 μT-Kernel firmware. Select its serial port and click **Read active model**.
2. Under **Deploy via Serial**, choose a supplied `.cute` model. The app checks CRC, layer topology, architecture, and the micro:bit slot limit before upload. Click **Upload and verify model**. The uploader checks `MCAP` before writing and verifies the activated model with `MIN2` after upload.
3. Under **Test on Device**, open an image or start the laptop camera. Click **Capture and detect on device**. The host sends a 128×128 grayscale frame to the micro:bit. The micro:bit runs the detector and returns boxes and timing. Green boxes and the timing summary appear in the app. The **Log** tab keeps the deployment output, model queries, test summaries, and errors until you clear it.

To try another model, return to the first tab, select another `.cute` file, and deploy it. This project does not include a model package yet.

## Files

- `cute_yolo_tester.py`: three-tab graphical application.
- `upload_cute_serial.py`: original serial uploader with `MCAP`, `MBEG`, `MDAT`, `MEND`, and `MIN2` validation.
- `device_protocol.py`: device model query and image inference transactions.
- `cute_host_common.py`: the existing RGB565-to-grayscale, centered-crop, half-pixel resize preprocessing used by the Studio.

Before using it for a demonstration, perform an end-to-end trial on the actual micro:bit and with each supplied model. The CUTE firmware must be flashed separately.
