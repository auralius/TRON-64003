#!/usr/bin/env python3
from __future__ import annotations

import argparse
import struct
import time
import zlib
from pathlib import Path

try:
    import serial
except Exception:
    serial = None


CUTE_V2_ARCH_ID = 0xB3E82A71
CUTE_BASE_BYTES = 128
CUTE_RECORD_BYTES = 40
CUTE_HEADER_ALIGNMENT = 16
CUTE_FLAG_MLP32_SPLIT_HEAD = 0x0001
CUTE_SLOT_BYTES = 0xA000


def align_up(value, alignment):
    return (value + alignment - 1) & ~(alignment - 1)


def expected_counts(index, n, m, flags):
    stem_w = (36, 36, 576, 576, 2304, 2304)
    stem_b = (4, 4, 8, 8, 16, 16)
    if index < 6:
        return stem_w[index], stem_b[index]

    head = 6 + 4 * n
    if index < head:
        role = (index - 6) % 4
        return (288, 32) if role in (0, 2) else (512, 16)

    if not (flags & CUTE_FLAG_MLP32_SPLIT_HEAD):
        if index == head:
            return 160, 5
        raise ValueError(f"invalid layer index {index}")

    if head <= index < head + m:
        return 1024, 32
    if index == head + m:
        return 32, 1
    if index == head + m + 1:
        return 128, 4
    raise ValueError(f"invalid layer index {index}")


