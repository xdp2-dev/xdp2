import ctypes
from enum import Enum
from scapy.all import *

# Python helpers for SUPERp protocol format definitions

class SUPERp_Op_Type(Enum):
    SUPERP_OP_TYPE_NOOP = 0
    SUPERP_OP_TYPE_LAST_NULL = 1
    SUPERP_OP_TYPE_TRANSACT_ERR = 2
    SUPERP_OP_TYPE_READ = 8
    SUPERP_OP_TYPE_WRITE = 9
    SUPERP_OP_TYPE_READ_RESP = 10
    SUPERP_OP_TYPE_SEND = 11
    SUPERP_OP_TYPE_SEND_TO_QP = 12

class PacketDeliveryHeader(ctypes.LittleEndianStructure):
	_pack_ = 1
	_fields_ = [
		("dcid", ctypes.c_uint16),
		("rwin", ctypes.c_uint16),
		("psn", ctypes.c_uint32),
		("ack_psn", ctypes.c_uint32),
		("sack_bitmap", ctypes.c_uint32),
	]

def MakePacketDeliveryHdr(dcid, rwin, psn, ack_psn, sack_bitmap):
    hdr = PacketDeliveryHeader()

    hdr.dcid = dcid
    hdr.rwin = rwin
    hdr.psn = psn
    hdr.ack_psn = ack_psn
    hdr.sack_bitmap = sack_bitmap

    return hdr

class TransactionHeader(ctypes.LittleEndianStructure):
	_pack_ = 1
	_fields_ = [
        ("num_ops", ctypes.c_uint8, 4),
        ("rsvd", ctypes.c_uint8, 3),
        ("eom", ctypes.c_uint8, 1),

        ("opcode", ctypes.c_uint8),

		("seqno", ctypes.c_uint16),
		("xid", ctypes.c_uint16),
		("ack_xid", ctypes.c_uint16),
	]

def MakeTransactionHdr(eom, num_ops, opcode, seqno, xid, ack_xid):
    hdr = TransactionHeader()

    hdr.eom = eom
    hdr.num_ops = num_ops
    hdr.opcode = opcode
    hdr.seqno = seqno
    hdr.xid = xid
    hdr.ack_xid = ack_xid

    return hdr

class ReadOp(ctypes.LittleEndianStructure):
	_pack_ = 1
	_fields_ = [
        ("addr", ctypes.c_uint64),
        ("length", ctypes.c_uint32),
    ]

def MakeReadOp(addr, length):
    hdr = ReadOp()

    hdr.addr = addr
    hdr.length = length

    return hdr

class WriteOp(ctypes.LittleEndianStructure):
	_pack_ = 1
	_fields_ = [
        ("addr", ctypes.c_uint64),
    ]

def MakeWriteOp(addr):
    hdr = WriteOp()

    hdr.addr = addr

    return hdr

class ReadResp(ctypes.LittleEndianStructure):
	_pack_ = 1
	_fields_ = [
        ("seqno", ctypes.c_uint16),
        ("op_num", ctypes.c_uint8),
        ("rsvd", ctypes.c_uint8),
        ("offset", ctypes.c_uint32),
    ]

def MakeReadResp(seqno, op_num, offset):
    hdr = ReadResp()

    hdr.seqno = seqno
    hdr.op_num = op_num
    hdr.offset = offset

    return hdr

class TransactErr(ctypes.LittleEndianStructure):
	_pack_ = 1
	_fields_ = [
        ("seqno", ctypes.c_uint16),
        ("op_num", ctypes.c_uint8),
        ("rsvd", ctypes.c_uint8),
        ("major_err_code", ctypes.c_uint16),
        ("minor_err_code", ctypes.c_uint16),
    ]

def MakeTransactErr(seqno, op_num, major_err_code, minor_err_code):
    hdr = TransactErr()

    hdr.seqno = seqno
    hdr.op_num = op_num
    hdr.major_err_code = major_err_code
    hdr.minor_err_code = minor_err_code

    return hdr

class SendOp(ctypes.LittleEndianStructure):
	_pack_ = 1
	_fields_ = [
        ("key", ctypes.c_uint32),
        ("offset", ctypes.c_uint32),
    ]

def MakeSendOp(addr, length):
    hdr = SendOp()

    hdr.key = key
    hdr.offset = offset

    return hdr

class SendToQPOp(ctypes.LittleEndianStructure):
	_pack_ = 1
	_fields_ = [
        ("queue_pair", ctypes.c_uint32, 24),
        ("rsvd", ctypes.c_uint32, 8),
    ]

def MakeSendToQPOp(queue_pair):
    hdr = SendToQPOp()

    hdr.queue_pair = queue_pair

    return hdr


