"""
Package for communicating with a device using a COBS-encoded serial protocol.

This package provides:
    - COBS (Consistent Overhead Byte Stuffing) encoding and decoding utilities.
    - A `Connection` class for managing serial communication with the device.
    - Enumerations for incoming and outgoing packet types.
    - A `connect()` helper for quickly establishing a connection.

Typical usage example:
    from mypackage import connect

    with connect("/dev/ttyACM0") as conn:
        conn.send_nop()
        conn.iterate()

Modules:
    cobs: Implements COBS encoding and decoding functions.
    connection: Handles serial communication, packet parsing, and message
        dispatch.
"""


__all__ = [
    "Connection",
    "connect",
]

from .connection import Connection, connect
