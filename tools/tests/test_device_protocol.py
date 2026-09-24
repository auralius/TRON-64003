import struct
import unittest

from device_protocol import detect, model_info


class FakeSerial:
    def __init__(self, responses):
        self.responses = bytearray(b"".join(responses))
        self.writes = bytearray()

    def write(self, data):
        self.writes.extend(data)

    def flush(self):
        pass

    def reset_input_buffer(self):
        pass

    def read(self, n):
        data = self.responses[:min(n, 3)]
        del self.responses[:len(data)]
        return bytes(data)


class DeviceProtocolTest(unittest.TestCase):
    def test_model_info_reports_active_topology(self):
        info = bytearray(40)
        info[:8] = b"MIN2A\x02\x06\x01"
        struct.pack_into("<HHI", info, 8, 33, 1, 26128)
        info[16:20] = b"face"
        ser = FakeSerial([info])
        self.assertEqual(model_info(ser)["H"], 6)
        self.assertEqual(ser.writes, b"MIN2")

    def test_detect_transfers_exact_gray_frame_and_reads_timing(self):
        rec = bytes([204, 10, 20, 40, 50])
        response = b"DTOK" + bytes([1, 0, 0, 0]) + rec
        timing = b"STOK" + struct.pack("<7I", 1, 100, 30, 900, 15, 20, 1065)
        ser = FakeSerial([response, timing])
        frame = bytes([127]) * (128 * 128)
        detections, stats = detect(ser, frame)
        self.assertEqual(ser.writes, b"CUTE" + frame + b"STAT")
        self.assertEqual(detections[0]["x1"], 10)
        self.assertEqual(stats["inference_ms"], 900)

    def test_detect_rejects_wrong_frame_size(self):
        with self.assertRaises(ValueError):
            detect(FakeSerial([]), b"bad")


if __name__ == "__main__":
    unittest.main()
