import ctypes
from enum import Enum

# Python helpers for UET-SES protocol format definitions

class UetSesRequestMsgType(Enum):
	UET_SES_REQUEST_MSG_NO_OP = 0x0
	UET_SES_REQUEST_MSG_WRITE = 0x1
	UET_SES_REQUEST_MSG_READ = 0x2
	UET_SES_REQUEST_MSG_ATOMIC = 0x3
	UET_SES_REQUEST_MSG_FETCHING_ATOMIC = 0x4
	UET_SES_REQUEST_MSG_SEND = 0x5
	UET_SES_REQUEST_MSG_RENDEZVOUS_SEND = 0x6
	UET_SES_REQUEST_MSG_DATAGRAM_SEND = 0x7
	UET_SES_REQUEST_MSG_DEFERRABLE_SEND = 0x8
	UET_SES_REQUEST_MSG_TAGGED_SEND = 0x9
	UET_SES_REQUEST_MSG_RENDEZVOUS_TSEND = 0xa
	UET_SES_REQUEST_MSG_DEFERRABLE_TSEND = 0xb
	UET_SES_REQUEST_MSG_READY_TO_RESTART = 0xc
	UET_SES_REQUEST_MSG_TSEND_ATOMIC = 0xd
	UET_SES_REQUEST_MSG_TSEND_FETCH_ATOMIC = 0xe
	UET_SES_REQUEST_MSG_MSG_ERROR = 0xf

	UET_SES_REQUEST_MSG_VENDOR_DEFINED_0 = 0x30
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_1 = 0x31
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_2 = 0x32
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_3 = 0x33
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_4 = 0x34
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_5 = 0x35
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_6 = 0x36
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_7 = 0x37
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_8 = 0x38
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_9 = 0x39
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_10 = 0x3a
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_11 = 0x3b
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_12 = 0x3c
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_13 = 0x3d
	UET_SES_REQUEST_MSG_VENDOR_DEFINED_14 = 0x3e

	UET_SES_REQUEST_MSG_EXTENDED = 0x3f

class UetSesResponseMsgType(Enum):
	UET_SES_RESPONSE_DEFAULT = 0x0
	UET_SES_RESPONSE = 0x1
	UET_SES_RESPONSE_WITH_DATA = 0x2
	UET_SES_RESPONSE_NONE = 0x3

	UET_SES_REPONSE_MSG_VENDOR_DEFINED_0 = 0x30
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_1 = 0x31
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_2 = 0x32
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_3 = 0x33
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_4 = 0x34
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_5 = 0x35
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_6 = 0x36
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_7 = 0x37
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_8 = 0x38
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_9 = 0x39
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_10 = 0x3a
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_11 = 0x3b
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_12 = 0x3c
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_13 = 0x3d
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_14 = 0x3e
	UET_SES_REPONSE_MSG_VENDOR_DEFINED_15 = 0x3f

