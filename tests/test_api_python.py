import binascii
import os
import pty
import struct
import sys

from unittest.mock import patch

sys.path.append(os.path.join(os.path.dirname(__file__), os.path.pardir, "api"))

from pyeese import Connection
from pyeese.connection import PacketInType
from pyeese.cobs import encode


def _get_connection():
    master_fd, slave_fd = pty.openpty()
    slave_path = os.readlink(f"/proc/self/fd/{slave_fd}")
    return master_fd, Connection(tty=slave_path)

def _send_packet(fd: int, type_: PacketInType, payload: bytes = b""):
    header = struct.pack(
        Connection.HEADER_STRUCT,
        Connection.PROTOCOL_VERSION,
        type_.value
    )
    crc = binascii.crc32(header + payload)
    packet = header + payload + crc.to_bytes(4)
    enc = encode(packet)
    enc += (0).to_bytes(1)
    os.write(fd, enc)

@patch('pyeese.Connection._handle_nop')
def test_connection(handle_nop):
    master_fd, conn = _get_connection()
    conn.send_nop()
    conn.iterate()
    _send_packet(master_fd, PacketInType.NOP)
    conn.iterate()
    handle_nop.assert_called_once()

def test_measurements():
    master_fd, conn = _get_connection()
    temperature = 21.5
    relative_humidity = 49.5
    payload = struct.pack(
        Connection.MEASUREMENTS_STRUCT,
        int(temperature * 100.),
        int(relative_humidity * 100.),
    )
    _send_packet(master_fd, PacketInType.MEASUREMENTS, payload)
    conn.iterate()
    conn_temperature = conn.temperature
    assert temperature == conn_temperature, f"Temperature is wrong ({temperature} != {conn_temperature})"
    conn_relative_humidity = conn.relative_humidity
    assert relative_humidity == conn_relative_humidity, f"Temperature is wrong ({relative_humidity} != {conn_relative_humidity})"
