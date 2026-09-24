#!/usr/bin/env python3
from __future__ import annotations

import numpy as np


ESP32_CAMERA_W = 320
ESP32_CAMERA_H = 240
ESP32_DET_CROP_W = 240
ESP32_DET_CROP_H = 240
DETECTOR_SIZE = 128


def esp32_crop_rect(frame_bgr: np.ndarray) -> tuple[int, int, int, int]:
    """
    Return the centered square detector crop in source-image coordinates.

    For the real ESP32 QVGA camera path this is exactly:
        320x240 -> crop x=40, y=0, w=240, h=240.

    For arbitrary host images we use the same geometric rule:
        centered square with side=min(width,height).
    """
    if frame_bgr is None or frame_bgr.size == 0:
        raise ValueError("No image is available")

    h, w = frame_bgr.shape[:2]
    side = min(h, w)
    x0 = (w - side) // 2
    y0 = (h - side) // 2
    return x0, y0, side, side


def _rgb565_gray_float(crop_bgr: np.ndarray) -> np.ndarray:
    """
    Reproduce the ESP32 RGB565 -> grayscale conversion.

    The ESP32 firmware reads RGB565 components as:
        R5 / 31, G6 / 63, B5 / 31
    then computes:
        0.299 R + 0.587 G + 0.114 B
    """
    if crop_bgr.ndim != 3 or crop_bgr.shape[2] < 3:
        raise ValueError(
            f"Expected BGR image with 3 channels, got {crop_bgr.shape}"
        )

    b = crop_bgr[..., 0].astype(np.uint16)
    g = crop_bgr[..., 1].astype(np.uint16)
    r = crop_bgr[..., 2].astype(np.uint16)

    # Match 8-bit -> RGB565 truncation.
    r5 = (r >> 3).astype(np.float32)
    g6 = (g >> 2).astype(np.float32)
    b5 = (b >> 3).astype(np.float32)

    return (
        0.299 * (r5 / 31.0)
        + 0.587 * (g6 / 63.0)
        + 0.114 * (b5 / 31.0)
    ).astype(np.float32)


def _half_pixel_bilinear_gray(
    gray: np.ndarray,
    out_w: int = DETECTOR_SIZE,
    out_h: int = DETECTOR_SIZE,
) -> np.ndarray:
    """
    Exact geometry used by the ESP32 firmware:

        s = (d + 0.5) * (src / dst) - 0.5

    followed by clamping and bilinear interpolation.
    """
    src_h, src_w = gray.shape

    ys = (
        (np.arange(out_h, dtype=np.float32) + 0.5)
        * (float(src_h) / float(out_h))
        - 0.5
    )
    xs = (
        (np.arange(out_w, dtype=np.float32) + 0.5)
        * (float(src_w) / float(out_w))
        - 0.5
    )

    ys = np.clip(ys, 0.0, float(src_h - 1))
    xs = np.clip(xs, 0.0, float(src_w - 1))

    y0 = np.floor(ys).astype(np.int32)
    x0 = np.floor(xs).astype(np.int32)
    y1 = np.minimum(y0 + 1, src_h - 1)
    x1 = np.minimum(x0 + 1, src_w - 1)

    wy = (ys - y0).astype(np.float32)[:, None]
    wx = (xs - x0).astype(np.float32)[None, :]

    g00 = gray[y0[:, None], x0[None, :]]
    g10 = gray[y0[:, None], x1[None, :]]
    g01 = gray[y1[:, None], x0[None, :]]
    g11 = gray[y1[:, None], x1[None, :]]

    top = g00 + wx * (g10 - g00)
    bottom = g01 + wx * (g11 - g01)
    return top + wy * (bottom - top)


def esp32_preprocess_bgr(frame_bgr: np.ndarray) -> np.ndarray:
    """
    Convert a host BGR frame into the exact 128x128 GRAY8 detector input
    used by the ESP32-style Cute-YOLO camera pipeline.

    Pipeline:
        centered square crop
        -> RGB565 component quantization
        -> luminance grayscale
        -> half-pixel bilinear resize to 128x128
        -> lround-style conversion to uint8
    """
    if frame_bgr is None or frame_bgr.size == 0:
        raise ValueError("No image is available")

    # Allow grayscale callers, but preserve the same RGB565 path by
    # replicating gray into B/G/R channels.
    if frame_bgr.ndim == 2:
        frame_bgr = np.repeat(frame_bgr[..., None], 3, axis=2)
    elif frame_bgr.ndim == 3 and frame_bgr.shape[2] == 4:
        frame_bgr = frame_bgr[..., :3]

    x0, y0, cw, ch = esp32_crop_rect(frame_bgr)
    crop = np.ascontiguousarray(
        frame_bgr[y0:y0 + ch, x0:x0 + cw, :3],
        dtype=np.uint8,
    )

    gray_float = _rgb565_gray_float(crop)
    resized = _half_pixel_bilinear_gray(gray_float)

    # ESP32 uses lroundf(gray * 255). Values are non-negative, so
    # floor(x+0.5) is the same rounding rule.
    gray_u8 = np.floor(resized * 255.0 + 0.5)
    return np.clip(gray_u8, 0, 255).astype(np.uint8)
