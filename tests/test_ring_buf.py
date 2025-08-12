import os
import ctypes


class RingBuf(ctypes.Structure):
    """
    typedef struct {
        volatile uint8_t* buf;
        uint32_t size;
        volatile uint32_t r_pos;
        volatile uint32_t w_pos;
    } ring_buf_t;
    """
    _fields_ = [
        ("buf", ctypes.POINTER(ctypes.c_uint8)),
        ("size", ctypes.c_uint32),
        ("r_pos", ctypes.c_uint32),
        ("w_pos", ctypes.c_uint32),
    ]

    @staticmethod
    def ring_buf_init(size: int = 128):
        buf = (ctypes.c_uint8 * size)()
        buf_ptr = ctypes.cast(buf, ctypes.POINTER(ctypes.c_uint8))
        return RingBuf(buf_ptr, size, 0, 0)


def test_ringbuf_read_write():
    path = os.path.join(os.getenv("BUILD_TESTS_DIR", "build/tests"), "ring_buf.so")
    lib_blob = ctypes.CDLL(path)
    assert lib_blob, f"Library missing at {path}"
    ring = RingBuf.ring_buf_init()
    w_data_str = b"hello world"
    w_data = ctypes.c_char_p(w_data_str)
    lib_blob.ring_buf_write(ctypes.pointer(ring), w_data, len(w_data_str))
    r_data = (ctypes.c_char * 128)()
    r_data_ptr = ctypes.cast(r_data, ctypes.POINTER(ctypes.c_uint8))
    to_read = lib_blob.ring_buf_read(ctypes.pointer(ring), r_data_ptr, 128)
    assert to_read == len(w_data_str), f"Wrong length to read returned ({to_read} != {len(w_data_str)})"
    assert w_data_str == r_data.value, f"Wrong text returned ({w_data_str} != {r_data.value})"
