import time
import struct
import logging

import serial
import serial.tools.list_ports

################################################################################

# RPC Protocol
#
# RPC Request/Response Format
#
#     55 ab 55 cd CCCC MM RR LLLL <data>
#         CCCC    16-bit little endian checksum over method, length, return code, data
#           MM    8-bit method id
#           RR    8-bit signed return code (-128 for method not found, don't care for request)
#         LLLL    16-bit little endian length of data
#       <data>    data
#
# RPC Handler Prototype
#
#     int8_t foo(uint8_t *buf, uint16_t *len);
#

URPC_BUFFER_SIZE = 2048
URPC_SERIAL_BAUDRATE = 115200

class RPC_Exception(Exception): pass
class RPC_TimeoutException(RPC_Exception): pass
class RPC_LengthException(RPC_Exception): pass
class RPC_CRCException(RPC_Exception): pass
class RPC_MethodNotFoundException(RPC_Exception): pass

class RPC:
    CRC16_POLY  = 0x1021    # CRC-16-CCITT
    @staticmethod
    def _crc16(checksum, data):
        for i in range(len(data)):
            checksum = checksum ^ (data[i] << 8)
            for _ in range(8):
                if checksum & 0x8000:
                    checksum = (checksum << 1) ^ RPC.CRC16_POLY
                else:
                    checksum <<= 1
                checksum &= 0xffff
        return checksum

    def _rpc_read_response(self):
        # Find the header (shift in one at a time)
        data = "0000"
        while data != "\x55\xab\x55\xcd":
            c = self.ser.read(1)
            if len(c) < 1:
                raise RPC_TimeoutException()
            data = data[1:] + c

        # Read checksum, method, return code, length
        data = self.ser.read(6)
        if len(data) < 6:
            raise RPC_TimeoutException()

        # Unpack checksum, method, return code, length
        (checksum, method, retcode, length) = struct.unpack("<HBbH", data)

        logging.debug("found rpc response with checksum 0x%02x, method 0x%02x, retcode %d, length %u", checksum, method, retcode, length)

        # Validate length
        if length > URPC_BUFFER_SIZE:
            logging.debug("error, huge data length %u (URPC_BUFFER_SIZE is %u)", length, URPC_BUFFER_SIZE)
            raise RPC_LengthException()

        # Read data
        data = self.ser.read(length)
        if len(data) < length:
            raise RPC_TimeoutException()

        # Verify checksum
        calc_checksum = self._crc16(0, [method, retcode & 0xff, length & 0xff, (length >> 8) & 0xff])
        calc_checksum = self._crc16(calc_checksum, bytearray(data))
        if calc_checksum != checksum:
            logging.debug("invalid checksum. got 0x%04x, computed 0x%04x", checksum, calc_checksum)
            raise RPC_CRCException()

        return (method, retcode, data)

    def _rpc_write_request(self, method, data):
        # Compute checksum
        checksum = self._crc16(0, [method, 0x00, len(data) & 0xff, (len(data) >> 8) & 0xff])
        checksum = self._crc16(checksum, bytearray(data))

        logging.debug("writing rpc request with checksum 0x%02x, method %d, length %u", checksum, method, len(data))

        # Pack header, checksum, method, return code, length
        frame = struct.pack("<IHBBH", 0xcd55ab55, checksum, method, 0x00, len(data)) + data
        logging.debug("bytes on the wire: %s", ",".join(["%02x" % c for c in bytearray(frame)]))

        # Write request
        self.ser.write(frame)

    def __init__(self, timeout=10, max_retries=3, debug=False):
        self.timeout = timeout
        self.max_retries = max_retries
        if debug:
            logging.basicConfig(level=logging.DEBUG)

        # Find the FTDI cable
        ports = list(serial.tools.list_ports.grep("FTDI"))
        if len(ports) < 1:
            raise IOError("Could not find FTDI TTL cable!")

        # Open the serial port
        self.ser = serial.Serial(ports[0][0], URPC_SERIAL_BAUDRATE, timeout=self.timeout)

    def call(self, req_method, req_data=""):
        attempt = 0
        while attempt < self.max_retries:
            logging.debug("rpc attempt %d", attempt)
            try:
                # Send RPC request
                logging.debug("sending rpc request method 0x%02x, length %d", req_method, len(req_data))
                self._rpc_write_request(req_method, req_data)
                # Get RPC response
                (resp_method, resp_retcode, resp_data) = self._rpc_read_response()
                logging.debug("received rpc response method 0x%02x, retcode %d, length %u", resp_method, resp_retcode, len(resp_data))
            except RPC_Exception as e:
                logging.debug("RPC exception: %s", str(e))
                attempt += 1
                continue
            break

        # If all attempts failed
        if attempt == self.max_retries:
            raise RPC_TimeoutException("RPC retries exhausted")

        # RPC Method Not Found
        if resp_retcode == -128:
            raise RPC_MethodNotFoundException()
        # RPC Response Method doesn't match
        elif resp_method != req_method:
            raise RPC_Exception("mismatched response. got method 0x%02x, retcode %d, length %u" % (resp_method, resp_retcode, len(resp_data)))

        return (resp_retcode, resp_data)