def inspect_package(blob):
    if len(blob) < CUTE_BASE_BYTES or blob[:4] != b"CUTE":
        raise RuntimeError("not a valid CUTE package")

    version, header_bytes = struct.unpack_from("<HH", blob, 4)
    arch_id, total_bytes, payload_crc = struct.unpack_from("<III", blob, 8)
    layer_count, flags = struct.unpack_from("<HH", blob, 20)
    label = blob[24:48].split(b"\0", 1)[0].decode("utf-8", errors="replace")
    n = blob[65]
    encoded_m = blob[66]
    header_crc = struct.unpack_from("<I", blob, 124)[0]

    if version != 2:
        raise RuntimeError(f"micro:bit H/M firmware requires CUTE v2, got v{version}")
    if arch_id != CUTE_V2_ARCH_ID:
        raise RuntimeError(
            f"architecture 0x{arch_id:08X} != expected 0x{CUTE_V2_ARCH_ID:08X}"
        )
    if flags & ~CUTE_FLAG_MLP32_SPLIT_HEAD:
        raise RuntimeError(f"unsupported flags 0x{flags:04X}")
    if not 1 <= n <= 12:
        raise RuntimeError(f"invalid H={n}")

    if flags & CUTE_FLAG_MLP32_SPLIT_HEAD:
        m = encoded_m if encoded_m else 1
        if not 1 <= m <= 12:
            raise RuntimeError(f"invalid M={m}")
        expected_layers = 8 + 4 * n + m
    else:
        if encoded_m != 0:
            raise RuntimeError("linear head must encode M=0")
        m = 0
        expected_layers = 7 + 4 * n

    expected_header = align_up(
        CUTE_BASE_BYTES + expected_layers * CUTE_RECORD_BYTES,
        CUTE_HEADER_ALIGNMENT,
    )
    if layer_count != expected_layers:
        raise RuntimeError(
            f"layer count {layer_count} != expected {expected_layers} for H{n}/M{m}"
        )
    if header_bytes != expected_header:
        raise RuntimeError(
            f"header bytes {header_bytes} != expected {expected_header}"
        )
    if total_bytes != len(blob):
        raise RuntimeError(
            f"file size {len(blob)} != header total {total_bytes}"
        )
    if total_bytes > CUTE_SLOT_BYTES:
        raise RuntimeError(
            f"package {total_bytes} exceeds micro:bit slot {CUTE_SLOT_BYTES}"
        )

    h = bytearray(blob[:header_bytes])
    h[124:128] = b"\0\0\0\0"
    got_hcrc = zlib.crc32(h) & 0xFFFFFFFF
    if got_hcrc != header_crc:
        raise RuntimeError(
            f"header CRC mismatch 0x{got_hcrc:08X} != 0x{header_crc:08X}"
        )

    got_pcrc = zlib.crc32(blob[header_bytes:]) & 0xFFFFFFFF
    if got_pcrc != payload_crc:
        raise RuntimeError(
            f"payload CRC mismatch 0x{got_pcrc:08X} != 0x{payload_crc:08X}"
        )

    fmt = "<IIIIIIffii"
    q = []
    for i in range(layer_count):
        off = CUTE_BASE_BYTES + i * CUTE_RECORD_BYTES
        wo, wc, bo, bc, mo, so, ins, outs, izp, ozp = struct.unpack_from(fmt, blob, off)
        ewc, ebc = expected_counts(i, n, m, flags)
        if (wc, bc) != (ewc, ebc):
            raise RuntimeError(
                f"L{i} shape mismatch W={wc}/{ewc} B={bc}/{ebc}"
            )
        if not (ins > 0 and outs > 0):
            raise RuntimeError(f"L{i} invalid quantization scale")
        for data_off, size in (
            (wo, wc), (bo, bc * 4), (mo, bc * 4), (so, bc * 4)
        ):
            if data_off < header_bytes or data_off + size > total_bytes:
                raise RuntimeError(f"L{i} payload range invalid")
        if bo % 4 or mo % 4 or so % 4:
            raise RuntimeError(f"L{i} INT32 array alignment invalid")
        q.append((ins, izp, outs, ozp))

    def same_output_input(a, b):
        return a[2] == b[0] and a[3] == b[1]

    # Header/stem and split-branch concat boundaries.
    input_scale = struct.unpack_from("<f", blob, 68)[0]
    input_zp = struct.unpack_from("<i", blob, 72)[0]
    output_scale = struct.unpack_from("<f", blob, 76)[0]
    output_zp = struct.unpack_from("<i", blob, 80)[0]

    if not (q[0][0] == input_scale and q[0][1] == input_zp):
        raise RuntimeError("model input quantization != Stem1 input")
    for a, b in ((0, 1), (2, 3), (4, 5)):
        if not (q[a][0:2] == q[b][0:2] and q[a][2:4] == q[b][2:4]):
            raise RuntimeError(f"split stem quantization mismatch at L{a}/L{b}")
    if not same_output_input(q[0], q[2]) or not same_output_input(q[2], q[4]):
        raise RuntimeError("stem boundary quantization mismatch")

    previous = q[4][2:4]
    for block in range(n):
        first = 6 + 4 * block
        dwa, pwa, dwb, pwb = q[first:first + 4]
        if dwa[0:2] != previous or dwb[0:2] != previous:
            raise RuntimeError(f"H{block+1} DW input quantization mismatch")
        if not same_output_input(dwa, pwa) or not same_output_input(dwb, pwb):
            raise RuntimeError(f"H{block+1} DW/PW quantization mismatch")
        if pwa[2:4] != pwb[2:4]:
            raise RuntimeError(f"H{block+1} concat quantization mismatch")
        previous = pwa[2:4]

    head = 6 + 4 * n
    if flags & CUTE_FLAG_MLP32_SPLIT_HEAD:
        for hidden in range(m):
            layer = q[head + hidden]
            if layer[0:2] != previous:
                raise RuntimeError(f"M{hidden+1} input quantization mismatch")
            previous = layer[2:4]
        obj = q[head + m]
        box = q[head + m + 1]
        if obj[0:2] != previous or box[0:2] != previous:
            raise RuntimeError("head branch input quantization mismatch")
        if obj[2:4] != box[2:4] or obj[2:4] != (output_scale, output_zp):
            raise RuntimeError("head output quantization mismatch")
    else:
        layer = q[head]
        if layer[0:2] != previous or layer[2:4] != (output_scale, output_zp):
            raise RuntimeError("linear head quantization mismatch")

    return {
        "version": version,
        "arch_id": arch_id,
        "total_bytes": total_bytes,
        "layer_count": layer_count,
        "flags": flags,
        "label": label,
        "H": n,
        "M": m,
    }


