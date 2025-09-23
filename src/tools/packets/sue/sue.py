import ctypes
from enum import Enum
from scapy.all import *

# Python helpers for SUE protocol format definitions

SUE_PROTO_VERSION = 0

class SueOpcode(Enum):
    SUE_OPCODE_ACK = 0
    SUE_OPCODE_NACK = 1
    SUE_OPCODE_INVALID1 = 2
    SUE_OPCODE_INVALID2 = 3

class SueHdr(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("version", ctypes.c_uint16, 2),
		("opcode", ctypes.c_uint16, 2),
		("rsvd1", ctypes.c_uint16, 2),
		("xpuid", ctypes.c_uint16, 10),

		("psn", ctypes.c_uint16),

		("vc", ctypes.c_uint16, 2),
		("rsvd2", ctypes.c_uint16, 4),
		("partition", ctypes.c_uint16, 10),

		("apsn", ctypes.c_uint16),

	]

def MakeSueRhdr(opcode, xpuid, psn, vc, partition, apsn):
    hdr = SueHdr()

    hdr.version = SUE_PROTO_VERSION
    hdr.opcode = opcode
    hdr.rsvd1 = 0
    hdr.xpuid = xpuid
    hdr.psn = psn
    hdr.vc = vc
    hdr.rsvd2 = 0
    hdr.partition = partition
    hdr.apsn = apsn

    return hdr