class UetSesReturnCode(Enum):
	UET_SES_RETURN_CODE_NULL = 0x0
	UET_SES_RETURN_CODE_OK = 0x1
	UET_SES_RETURN_CODE_BAD_GENERATION = 0x2
	UET_SES_RETURN_CODE_DISABLED = 0x3
	UET_SES_RETURN_CODE_DISABLED_GEN = 0x4
	UET_SES_RETURN_CODE_NO_MATCH = 0x5
	UET_SES_RETURN_CODE_UNSUPPORTED_OP = 0x6
	UET_SES_RETURN_CODE_UNSUPPORTED_SIZE = 0x7
	UET_SES_RETURN_CODE_AT_INVALID = 0x8
	UET_SES_RETURN_CODE_AT_PERM = 0x9
	UET_SES_RETURN_CODE_AT_ATS_ERROR = 0xa
	UET_SES_RETURN_CODE_AT_NO_TRANS = 0xb
	UET_SES_RETURN_CODE_AT_OUT_OF_RANGE = 0xc
	UET_SES_RETURN_CODE_HOST_POISONED = 0xd
	UET_SES_RETURN_CODE_HOST_UNSUCCESS_CMPL = 0xe
	UET_SES_RETURN_CODE_AMO_UNSUPPORTED_OP = 0xf
	UET_SES_RETURN_CODE_AMO_UNSUPPORTED_DT = 0x10
	UET_SES_RETURN_CODE_AMO_UNSUPPORTED_SIZE = 0x11
	UET_SES_RETURN_CODE_AMO_UNALIGNED = 0x12
	UET_SES_RETURN_CODE_AMO_FP_NAN = 0x13
	UET_SES_RETURN_CODE_AMO_FP_UNDERFLOW = 0x14
	UET_SES_RETURN_CODE_AMO_FP_OVERFLOW = 0x15
	UET_SES_RETURN_CODE_AMO_FP_INEXACT = 0x16
	UET_SES_RETURN_CODE_PERM_VIOLATION = 0x17
	UET_SES_RETURN_CODE_OP_VIOLATION = 0x18
	UET_SES_RETURN_CODE_BAD_INDEX = 0x19
	UET_SES_RETURN_CODE_BAD_PID = 0x1a
	UET_SES_RETURN_CODE_BAD_JOB_ID = 0x1b
	UET_SES_RETURN_CODE_BAD_MKEY = 0x1c
	UET_SES_RETURN_CODE_BAD_ADDR = 0x1d
	UET_SES_RETURN_CODE_CANCELLED = 0x1e
	UET_SES_RETURN_CODE_UNDELIVERABLE = 0x1f
	UET_SES_RETURN_CODE_UNCOR = 0x20
	UET_SES_RETURN_CODE_UNCOR_TRNSNT = 0x21
	UET_SES_RETURN_CODE_TOO_LONG = 0x22
	UET_SES_RETURN_CODE_INITIATOR_ERROR = 0x23
	UET_SES_RETURN_CODE_DROPPED = 0x24

	UET_SES_RETURN_CODE_VENDOR_DEFINED_0 = 0x30
	UET_SES_RETURN_CODE_VENDOR_DEFINED_1 = 0x31
	UET_SES_RETURN_CODE_VENDOR_DEFINED_2 = 0x32
	UET_SES_RETURN_CODE_VENDOR_DEFINED_3 = 0x33
	UET_SES_RETURN_CODE_VENDOR_DEFINED_4 = 0x34
	UET_SES_RETURN_CODE_VENDOR_DEFINED_5 = 0x35
	UET_SES_RETURN_CODE_VENDOR_DEFINED_6 = 0x36
	UET_SES_RETURN_CODE_VENDOR_DEFINED_7 = 0x37

	UET_SES_RETURN_CODE_EXTENDED = 0x3e

class UetSesAtomicDatatype(Enum):
	UET_SES_AMO_DATATYPE_INT8 = 0x0
	UET_SES_AMO_DATATYPE_UINT8 = 0x1
	UET_SES_AMO_DATATYPE_INT16 = 0x2
	UET_SES_AMO_DATATYPE_UINT16 = 0x3
	UET_SES_AMO_DATATYPE_INT32 = 0x4
	UET_SES_AMO_DATATYPE_UINT32 = 0x5
	UET_SES_AMO_DATATYPE_INT64 = 0x6
	UET_SES_AMO_DATATYPE_UINT64 = 0x7
	UET_SES_AMO_DATATYPE_INT128 = 0x8
	UET_SES_AMO_DATATYPE_UINT128 = 0x9
	UET_SES_AMO_DATATYPE_FLOAT = 0xa
	UET_SES_AMO_DATATYPE_DOUBLE = 0xb
	UET_SES_AMO_DATATYPE_FLOAT_COMPLEX = 0xc
	UET_SES_AMO_DATATYPE_DOUBLE_COMPLEX = 0xd
	UET_SES_AMO_DATATYPE_LONG_DOUBLE = 0xe
	UET_SES_AMO_DATATYPE_LONG_DOUBLE_COMPLEX = 0xf
	UET_SES_AMO_DATATYPE_BF16 = 0x10
	UET_SES_AMO_DATATYPE_FP16 = 0x11

	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_0 = 0xe0
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_1 = 0xe1
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_2 = 0xe2
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_3 = 0xe3
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_4 = 0xe4
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_5 = 0xe5
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_6 = 0xe6
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_7 = 0xe7
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_8 = 0xe8
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_9 = 0xe9
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_10 = 0xea
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_11 = 0xeb
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_12 = 0xec
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_13 = 0xed
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_14 = 0xee
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_15 = 0xef
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_16 = 0xf0
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_17 = 0xf1
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_18 = 0xf2
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_19 = 0xf3
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_20 = 0xf4
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_21 = 0xf5
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_22 = 0xf6
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_23 = 0xf7
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_24 = 0xf8
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_25 = 0xf9
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_26 = 0xfa
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_27 = 0xfb
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_28 = 0xfc
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_29 = 0xfd
	UET_SES_AMO_DATATYPE_VENDOR_DEFINED_30 = 0xfe

