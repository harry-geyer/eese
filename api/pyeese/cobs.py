"""
COBS (Consistent Overhead Byte Stuffing) encoding and decoding utilities.

This module implements a Python version of the COBS algorithm, which encodes
data to eliminate zero bytes and makes packet framing simpler for byte-oriented
protocols.

Functions:
    encode(in_bytes): Encode a byte sequence into COBS format.
    decode(in_bytes): Decode a COBS-encoded byte sequence back to raw bytes.

Classes:
    CobsDecodeError: Raised when COBS decoding fails due to invalid input.

Notes:
    - Input must be a one-dimensional buffer of bytes (e.g., bytes, bytearray).
    - Strings (str) are not supported and must be encoded to bytes before use.
    - Based on the COBS specification by Stuart Cheshire and Mary Baker.
"""


class CobsDecodeError(Exception):
    """Raised when a COBS decoding operation encounters invalid data."""


def _get_buffer_view(in_bytes):
    mv = memoryview(in_bytes)
    if mv.ndim > 1 or mv.itemsize > 1:
        raise BufferError('object must be a single-dimension buffer of bytes.')
    try:
        mv = mv.cast('c')
    except AttributeError:
        pass
    return mv


def encode(in_bytes):
    """
    Encode a bytes-like object using COBS.

    Args:
        in_bytes: Bytes-like object to encode. Must not be a string.

    Raises:
        TypeError: If the input is a str instead of bytes.

    Returns:
        bytes: The COBS-encoded representation of the input.
    """
    if isinstance(in_bytes, str):
        raise TypeError('Unicode-objects must be encoded as bytes first')
    in_bytes_mv = _get_buffer_view(in_bytes)
    final_zero = True
    out_bytes = bytearray()
    idx = 0
    search_start_idx = 0
    for in_char in in_bytes_mv:
        if in_char == b'\x00':
            final_zero = True
            out_bytes.append(idx - search_start_idx + 1)
            out_bytes += in_bytes_mv[search_start_idx:idx]
            search_start_idx = idx + 1
        else:
            if idx - search_start_idx == 0xFD:
                final_zero = False
                out_bytes.append(0xFF)
                out_bytes += in_bytes_mv[search_start_idx:idx+1]
                search_start_idx = idx + 1
        idx += 1
    if idx != search_start_idx or final_zero:
        out_bytes.append(idx - search_start_idx + 1)
        out_bytes += in_bytes_mv[search_start_idx:idx]
    return bytes(out_bytes)


def decode(in_bytes):
    """
    Decode a COBS-encoded bytes-like object.

    Args:
        in_bytes: COBS-encoded bytes-like object. Must not be a string.

    Raises:
        TypeError: If the input is a str.
        CobsDecodeError: If the data contains invalid length codes or embedded
            zeros.

    Returns:
        bytes: The decoded byte sequence.
    """
    if isinstance(in_bytes, str):
        raise TypeError("Unicode-objects are not supported; byte buffer"
                        + "objects only")
    in_bytes_mv = _get_buffer_view(in_bytes)
    out_bytes = bytearray()
    idx = 0

    if len(in_bytes_mv) > 0:
        while True:
            length = ord(in_bytes_mv[idx])
            if length == 0:
                raise CobsDecodeError("zero byte found in input")
            idx += 1
            end = idx + length - 1
            copy_mv = in_bytes_mv[idx:end]
            if b'\x00' in copy_mv:
                raise CobsDecodeError("zero byte found in input")
            out_bytes += copy_mv
            idx = end
            if idx > len(in_bytes_mv):
                raise CobsDecodeError("not enough input bytes for length code")
            if idx < len(in_bytes_mv):
                if length < 0xFF:
                    out_bytes.append(0)
            else:
                break
    return bytes(out_bytes)
