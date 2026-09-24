"""The micro:bit CUTE v2 serial test protocol."""

import struct


def read_exact(port, count):
    data = bytearray()
    while len(data) < count:
        part = port.read(count - len(data))
        if not part:
            raise TimeoutError(f"Device replied with {len(data)}/{count} bytes")
        data.extend(part)
    return bytes(data)


def model_info(port):
    port.reset_input_buffer()
    port.write(b"MIN2")
    port.flush()
    reply = read_exact(port, 40)
    if reply[:4] != b"MIN2":
        raise RuntimeError(f"Unexpected model reply: {reply[:8]!r}")
    layers, flags, size = struct.unpack_from("<HHI", reply, 8)
    return {
        "slot": chr(reply[4]) if reply[4] else "?",
        "version": reply[5], "H": reply[6], "M": reply[7],
        "layers": layers, "flags": flags, "bytes": size,
        "label": reply[16:40].split(b"\0", 1)[0].decode("utf-8", "replace"),
    }


def detect(port, gray):
    if len(gray) != 128 * 128:
        raise ValueError("The detector needs exactly 128×128 GRAY8 bytes")
    port.reset_input_buffer()
    port.write(b"CUTE")
    port.write(gray)
    port.flush()
    header = read_exact(port, 8)
    if header[:4] == b"DTER":
        raise RuntimeError(f"Device inference failed: {struct.unpack('<i', header[4:])[0]}")
    if header[:4] != b"DTOK":
        raise RuntimeError(f"Unexpected detector reply: {header!r}")
    records = read_exact(port, header[4] * 5)
    detections = []
    for offset in range(0, len(records), 5):
        confidence, x1, y1, x2, y2 = records[offset:offset + 5]
        detections.append({"confidence": confidence / 255, "x1": x1,
                           "y1": y1, "x2": x2, "y2": y2})
    port.write(b"STAT")
    port.flush()
    timing = read_exact(port, 32)
    stats = None
    if timing[:4] == b"STOK":
        keys = ("frame_id", "uart_rx_ms", "lcd_image_ms", "inference_ms",
                "decode_nms_ms", "lcd_boxes_ms", "total_ms")
        stats = dict(zip(keys, struct.unpack("<7I", timing[4:])))
    return detections, stats