class UetSesAtomicOpcode(Enum):
    UET_SES_AMO_MIN = 0x0
    UET_SES_AMO_MAX = 0x1
    UET_SES_AMO_SUM = 0x2
    UET_SES_AMO_DIFF = 0x3
    UET_SES_AMO_PROD = 0x4
    UET_SES_AMO_LOR  = 0x5
    UET_SES_AMO_LAND = 0x6
    UET_SES_AMO_BOR = 0x7
    UET_SES_AMO_BAND = 0x8
    UET_SES_AMO_LXOR = 0x9
    UET_SES_AMO_BXOR = 0xa
    UET_SES_AMO_READ = 0xb
    UET_SES_AMO_WRITE = 0xc
    UET_SES_AMO_CSWAP = 0xd
    UET_SES_AMO_CSWAP_NE = 0x0e
    UET_SES_AMO_CSWAP_LE = 0xf
    UET_SES_AMO_CSWAP_LT = 0x10
    UET_SES_AMO_CSWAP_GE = 0x11
    UET_SES_AMO_CSWAP_GT = 0x12
    UET_SES_AMO_MSWAP = 0x13
    UET_SES_AMO_INVAL = 0x14

    UET_SES_AMO_VENDOR_DEFINED_0 = 0xe0
    UET_SES_AMO_VENDOR_DEFINED_1 = 0xe1
    UET_SES_AMO_VENDOR_DEFINED_2 = 0xe2
    UET_SES_AMO_VENDOR_DEFINED_3 = 0xe3
    UET_SES_AMO_VENDOR_DEFINED_4 = 0xe4
    UET_SES_AMO_VENDOR_DEFINED_5 = 0xe5
    UET_SES_AMO_VENDOR_DEFINED_6 = 0xe6
    UET_SES_AMO_VENDOR_DEFINED_7 = 0xe7
    UET_SES_AMO_VENDOR_DEFINED_8 = 0xe8
    UET_SES_AMO_VENDOR_DEFINED_9 = 0xe9
    UET_SES_AMO_VENDOR_DEFINED_10 = 0xea
    UET_SES_AMO_VENDOR_DEFINED_11 = 0xeb
    UET_SES_AMO_VENDOR_DEFINED_12 = 0xec
    UET_SES_AMO_VENDOR_DEFINED_13 = 0xed
    UET_SES_AMO_VENDOR_DEFINED_14 = 0xee
    UET_SES_AMO_VENDOR_DEFINED_15 = 0xef
    UET_SES_AMO_VENDOR_DEFINED_16 = 0xf0
    UET_SES_AMO_VENDOR_DEFINED_17 = 0xf1
    UET_SES_AMO_VENDOR_DEFINED_18 = 0xf2
    UET_SES_AMO_VENDOR_DEFINED_19 = 0xf3
    UET_SES_AMO_VENDOR_DEFINED_20 = 0xf4
    UET_SES_AMO_VENDOR_DEFINED_21 = 0xf5
    UET_SES_AMO_VENDOR_DEFINED_22 = 0xf6
    UET_SES_AMO_VENDOR_DEFINED_23 = 0xf7
    UET_SES_AMO_VENDOR_DEFINED_24 = 0xf8
    UET_SES_AMO_VENDOR_DEFINED_25 = 0xf9
    UET_SES_AMO_VENDOR_DEFINED_26 = 0xfa
    UET_SES_AMO_VENDOR_DEFINED_27 = 0xfb
    UET_SES_AMO_VENDOR_DEFINED_28 = 0xfc
    UET_SES_AMO_VENDOR_DEFINED_29 = 0xfd
    UET_SES_AMO_VENDOR_DEFINED_30 = 0xfe