def read_exact(ser, n):
    data = bytearray()
    while len(data) < n:
        part = ser.read(n - len(data))
        if not part:
            raise RuntimeError(f"serial timeout: got {len(data)}/{n}")
        data.extend(part)
    return bytes(data)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("cute")
    parser.add_argument("--port", default="/dev/ttyACM0")
    parser.add_argument("--chunk", type=int, default=128)
    parser.add_argument(
        "--inspect-only",
        action="store_true",
        help="validate and print .cute H/M metadata without opening a serial port",
    )
    args = parser.parse_args()

    blob = Path(args.cute).read_bytes()
    package = inspect_package(blob)

    if not 1 <= args.chunk <= 256:
        raise RuntimeError("--chunk must be 1..256")

    print("Uploading:", args.cute)
    print("Bytes:", len(blob))
    print(
        f"Package: CUTE v{package['version']} H{package['H']}/M{package['M']} "
        f"layers={package['layer_count']} label={package['label']!r}"
    )

    if args.inspect_only:
        print("VALID: package CRCs, H/M topology, layer shapes, and slot size are consistent.")
        return

    if serial is None:
        raise RuntimeError("pyserial is not installed; run: pip install pyserial")

    with serial.Serial(args.port, 115200, timeout=30) as ser:
        time.sleep(1.0)
        ser.reset_input_buffer()

        old_timeout = ser.timeout
        ser.timeout = 2.0
        ser.write(b"MCAP")
        ser.flush()
        try:
            cap = read_exact(ser, 16)
        except Exception as exc:
            raise RuntimeError(
                "MCAP failed: flash the updated CUTE v2 H/M micro:bit firmware first"
            ) from exc
        finally:
            ser.timeout = old_timeout

        if cap[:4] != b"MCAP":
            raise RuntimeError(f"unexpected MCAP reply: {cap!r}")

        max_version, min_h, max_h, max_m = cap[4:8]
        slot_bytes = struct.unpack_from("<I", cap, 8)[0]
        device_arch = struct.unpack_from("<I", cap, 12)[0]
        print(
            f"Device: CUTE<=v{max_version} H={min_h}..{max_h} M<={max_m} "
            f"slot={slot_bytes} arch=0x{device_arch:08X}"
        )

        if package["version"] > max_version:
            raise RuntimeError("package format is newer than firmware")
        if package["arch_id"] != device_arch:
            raise RuntimeError("package/device architecture mismatch")
        if not min_h <= package["H"] <= max_h:
            raise RuntimeError("device does not support package H")
        if package["M"] > max_m:
            raise RuntimeError("device does not support package M")
        if len(blob) > slot_bytes:
            raise RuntimeError("package does not fit device slot")

        ser.write(b"MBEG")
        ser.write(struct.pack("<I", len(blob)))
        ser.flush()

        reply = read_exact(ser, 8)
        if reply[:4] == b"MERR":
            raise RuntimeError(f"BEGIN failed rc={struct.unpack('<i', reply[4:8])[0]}")
        if reply[:4] != b"MRED":
            raise RuntimeError(f"BEGIN failed: {reply!r}")

        slot = chr(reply[4])
        print("Target slot:", slot)

        sent = 0
        while sent < len(blob):
            part = blob[sent:sent + args.chunk]
            ser.write(b"MDAT")
            ser.write(struct.pack("<H", len(part)))
            ser.write(part)
            ser.flush()

            ack = read_exact(ser, 8)
            if ack[:4] == b"MERR":
                rc = struct.unpack("<i", ack[4:8])[0]
                raise RuntimeError(f"DATA failed at {sent} rc={rc}")
            if ack[:4] != b"MACK":
                raise RuntimeError(f"DATA failed at {sent}: {ack!r}")

            received = struct.unpack("<I", ack[4:8])[0]
            sent += len(part)
            if received != sent:
                raise RuntimeError(f"offset mismatch: device={received}, host={sent}")

            if sent == len(blob) or sent % 4096 < len(part):
                print(f"  {sent}/{len(blob)} ({100*sent/len(blob):.1f}%)")

        ser.write(b"MEND")
        ser.flush()
        reply = read_exact(ser, 8)
        if reply[:4] != b"MACT":
            if reply[:4] == b"MERR":
                rc = struct.unpack("<i", reply[4:8])[0]
                raise RuntimeError(f"END/validation failed rc={rc}")
            raise RuntimeError(f"unexpected END reply: {reply!r}")

        active = chr(reply[4])
        print("ACTIVE slot:", active)

        ser.write(b"MIN2")
        ser.flush()
        info = read_exact(ser, 40)
        if info[:4] != b"MIN2":
            raise RuntimeError(f"unexpected MIN2 reply: {info!r}")

        info_slot = chr(info[4]) if info[4] else "?"
        version = info[5]
        h = info[6]
        m = info[7]
        layers = struct.unpack_from("<H", info, 8)[0]
        flags = struct.unpack_from("<H", info, 10)[0]
        model_bytes = struct.unpack_from("<I", info, 12)[0]
        label = info[16:40].split(b"\0", 1)[0].decode("utf-8", errors="replace")

        expected = (
            active,
            package["version"], package["H"], package["M"],
            package["layer_count"], package["flags"], package["total_bytes"],
            package["label"],
        )
        actual = (info_slot, version, h, m, layers, flags, model_bytes, label)
        if actual != expected:
            raise RuntimeError(f"MIN2 verification mismatch\nactual={actual}\nexpected={expected}")

        print(
            f"Verified: slot={info_slot} CUTE v{version} H{h}/M{m} "
            f"layers={layers} bytes={model_bytes} label={label!r}"
        )
        print("SUCCESS: package is validated and active from raw flash.")


if __name__ == "__main__":
    main()
