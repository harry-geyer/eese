import os
import ctypes


def test_crc32():
    path = os.path.join(os.getenv("BUILD_TESTS_DIR", "build/tests"), "crc32.so")
    lib_blob = ctypes.CDLL(path)
    assert lib_blob, f"Library missing at {path}"
    data_str = b"hello world"
    data = ctypes.cast(data_str, ctypes.POINTER(ctypes.c_uint8))
    crc = ctypes.c_uint32(lib_blob.crc32(data, len(data_str), 0xFFFFFFFF))
    assert 0x0 != crc, "CRC most likely should NOT be 0x00"
    data_str = data_str + crc
    data = ctypes.cast(data_str, ctypes.POINTER(ctypes.c_uint8))
    crc = lib_blob.crc32(data, len(data_str), 0xFFFFFFFF)
    assert 0x0 == crc, f"When packet includes CRC, calculated CRC should be 0 (crc:{crc})"