# Standard headers

class UetSesCommonHdr(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("rsvd1", ctypes.c_uint8, 2),
		("opcode", ctypes.c_uint8, 6),

		("version", ctypes.c_uint8, 2),
		("delivery_complete", ctypes.c_uint8, 1),
		("initiator_error", ctypes.c_uint8, 1),
		("relative_addressing", ctypes.c_uint8, 1),
		("hdr_data_present", ctypes.c_uint8, 1),
		("end_of_msg", ctypes.c_uint8, 1),
		("start_of_msg", ctypes.c_uint8, 1),

		("message_id", ctypes.c_uint16),
		("ri_generation", ctypes.c_uint32, 8),
		("job_id",ctypes.c_uint32, 24),

		("rsvd2", ctypes.c_uint16, 4),
		("pid_on_fep", ctypes.c_uint16, 12),

		("rsvd3", ctypes.c_uint16, 4),
		("resource_index", ctypes.c_uint16, 12),
	]

def FillCommonHdr(hdr, opcode, delivery_complete, initiator_error,
		     relative_addressing, hdr_data_present, end_of_msg,
		     start_of_msg, message_id, ri_generation, job_id,
		     pid_on_fep, resource_index):
	hdr.opcode = opcode
	hdr.version = 0
	hdr.delivery_complete = delivery_complete
	hdr.initiator_error = initiator_error
	hdr.relative_addressing = relative_addressing
	hdr.hdr_data_present = hdr_data_present
	hdr.end_of_msg = end_of_msg
	hdr.start_of_msg = start_of_msg
	hdr.message_id = message_id
	hdr.ri_generation = ri_generation
	hdr.job_id = job_id
	hdr.pid_on_fep = pid_on_fep
	hdr.resource_index = resource_index

class UetSesRequestSomHdrStd(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("cmn", UetSesCommonHdr),

		("buffer_offset", ctypes.c_uint64),
		("initiator", ctypes.c_uint32),
		("memory_key_match_bits", ctypes.c_uint64),

		("header_data", ctypes.c_uint64),
		("request_length", ctypes.c_uint32),
	]

def MakeRequestSomHdrStd(opcode, delivery_complete, initiator_error,
		      relative_addressing, hdr_data_present, end_of_msg,
		      message_id, ri_generation, job_id, pid_on_fep,
		      resource_index, buffer_offset, initiator,
		      memory_key_match_bits, header_data, request_length):
	hdr = UetSesRequestSomHdrStd()

	FillCommonHdr(hdr.cmn, opcode, delivery_complete, initiator_error,
			 relative_addressing, hdr_data_present, end_of_msg,
			 1, message_id, ri_generation, job_id, pid_on_fep,
			 resource_index)

	hdr.buffer_offset = buffer_offset
	hdr.initiator = initiator
	hdr.memory_key_match_bits = memory_key_match_bits
	hdr.header_data = header_data
	hdr.request_length = request_length

	return hdr

class UetSesRequestNoSomHdrStd(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("cmn", UetSesCommonHdr),

		("buffer_offset", ctypes.c_uint64),
		("initiator", ctypes.c_uint32),
		("memory_key_match_bits", ctypes.c_uint64),

		("rsvd1", ctypes.c_uint16),

		("rsvd2", ctypes.c_uint16, 2),
		("payload_length", ctypes.c_uint16, 14),

		("message_offset", ctypes.c_uint32),
		("request_length", ctypes.c_uint32),
	]

