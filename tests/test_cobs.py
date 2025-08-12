import os
import ctypes
import enum


class CobsEncCtx(ctypes.Structure):
    """
    typedef struct cobs_enc_ctx {
      void *dst;
      size_t dst_max;
      size_t cur;
      size_t code_idx;
      unsigned code;
      int need_advance;
    } cobs_enc_ctx_t;
    """
    _fields_ = [
        ("dst", ctypes.c_void_p),
        ("dst_max", ctypes.c_size_t),
        ("cur", ctypes.c_size_t),
        ("code_idx", ctypes.c_size_t),
        ("code", ctypes.c_uint),
        ("need_advance", ctypes.c_int),
    ]


class CobsDecodeIncCtx(ctypes.Structure):
    """
    typedef struct cobs_decode_inc_ctx {
      enum cobs_decode_inc_state {
        COBS_DECODE_READ_CODE,
        COBS_DECODE_RUN,
        COBS_DECODE_FINISH_RUN
      } state;
      uint8_t block, code;
    } cobs_decode_inc_ctx_t;
    """
    _fields_ = [
        ("state", ctypes.c_int),
        ("block", ctypes.c_uint8),
        ("code", ctypes.c_uint8),
    ]


class CobsDecodeIncArgs(ctypes.Structure):
    """
    typedef struct cobs_decode_inc_args {
      void const *enc_src;  // pointer to current position of encoded payload
      void *dec_dst;        // pointer to decoded output buffer.
      size_t enc_src_max;   // length of the |src| input buffer.
      size_t dec_dst_max;   // length of the |dst| output buffer.
    } cobs_decode_inc_args_t;
    """
    _fields_ = [
        ("enc_src", ctypes.c_void_p),
        ("dec_dst", ctypes.c_void_p),
        ("enc_src_max", ctypes.c_size_t),
        ("dec_dst_max", ctypes.c_size_t),
    ]


class CobsRet(enum.Enum):
    COBS_RET_SUCCESS = 0
    COBS_RET_ERR_BAD_ARG = 1
    COBS_RET_ERR_BAD_PAYLOAD = 2
    COBS_RET_ERR_EXHAUSTED = 3


COBS_TINYFRAME_SENTINEL_VALUE = 0x5A
COBS_BUFFER_LENGTH = 128


def test_cobs():
    path = os.path.join(os.getenv("BUILD_TESTS_DIR", "build/tests"), "cobs.so")
    lib_blob = ctypes.CDLL(path)
    assert lib_blob, f"Library missing at {path}"
    enc_ctx = CobsEncCtx()

    enc_data = (ctypes.c_char * COBS_BUFFER_LENGTH)()
    enc_data[0] = COBS_TINYFRAME_SENTINEL_VALUE
    enc_data[COBS_BUFFER_LENGTH-1] = COBS_TINYFRAME_SENTINEL_VALUE
    enc_data_ptr = ctypes.cast(enc_data, ctypes.c_void_p)
    ret = lib_blob.cobs_encode_inc_begin(enc_data_ptr, COBS_BUFFER_LENGTH, ctypes.pointer(enc_ctx))
    assert ret == CobsRet.COBS_RET_SUCCESS.value, f"Failed to start the encoding: {CobsRet(ret)}"

    first_str = b"hello there\n"
    w_data = ctypes.cast(ctypes.c_char_p(first_str), ctypes.c_void_p)
    ret = lib_blob.cobs_encode_inc(ctypes.pointer(enc_ctx), w_data, len(first_str))
    assert ret == CobsRet.COBS_RET_SUCCESS.value, f"Failed to add first packet: {CobsRet(ret)}"

    second_str = b"general grevious"
    w_data = ctypes.cast(ctypes.c_char_p(second_str), ctypes.c_void_p)
    ret = lib_blob.cobs_encode_inc(ctypes.pointer(enc_ctx), w_data, len(second_str))
    assert ret == CobsRet.COBS_RET_SUCCESS.value, f"Failed to add second packet: {CobsRet(ret)}"

    out_enc_len = ctypes.c_size_t()
    ret = lib_blob.cobs_encode_inc_end(ctypes.pointer(enc_ctx), ctypes.pointer(out_enc_len))
    assert ret == CobsRet.COBS_RET_SUCCESS.value, f"Failed to finish encoding: {CobsRet(ret)}"

    dec_data = (ctypes.c_char * COBS_BUFFER_LENGTH)()
    dec_data[0] = COBS_TINYFRAME_SENTINEL_VALUE
    dec_data[COBS_BUFFER_LENGTH-1] = COBS_TINYFRAME_SENTINEL_VALUE
    dec_data_ptr = ctypes.cast(dec_data, ctypes.c_void_p)
    dec_ctx = CobsDecodeIncCtx()
    dec_args = CobsDecodeIncArgs(enc_data_ptr, dec_data_ptr, COBS_BUFFER_LENGTH, COBS_BUFFER_LENGTH)
    ret = lib_blob.cobs_decode_inc_begin(ctypes.pointer(dec_ctx))
    assert ret == CobsRet.COBS_RET_SUCCESS.value, f"Failed to start the decoding: {CobsRet(ret)}"

    out_enc_src_len = ctypes.c_size_t()
    out_dec_dst_len = ctypes.c_size_t()
    out_decode_complete = ctypes.c_bool()
    ret = lib_blob.cobs_decode_inc(
        ctypes.pointer(dec_ctx),
        ctypes.pointer(dec_args),
        ctypes.pointer(out_enc_src_len),
        ctypes.pointer(out_dec_dst_len),
        ctypes.pointer(out_decode_complete),
    )
    assert ret == CobsRet.COBS_RET_SUCCESS.value, f"Failed to decode: {CobsRet(ret)}"
    assert out_decode_complete, "Decoding incomplete"
    dec = first_str + second_str
    assert out_dec_dst_len.value == len(dec), f"Decoding length mismatch ({out_dec_dst_len.value} != {len(dec)})"
    assert dec_data.value == dec, f"Decoding mismatch ({dec_data.value} != {dec})"
