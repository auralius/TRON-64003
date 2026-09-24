#!/usr/bin/env python3
"""Small micro:bit CUTE-YOLO deployment and inference interface for TRON."""

import base64
import queue
import subprocess
import sys
import threading
from pathlib import Path
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

import cv2
import serial
from serial.tools import list_ports

from cute_host_common import esp32_crop_rect, esp32_preprocess_bgr
from device_protocol import detect, model_info
from upload_cute_serial import inspect_package


class TesterApp:
    def __init__(self, root):
        self.root = root
        root.title("CUTE-YOLO · micro:bit demo")
        root.geometry("930x720")
        self.events = queue.Queue()
        self.busy = False
        self.capture = None
        self.camera_after = None
        self.frame = None
        self.model_path = tk.StringVar()
        self.port = tk.StringVar(value="/dev/ttyACM0" if sys.platform != "win32" else "")
        self.package = tk.StringVar(value="Choose a supplied .cute model")
        self.active = tk.StringVar(value="Active device model: unknown")
        self.result = tk.StringVar(value="Open an image or start the camera.")

        shell = ttk.Frame(root, padding=12)
        shell.pack(fill="both", expand=True)
        ttk.Label(shell, text="CUTE-YOLO · micro:bit V2", font=("TkDefaultFont", 18, "bold")).pack(anchor="w")
        ttk.Label(shell, text="Choose a model, deploy it over USB serial, and run detection on the micro:bit.").pack(anchor="w", pady=(3, 12))
        connection = ttk.Frame(shell)
        connection.pack(fill="x")
        ttk.Label(connection, text="Serial port").pack(side="left")
        self.ports = ttk.Combobox(connection, textvariable=self.port, width=30)
        self.ports.pack(side="left", padx=8)
        ttk.Button(connection, text="Refresh ports", command=self.refresh_ports).pack(side="left")
        ttk.Button(connection, text="Read active model", command=self.query_model).pack(side="left", padx=10)
        self.refresh_ports()
        ttk.Label(shell, textvariable=self.active).pack(anchor="w", pady=(7, 5))

        tabs = ttk.Notebook(shell)
        tabs.pack(fill="both", expand=True)
        deploy_tab = ttk.Frame(tabs, padding=12)
        test_tab = ttk.Frame(tabs, padding=12)
        log_tab = ttk.Frame(tabs, padding=12)
        tabs.add(deploy_tab, text="1  Deploy via Serial")
        tabs.add(test_tab, text="2  Test on Device")
        tabs.add(log_tab, text="3  Log")

        model_row = ttk.Frame(deploy_tab)
        model_row.pack(fill="x")
        ttk.Label(model_row, text=".cute model").pack(side="left")
        ttk.Entry(model_row, textvariable=self.model_path).pack(side="left", fill="x", expand=True, padx=8)
        ttk.Button(model_row, text="Browse…", command=self.choose_model).pack(side="left")
        ttk.Label(deploy_tab, textvariable=self.package, wraplength=850).pack(anchor="w", pady=12)
        self.upload_button = ttk.Button(deploy_tab, text="Upload and verify model", command=self.upload)
        self.upload_button.pack(anchor="w")
        ttk.Label(deploy_tab, text="After upload, the app verifies the active slot, H/M topology, layer count and package size through MIN2.", wraplength=850).pack(anchor="w", pady=10)
        self.progress = ttk.Progressbar(deploy_tab, mode="indeterminate")
        self.progress.pack(fill="x", pady=8)
        ttk.Label(log_tab, text="Serial deployment, model queries, device tests, and errors appear here.").pack(anchor="w")
        ttk.Button(log_tab, text="Clear log", command=self.clear_log).pack(anchor="e", pady=(0, 6))
        log_frame = ttk.Frame(log_tab)
        log_frame.pack(fill="both", expand=True)
        self.log = tk.Text(log_frame, wrap="word", state="disabled")
        scroll = ttk.Scrollbar(log_frame, orient="vertical", command=self.log.yview)
        self.log.configure(yscrollcommand=scroll.set)
        self.log.pack(side="left", fill="both", expand=True)
        scroll.pack(side="right", fill="y")

        controls = ttk.Frame(test_tab)
        controls.pack(fill="x")
        ttk.Button(controls, text="Open image…", command=self.open_image).pack(side="left")
        ttk.Button(controls, text="Start camera", command=self.start_camera).pack(side="left", padx=6)
        ttk.Button(controls, text="Stop camera", command=self.stop_camera).pack(side="left")
        self.detect_button = ttk.Button(controls, text="Capture and detect on device", command=self.start_detect)
        self.detect_button.pack(side="right")
        previews = ttk.Frame(test_tab)
        previews.pack(fill="both", expand=True, pady=12)
        for col, title in enumerate(("Source (red square = detector crop)", "128×128 device input and detections")):
            box = ttk.LabelFrame(previews, text=title, padding=6)
            box.grid(row=0, column=col, sticky="nsew", padx=4)
            label = ttk.Label(box, text="No image", anchor="center")
            label.pack(fill="both", expand=True)
            if col == 0:
                self.source_label = label
            else:
                self.output_label = label
            previews.columnconfigure(col, weight=1)
        previews.rowconfigure(0, weight=1)
        ttk.Label(test_tab, textvariable=self.result, wraplength=850).pack(anchor="w")
        self.table = ttk.Treeview(test_tab, columns=("confidence", "x1", "y1", "x2", "y2"), show="headings", height=5)
        for name in self.table["columns"]:
            self.table.heading(name, text=name)
            self.table.column(name, width=95, anchor="center")
        self.table.pack(fill="x", pady=5)
        root.protocol("WM_DELETE_WINDOW", self.close)
        self.poll()

    def refresh_ports(self):
        available = [p.device for p in list_ports.comports()]
        self.ports.configure(values=available)
        if available and self.port.get() not in available:
            self.port.set(available[0])

    def choose_model(self):
        path = filedialog.askopenfilename(filetypes=[("CUTE models", "*.cute"), ("All files", "*")])
        if path:
            self.model_path.set(path)
            try:
                info = inspect_package(Path(path).read_bytes())
                self.package.set(f"{Path(path).name} · {info['label']} · H{info['H']}/M{info['M']} · {info['layer_count']} layers · {info['total_bytes']:,} bytes · CRC verified")
            except Exception as exc:
                self.package.set(f"Invalid model: {exc}")

    def run_worker(self, operation, *args):
        if self.busy:
            return
        if not self.port.get().strip():
            messagebox.showerror("Serial port", "Choose a micro:bit serial port first.")
            return
        self.busy = True
        self.progress.start(12)
        threading.Thread(target=self.worker, args=(operation, self.port.get().strip(), *args), daemon=True).start()

    def worker(self, operation, port_name, *args):
        try:
            if operation == "upload":
                path = args[0]
                proc = subprocess.Popen([sys.executable, "-u", str(Path(__file__).with_name("upload_cute_serial.py")), path, "--port", port_name], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
                for line in proc.stdout:
                    self.events.put(("log", line))
                if proc.wait() != 0:
                    raise RuntimeError("Upload failed; see the deployment log.")
                self.events.put(("uploaded", None))
            else:
                with serial.Serial(port_name, 115200, timeout=60, write_timeout=10) as port:
                    if operation == "model":
                        result = model_info(port)
                    else:
                        info = model_info(port)
                        if info["H"] == 0:
                            raise RuntimeError("No active model. Upload a .cute model first.")
                        result = (info, *detect(port, args[0]))
                self.events.put((operation, result))
        except Exception as exc:
            self.events.put(("error", str(exc)))
        finally:
            self.events.put(("idle", None))

    def upload(self):
        path = self.model_path.get().strip()
        try:
            inspect_package(Path(path).read_bytes())
        except Exception as exc:
            messagebox.showerror("CUTE model", str(exc))
            return
        self.append_log(f"\nDeploying {Path(path).name} on {self.port.get().strip()}\n")
        self.run_worker("upload", path)

    def query_model(self):
        self.append_log(f"Querying active model on {self.port.get().strip()}\n")
        self.run_worker("model")

    def clear_log(self):
        self.log.configure(state="normal")
        self.log.delete("1.0", "end")
        self.log.configure(state="disabled")

    def append_log(self, line):
        self.log.configure(state="normal")
        self.log.insert("end", line)
        self.log.see("end")
        self.log.configure(state="disabled")

    def display(self, label, image):
        h, w = image.shape[:2]
        factor = min(410 / w, 320 / h)
        image = cv2.resize(image, (int(w * factor), int(h * factor)), interpolation=cv2.INTER_NEAREST if factor > 1 else cv2.INTER_AREA)
        ok, png = cv2.imencode(".png", image)
        if not ok:
            raise RuntimeError("Preview PNG encoding failed")
        photo = tk.PhotoImage(data=base64.b64encode(png.tobytes()).decode("ascii"))
        label.configure(image=photo, text="")
        label.image = photo

    def show_source(self):
        if self.frame is None:
            return
        guided = self.frame.copy()
        x, y, w, h = esp32_crop_rect(guided)
        cv2.rectangle(guided, (x, y), (x + w - 1, y + h - 1), (0, 0, 255), 2)
        self.display(self.source_label, guided)

    def show_output(self, gray, detections=()):
        canvas = cv2.cvtColor(gray, cv2.COLOR_GRAY2BGR)
        for det in detections:
            cv2.rectangle(canvas, (det["x1"], det["y1"]), (det["x2"], det["y2"]), (0, 255, 0), 1)
        self.display(self.output_label, canvas)

    def open_image(self):
        path = filedialog.askopenfilename(filetypes=[("Images", "*.png *.jpg *.jpeg *.bmp *.tif *.tiff")])
        if not path:
            return
        frame = cv2.imread(path)
        if frame is None:
            messagebox.showerror("Image", "Could not read that image.")
            return
        self.stop_camera()
        self.frame = frame
        self.show_source()
        self.show_output(esp32_preprocess_bgr(frame))
        self.result.set(f"Loaded {Path(path).name}; ready to send 128×128 GRAY8 input.")

    def start_camera(self):
        self.stop_camera()
        capture = cv2.VideoCapture(0)
        if not capture.isOpened():
            capture.release()
            messagebox.showerror("Camera", "Could not open camera 0.")
            return
        capture.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
        capture.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)
        self.capture = capture
        self.camera_tick()

    def camera_tick(self):
        if self.capture is None:
            return
        ok, frame = self.capture.read()
        if ok:
            self.frame = frame
            self.show_source()
        self.camera_after = self.root.after(80, self.camera_tick)

    def stop_camera(self):
        if self.camera_after is not None:
            self.root.after_cancel(self.camera_after)
            self.camera_after = None
        if self.capture is not None:
            self.capture.release()
            self.capture = None

    def start_detect(self):
        if self.frame is None:
            messagebox.showerror("Input", "Open an image or start the camera first.")
            return
        frame = self.frame.copy()
        self.stop_camera()
        gray = esp32_preprocess_bgr(frame)
        self.show_output(gray)
        self.pending_gray = gray
        self.result.set("Sending frame and waiting for device detections…")
        self.append_log(f"Testing device on {self.port.get().strip()} with a 128×128 grayscale frame\n")
        self.run_worker("detect", gray.tobytes())

    def poll(self):
        while True:
            try:
                kind, value = self.events.get_nowait()
            except queue.Empty:
                break
            if kind == "log":
                self.append_log(value)
            elif kind == "error":
                self.append_log("ERROR: " + value + "\n")
                self.result.set(value)
                messagebox.showerror("micro:bit", value)
            elif kind == "uploaded":
                self.append_log("Model upload and MIN2 verification complete.\n")
                self.active.set("Active device model: verified; press Read active model to view details")
            elif kind == "model":
                self.active.set(f"Active model: {value['label'] or '(none)'} · H{value['H']}/M{value['M']} · slot {value['slot']} · {value['bytes']:,} bytes")
                self.append_log(f"Active model: {value['label'] or '(none)'} · H{value['H']}/M{value['M']} · slot {value['slot']} · {value['bytes']:,} bytes\n")
            elif kind == "detect":
                info, detections, timing = value
                self.active.set(f"Active model: {info['label']} · H{info['H']}/M{info['M']} · slot {info['slot']}")
                self.show_output(self.pending_gray, detections)
                for item in self.table.get_children():
                    self.table.delete(item)
                for det in detections:
                    self.table.insert("", "end", values=(f"{det['confidence']:.3f}", det['x1'], det['y1'], det['x2'], det['y2']))
                duration = f" · inference {timing['inference_ms']} ms · total {timing['total_ms']} ms" if timing else ""
                summary = f"{len(detections)} detection(s){duration}"
                self.result.set(summary)
                self.append_log(f"Device result: {summary}\n")
            elif kind == "idle":
                self.busy = False
                self.progress.stop()
        self.root.after(80, self.poll)

    def close(self):
        self.stop_camera()
        self.root.destroy()


if __name__ == "__main__":
    TesterApp(tk.Tk()).root.mainloop()