def MakeRequestNoSomHdrStd(opcode, delivery_complete, initiator_error,
			relative_addressing, hdr_data_present, end_of_msg,
			message_id, ri_generation, job_id, pid_on_fep,
			resource_index, buffer_offset, initiator,
			memory_key_match_bits, payload_length, message_offset,
			request_length):
	hdr = UetSesRequestNoSomHdrStd()

	FillCommonHdr(hdr.cmn, opcode, delivery_complete, initiator_error,
			 relative_addressing, hdr_data_present, end_of_msg,
			 0, message_id, ri_generation, job_id, pid_on_fep,
			 resource_index)

	hdr.buffer_offset = buffer_offset
	hdr.initiator = initiator
	hdr.memory_key_match_bits = memory_key_match_bits
	hdr.payload_length = payload_length
	hdr.message_offset = message_offset
	hdr.request_length = request_length

	return hdr

class UetSesDeferSendHdrStd(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("cmn", UetSesCommonHdr),

		("initiator_restart_token", ctypes.c_uint32),
		("target_restart_token", ctypes.c_uint32),
		("initiator", ctypes.c_uint32),
		("match_bits", ctypes.c_uint64),
		("header_data", ctypes.c_uint64),
		("request_length", ctypes.c_uint32),
	]

def MakeDeferSendHdrStd(opcode, delivery_complete, initiator_error,
		     relative_addressing, hdr_data_present, end_of_msg,
		     start_of_msg, message_id, ri_generation, job_id,
		     pid_on_fep, resource_index, initiator_restart_token,
		     target_restart_token, initiator, match_bits, header_data,
		     request_length):
	hdr = UetSesDeferSendHdrStd()

	FillCommonHdr(hdr.cmn, opcode, delivery_complete, initiator_error,
			 relative_addressing, hdr_data_present, end_of_msg,
			 start_of_msg, message_id, ri_generation, job_id,
			 pid_on_fep, resource_index)

	hdr.initiator_restart_token = initiator_restart_token
	hdr.target_restart_token = target_restart_token
	hdr.initiator = initiator
	hdr.match_bits = match_bits
	hdr.header_data = header_data
	hdr.request_length = request_length

	return hdr

class UetSesReadyToRestartHdrStd(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("cmn", UetSesCommonHdr),

		("buffer_offset", ctypes.c_uint64),
		("initiator", ctypes.c_uint32),
		("initiator_restart_token", ctypes.c_uint32),
		("target_restart_token", ctypes.c_uint32),
		("header_data", ctypes.c_uint64),
		("request_length", ctypes.c_uint32)
	]

def MakeReadyToRestartHdrStd(opcode, delivery_complete, initiator_error,
			  relative_addressing, hdr_data_present, end_of_msg,
			  start_of_msg, message_id, ri_generation, job_id,
			  pid_on_fep, resource_index, buffer_offset,
			  initiator, initiator_restart_token,
			  target_restart_token, header_data, request_length):
	hdr = UetSesReadyToRestartHdrStd()

	FillCommonHdr(hdr.cmn, opcode, delivery_complete, initiator_error,
			 relative_addressing, hdr_data_present, end_of_msg,
			 start_of_msg, message_id, ri_generation, job_id,
			 pid_on_fep, resource_index)

	hdr.buffer_offset = buffer_offset
	hdr.initiator = initiator
	hdr.initiator_restart_token = initiator_restart_token
	hdr.target_restart_token = target_restart_token
	hdr.header_data = header_data
	hdr.request_length = request_length

	return hdr

# Medium headers

class UetSesRequestHdrMedium(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("cmn", UetSesCommonHdr),

        ("header_data", ctypes.c_uint64),
		("initiator", ctypes.c_uint32),
        ("memory_key", ctypes.c_uint64)
   ]

