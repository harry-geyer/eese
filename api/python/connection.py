"""
Serial connection and packet handling for a COBS-encoded binary protocol.

This module provides:
- Enumerations for incoming (`PacketInType`) and outgoing (`PacketOutType`)
  packet types.
- A `Connection` class for managing a serial link to the device, sending
  packets, receiving packets, and dispatching them to handler methods.
- A `connect()` helper function for quickly creating a `Connection` instance.

Protocol details:
    Each packet is structured as:
        [protocol_version: uint32]
        [type: uint8]
        [payload: bytes]
        [CRC32: uint32]
    Packets are COBS-encoded for transmission.

Intended usage:
    with connect("/dev/ttyACM0") as conn:
        conn.iterate()

Requires:
    - `pyserial` for serial communication
    - `cobs` module for encoding/decoding packets
"""
import binascii
import enum
import logging
import select
import struct

import serial

from .cobs import encode, decode


class PacketInType(enum.Enum):
    """Enumeration of packet types received from the device."""
    NOP = 1
    MEASUREMENTS = 2
    HEALTH = 3
    EVENT = 4


class PacketOutType(enum.Enum):
    """Enumeration of packet types sent to the device."""
    NOP = 1
    RESET = 2


class Connection:
    """
    Handles serial communication with a device using a COBS-based packet
    protocol.

    Packets follow the structure:
        [protocol_version: uint8]
        [type: uint8]
        [payload: bytes]
        [CRC32: uint32]

    The packet is then COBS-encoded before transmission.
    """
    HEADER_STRUCT = "<BB"
    PROTOCOL_VERSION = 1
    MEASUREMENTS_STRUCT = "<ii"

    def __init__(self, tty: str = "/dev/ttyACM0"):
        self._serial = serial.Serial(
            port=tty,
            baudrate=115200,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=None,
            xonxoff=False,
            rtscts=False,
            write_timeout=None,
            dsrdtr=False,
            inter_byte_timeout=None,
            exclusive=None,
        )
        self._leftovers = b""
        self._temperature = None
        self._relative_humidity = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def __del__(self):
        self.close()

    def close(self) -> None:
        """Close the serial connection and clear any buffered data."""
        self._serial.flush()
        if self._serial is not None:
            self._leftovers = b""
            self._serial.close()
            self._serial = None

    def _send_message(self, type_: PacketOutType, payload: bytes) -> None:
        header = struct.pack(
            Connection.HEADER_STRUCT, Connection.PROTOCOL_VERSION, type_.value,
        )
        crc = binascii.crc32(header + payload)
        packet = header + payload + crc.to_bytes(4)
        enc = encode(packet)
        enc += (0).to_bytes(1)
        self._serial.write(enc)

    def send_nop(self) -> None:
        """Send a no-operation packet to the device."""
        self._send_message(PacketOutType.NOP, b"")

    def send_reset(self) -> None:
        """Send a reset packet to the device."""
        self._send_message(PacketOutType.RESET, b"")

    def _parse_message(self, message: bytes) -> None:
        message_size = len(message)
        logging.debug("Message in (%d): %s", message_size, list(message))
        header_size = struct.calcsize(Connection.HEADER_STRUCT)
        crc_size = 4  # CRC32 so 4 bytes
        min_size = header_size + crc_size
        if message_size < min_size:
            logging.error(
                "Packet length is shorter than header: %d < %d",
                message_size, min_size,
            )
            return

        crc = binascii.crc32(message[:-crc_size])
        if crc.to_bytes(4) != message[-crc_size:]:
            logging.error("CRC32 check failed: %08X", crc)
            return

        version, type_ = struct.unpack(
            Connection.HEADER_STRUCT,
            message[:header_size],
        )

        if version != Connection.PROTOCOL_VERSION:
            logging.error(
                "Wrong protocol version: %s != %s",
                version, Connection.PROTOCOL_VERSION,
            )
            return

        try:
            type_ = PacketInType(type_)
        except ValueError:
            logging.error("Received unknown type: %d", type_)
            return

        payload = message[header_size:-crc_size]

        function_name = "_handle_" + type_.name.lower()
        try:
            function = getattr(self, function_name)
        except AttributeError:
            logging.error("No handler function for %s", type_.name)
            return

        function(payload)

    def _handle_nop(self, payload):
        logging.info("Received NOP message")

    def _handle_measurements(self, payload):
        logging.info("Received MEASUREMENTS message")
        temperature, relative_humidity = struct.unpack(
            self.MEASUREMENTS_STRUCT,
            payload,
        )
        self._temperature = float(temperature) / 100.
        self._relative_humidity = float(relative_humidity) / 100.

    def _handle_health(self, payload):
        logging.info("Received HEALTH message")

    def _handle_event(self, payload):
        logging.info("Received EVENT message")

    def _parse_leftovers(self) -> None:
        index = self._leftovers.find(b"\x00")
        while index > 0:
            enc = self._leftovers[:index]
            self._leftovers = self._leftovers[index+1:]
            dec = decode(enc)
            self._parse_message(dec)
            index = self._leftovers.find(b"\x00")

    def iterate(self, timeout: float = 0.25) -> None:
        """
        Poll the serial port for incoming data and process any complete
        packets.

        Args:
            timeout: Timeout in seconds for waiting on serial data.
        """
        rs, *_ = select.select([self._serial], [], [], timeout)
        for r in rs:
            if r is self._serial:
                self._leftovers += self._serial.read(self._serial.in_waiting)
                self._parse_leftovers()

    @property
    def temperature(self):
        """
        float: The current temperature measurement.

        Returns:
            The most recent temperature value, in degrees Celsius.
        """
        return self._temperature

    @property
    def relative_humidity(self):
        """
        float: The current relative humidity measurement.

        Returns:
            The most recent relative humidity value, expressed as a percentage.
        """
        return self._relative_humidity


def connect(tty: str = "/dev/ttyACM0"):
    """
    Create and return a new `Connection` instance.

    Args:
        tty: Path to the serial device.

    Returns:
        Connection: A new connection object for communicating with the device.
    """
    return Connection(tty=tty)