def MakeRequestHdrMedium(opcode, delivery_complete, initiator_error,
		      relative_addressing, hdr_data_present, end_of_msg, start_of_msg,
		      message_id, ri_generation, job_id, pid_on_fep,
                        resource_index, header_data, initiator, memory_key):

    hdr = UetSesRequestHdrMedium()

    FillCommonHdr(hdr.cmn, opcode, delivery_complete, initiator_error,
			 relative_addressing, hdr_data_present, end_of_msg,
			 start_of_msg, message_id, ri_generation, job_id,
			 pid_on_fep, resource_index)

    hdr.header_data = header_data
    hdr.initiator = initiator
    hdr.memory_key = memory_key

    return hdr

# Small headers

class UetSesMessageHdrSmall(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("cmn", UetSesCommonHdr),

        ("buffer_offset", ctypes.c_uint64),
	]

def MakeRequestHdrSmall(opcode, delivery_complete, initiator_error,
			relative_addressing, hdr_data_present,
			end_of_msg, start_of_msg, message_id, ri_generation,
                        job_id, pid_on_fep, resource_index, buffer_offset):
	hdr = UetSesMessageHdrSmall()

	FillCommonHdr(hdr.cmn, opcode, delivery_complete, initiator_error,
			 relative_addressing, hdr_data_present, end_of_msg,
			 start_of_msg, message_id, ri_generation, job_id,
			 pid_on_fep, resource_index)

	hdr.buffer_offset = buffer_offset

	return hdr

# Rendezvous extension header

class UetSesRendezvousExtHdr(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("eager_length", ctypes.c_uint32),

		("ri_generation", ctypes.c_uint32, 8),
		("pid_on_fep", ctypes.c_uint32, 12),
		("resource_index", ctypes.c_uint32, 12)
	]

def MakeRendezvousExtHdr(eager_length, ri_generation, pid_on_fep,
			 resource_index):
	hdr = UetSesRendezvousExtHdr()

	hdr.eager_length = eager_length
	hdr.ri_generation = ri_generation
	hdr.pid_on_fep = pid_on_fep
	hdr.resource_index = resource_index

	return hdr

# Atomic extension headers

class UetSesAtomicOpExtHdr(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("atomic_code", ctypes.c_uint8),
		("atomic_datatype", ctypes.c_uint8),

		("ctrl_cacheable", ctypes.c_uint8, 1),
		("cpu_coherent", ctypes.c_uint8, 1),
		("rsvd1", ctypes.c_uint8, 3),
		("vendor_defined", ctypes.c_uint8, 3),

		("rsvd2", ctypes.c_uint8),
	]

def MakeAtomicOpExtHdr(atomic_code, atomic_datatype, ctrl_cacheable,
                       cpu_coherent, vendor_defined):
    hdr = UetSesAtomicOpExtHdr()

    hdr.atomic_code = atomic_code
    hdr.atomic_datatype = atomic_datatype
    hdr.cpu_coherent = cpu_coherent
    hdr.ctrl_cacheable = ctrl_cacheable
    hdr.vendor_defined = vendor_defined


    return hdr

class UetSesAtomicCmpAndSwapHdr(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("atomic_code", ctypes.c_uint8),
		("atomic_datatype", ctypes.c_uint8),

		("ctrl_cacheable", ctypes.c_uint8, 1),
		("cpu_coherent", ctypes.c_uint8, 1),
		("rsvd1", ctypes.c_uint8, 3),
		("vendor_defined", ctypes.c_uint8, 3),

		("rsvd2", ctypes.c_uint8),

		("compare_value1", ctypes.c_uint64),
		("compare_value2", ctypes.c_uint64),
		("swap_value1", ctypes.c_uint64),
		("swap_value2", ctypes.c_uint64)
	]

def MakeAtomicCmpAndSwapHdr(atomic_code, atomic_datatype, ctrl_cacheable,
                            cpu_coherent, vendor_defined, compare_value1,
                            compare_value2, swap_value1, swap_value2):
    hdr = UetSesAtomicCmpAndSwapHdr()

    hdr.atomic_code = atomic_code
    hdr.atomic_datatype = atomic_datatype
    hdr.cpu_coherent = cpu_coherent
    hdr.ctrl_cacheable = ctrl_cacheable
    hdr.vendor_defined = vendor_defined

    hdr.compare_value1 = compare_value1
    hdr.compare_value2 = compare_value2
    hdr.swap_value1 = swap_value1
    hdr.swap_value2 = swap_value2

    return hdr

# Response headers

class UetSesHdrResponse(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("list", ctypes.c_uint8, 2),
		("opcode", ctypes.c_uint8, 6),

		("version", ctypes.c_uint8, 2),
		("return_code", ctypes.c_uint8, 6),

		("message_id", ctypes.c_uint16),

		("ri_generation", ctypes.c_uint32, 8),
		("job_id", ctypes.c_uint32, 24),

		("modified_length", ctypes.c_uint32),
	]

def MakeHdrResponse(list, opcode, return_code, message_id, ri_generation,
		    job_id, modified_length):
	hdr = UetSesHdrResponse()

	hdr.list = list
	hdr.opcode = opcode
	hdr.version = 0
	hdr.return_code = return_code

	hdr.message_id = message_id
	hdr.ri_generation = ri_generation
	hdr.job_id = job_id

	hdr.modified_length = modified_length

	return hdr

class UetSesResponseWithDataHdr(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("list", ctypes.c_uint8, 2),
		("opcode", ctypes.c_uint8, 6),

		("version", ctypes.c_uint8, 2),
		("return_code", ctypes.c_uint8, 6),

# Want to do a single job_id: 24 bitfield but that doesn't seem to work.
# I suppose there's an issue with packing in this case

		("message_id", ctypes.c_uint16),

		("rsdv1", ctypes.c_uint8),
		("job_id1", ctypes.c_uint8),
		("job_id2", ctypes.c_uint16),

		("read_request_message_id", ctypes.c_uint16),

		("rsvd2", ctypes.c_uint16, 4),
		("payload_length", ctypes.c_uint16, 12),

		("modified_length", ctypes.c_uint32),
		("message_offset", ctypes.c_uint32)
	]

def MakeWithDataHdrResponse(list, opcode, return_code, message_id,
			    job_id, read_request_message_id, payload_length,
			    modified_length, message_offset):
	hdr = UetSesResponseWithDataHdr()

	hdr.list = list
	hdr.opcode = opcode
	hdr.version = 0
	hdr.return_code = return_code

	hdr.message_id = message_id
	hdr.job_id1 = job_id >> 16
	hdr.job_id2 = job_id & 0xffff
	hdr.read_request_message_id = read_request_message_id
	hdr.payload_length = payload_length

	hdr.modified_length = modified_length
	hdr.message_offset = message_offset

	return hdr

class UetSesResponseWithDataSmallHdr(ctypes.BigEndianStructure):
        _pack_ = 1
        _fields_ = [
		("list", ctypes.c_uint8, 2),
		("opcode", ctypes.c_uint8, 6),

		("version", ctypes.c_uint8, 2),
		("return_code", ctypes.c_uint8, 6),

		("rsdv1", ctypes.c_uint8, 2),
		("payload_length", ctypes.c_uint16, 14),

		("rsdv2", ctypes.c_uint8),
		("job_id1", ctypes.c_uint8),
		("job_id2", ctypes.c_uint16),

		("original_request_psn", ctypes.c_uint32),
    ]

def MakeWithDataHdrSmallResponse(list, opcode, return_code,
                payload_length, job_id, original_request_psn):
	hdr = UetSesResponseWithDataSmallHdr()

	hdr.list = list
	hdr.opcode = opcode
	hdr.version = 0
	hdr.return_code = return_code

	hdr.payload_length = payload_length
	hdr.job_id1 = job_id >> 16
	hdr.job_id2 = job_id & 0xffff

	hdr.original_request_psn = original_request_psn

	return hdr
