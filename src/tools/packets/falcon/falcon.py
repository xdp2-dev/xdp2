import ctypes
from enum import Enum
from scapy.all import *

# Python helpers for Falcon protocol format definitions

FALCON_PROTO_VERSION = 1

class FalconPktType(Enum):
	FALCON_PKT_TYPE_PULL_REQUEST = 0
	FALCON_PKT_TYPE_PULL_DATA = 3
	FALCON_PKT_TYPE_PUSH_DATA = 5
	FALCON_PKT_TYPE_RESYNC = 6
	FALCON_PKT_TYPE_NACK = 8
	FALCON_PKT_TYPE_ACK = 9
	FALCON_PKT_TYPE_EACK = 10

class FalconProtocolType(Enum):
	FALCON_PROTO_TYPE_RDMA = 2
	FALCON_PROTO_TYPE_NVME = 3

class FalconResyncCode(Enum):
	FALCON_RESYNC_CODE_TARG_ULP_COMPLETE_IN_ERROR = 1
	FALCON_RESYNC_CODE_LOCAL_XLR_FLOW = 2
	FALCON_RESYNC_CODE_PKT_RETRANS_ERROR = 3
	FALCON_RESYNC_CODE_TRANSACTION_TIMEOUT = 4
	FALCON_RESYNC_CODE_REMOTE_XLR_FLOW = 5
	FALCON_RESYNC_CODE_TARG_ULP_NONRECOV_ERROR = 6
	FALCON_RESYNC_CODE_TARG_ULP_INVALID_CID_ERROR = 7

class FalconNackCode(Enum):
	FALCON_NACK_CODE_INSUFF_RESOURCES = 1
	FALCON_NACK_CODE_ULP_NOT_READY = 2
	FALCON_NACK_CODE_XLR_DROP = 4
	FALCON_NACK_CODE_ULP_COMPLETE_IN_ERROR = 6
	FALCON_NACK_CODE_ULP_NONRECOV_ERROR = 7
	FALCON_NACK_CODE_INVAILD_CID = 8

class FalconBaseHdr(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("version", ctypes.c_uint32, 4),
		("rsvd", ctypes.c_uint32, 4),
		("dest_cid", ctypes.c_uint32, 24),

		("dest_function", ctypes.c_uint32, 24),
		("protocol_type", ctypes.c_uint32, 3),
		("packet_type", ctypes.c_uint32, 4),
		("ack_req", ctypes.c_uint32, 1),

		("recv_data_psn", ctypes.c_uint32),
		("recv_req_psn", ctypes.c_uint32),
		("psn", ctypes.c_uint32),
		("req_seqno", ctypes.c_uint32),
	]

def FillBaseHdr(hdr, dest_cid, dest_function, packet_type, protocol_type,
		ack_req, recv_data_psn, recv_req_psn, psn, req_seqno):
	hdr.version = FALCON_PROTO_VERSION
	hdr.rsvd = 0
	hdr.dest_cid = dest_cid
	hdr.dest_function = dest_function
	hdr.packet_type = packet_type
	hdr.protocol_type = protocol_type
	hdr.ack_req = ack_req
	hdr.recv_data_psn = recv_data_psn
	hdr.recv_req_psn = recv_req_psn
	hdr.psn = psn
	hdr.req_seqno = req_seqno

class FalconPullReq(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("base_hdr", FalconBaseHdr),

		("rsvd1", ctypes.c_uint16),
		("req_length", ctypes.c_uint16),
		("rsvd2", ctypes.c_uint32),
	]

def MakePullReq(dest_cid, dest_function, protocol_type, ack_req,
		recv_data_psn, recv_req_psn, psn, req_seqno, req_length):
	hdr = FalconPullReq()

	FillBaseHdr(hdr.base_hdr, dest_cid, dest_function,
		    FalconPktType.FALCON_PKT_TYPE_PULL_REQUEST.value,
		    protocol_type, ack_req, recv_data_psn, recv_req_psn, psn,
		    req_seqno)

	hdr.rsvd1 = 0
	hdr.req_length = req_length
	hdr.rsvd2 = 0

	return hdr

class FalconPullData(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("base_hdr", FalconBaseHdr),
	]

def MakePullData(dest_cid, dest_function, protocol_type, ack_req,
		recv_data_psn, recv_req_psn, psn, req_seqno):
	hdr = FalconPullData()

	FillBaseHdr(hdr.base_hdr, dest_cid, dest_function,
		    FalconPktType.FALCON_PKT_TYPE_PULL_DATA.value,
		    protocol_type, ack_req, recv_data_psn, recv_req_psn, psn,
		    req_seqno)

	return hdr

class FalconPushData(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("base_hdr", FalconBaseHdr),

		("rsvd", ctypes.c_uint16),
		("req_length", ctypes.c_uint16),
	]

def MakePushData(dest_cid, dest_function, protocol_type, ack_req,
		recv_data_psn, recv_req_psn, psn, req_seqno, req_length):
	hdr = FalconPushData()

	FillBaseHdr(hdr.base_hdr, dest_cid, dest_function,
		    FalconPktType.FALCON_PKT_TYPE_PUSH_DATA.value,
		    protocol_type, ack_req, recv_data_psn, recv_req_psn, psn,
		    req_seqno)

	hdr.rsvd = 0
	hdr.req_length = req_length

	return hdr

class FalconResync(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("base_hdr", FalconBaseHdr),

		("resync_code", ctypes.c_uint32, 8),
		("resync_pkt_type", ctypes.c_uint32, 4),
		("rsvd", ctypes.c_uint32, 20),
		("vendor_defined", ctypes.c_uint32),
	]

def MakeResync(dest_cid, dest_function, protocol_type, ack_req,
	       recv_data_psn, recv_req_psn, psn, req_seqno,
	       resync_code, resync_pkt_type, vendor_defined):
	hdr = FalconResync()

	FillBaseHdr(hdr.base_hdr, dest_cid, dest_function,
		    FalconPktType.FALCON_PKT_TYPE_RESYNC.value,
		    protocol_type, ack_req, recv_data_psn, recv_req_psn, psn,
		    req_seqno)

	hdr.resync_code = resync_code
	hdr.resync_pkt_type = resync_pkt_type
	hdr.rsvd = 0
	hdr.vendor_defined = vendor_defined

	return hdr

class FalconBaseAck(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("version", ctypes.c_uint32, 4),
		("rsvd1", ctypes.c_uint32, 4),
		("cnx_id", ctypes.c_uint32, 24),

		("rsvd2", ctypes.c_uint32, 27),
		("pkt_type", ctypes.c_uint32, 4),
		("rsvd3", ctypes.c_uint32, 1),

		("recv_data_wind_seqno", ctypes.c_uint32),
		("recv_req_wind_seqno", ctypes.c_uint32),
		("timestamp_t1", ctypes.c_uint32),
		("timestamp_t2", ctypes.c_uint32),

		("hop_count", ctypes.c_uint32, 4),
		("rx_buffer_occ", ctypes.c_uint32, 5),
		("ecn_rx_pkt_cnt", ctypes.c_uint32, 14),
		("rsvd4", ctypes.c_uint32, 9),

		("rsvd5", ctypes.c_uint32, 9),
		("rue_info", ctypes.c_uint32, 21),
		("oo_wind_notify", ctypes.c_uint32, 2),
	]

def FillBaseAck(hdr, cnx_id, pkt_type, recv_data_wind_seqno,
		recv_req_wind_seqno, timestamp_t1, timestamp_t2, hop_count,
		rx_buffer_occ, ecn_rx_pkt_cnt, rue_info, oo_wind_notify):
	hdr.version = FALCON_PROTO_VERSION
	hdr.rsvd1 = 0
	hdr.cnx_id = cnx_id

	hdr.rsvd2 = 0
	hdr.pkt_type = pkt_type
	hdr.rsvd3 = 0

	hdr.recv_data_wind_seqno = recv_data_wind_seqno
	hdr.recv_req_wind_seqno = recv_req_wind_seqno
	hdr.timestamp_t1 = timestamp_t1
	hdr.timestamp_t2 = timestamp_t2

	hdr.hop_count = hop_count
	hdr.rx_buffer_occ = rx_buffer_occ
	hdr.ecn_rx_pkt_cnt = ecn_rx_pkt_cnt
	hdr.rsvd4 = 0

	hdr.rsvd5 = 0
	hdr.rue_info = rue_info
	hdr.oo_wind_notify = oo_wind_notify

def MakeAck(cnx_id, recv_data_wind_seqno, recv_req_wind_seqno, timestamp_t1,
	    timestamp_t2, hop_count, rx_buffer_occ, ecn_rx_pkt_cnt, rue_info,
	    oo_wind_notify):
	hdr = FalconBaseAck()

	FillBaseAck(hdr, cnx_id, FalconPktType.FALCON_PKT_TYPE_ACK.value,
		    recv_data_wind_seqno, recv_req_wind_seqno, timestamp_t1,
		    timestamp_t2, hop_count, rx_buffer_occ, ecn_rx_pkt_cnt,
		    rue_info, oo_wind_notify)

	return hdr

class FalconEack(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("base_ack", FalconBaseAck),

		("rcv_data_seqno_ack_bitmap_1", ctypes.c_uint64),
		("rcv_data_seqno_ack_bitmap_2", ctypes.c_uint64),
		("rcv_req_seqno_ack_bitmap_1", ctypes.c_uint64),
		("rcv_req_seqno_ack_bitmap_2", ctypes.c_uint64),
		("rcv_req_seqno_bitmap", ctypes.c_uint64),
	]

def MakeEack(cnx_id, recv_data_wind_seqno, recv_req_wind_seqno, timestamp_t1,
	     timestamp_t2, hop_count, rx_buffer_occ, ecn_rx_pkt_cnt, rue_info,
	     oo_wind_notify, rcv_data_seqno_ack_bitmap_1,
	     rcv_data_seqno_ack_bitmap_2, rcv_req_seqno_ack_bitmap_1,
	     rcv_req_seqno_ack_bitmap_2, rcv_req_seqno_bitmap):
	hdr = FalconEack()

	FillBaseAck(hdr.base_ack, cnx_id,
		    FalconPktType.FALCON_PKT_TYPE_EACK.value,
		    recv_data_wind_seqno, recv_req_wind_seqno, timestamp_t1,
		    timestamp_t2, hop_count, rx_buffer_occ, ecn_rx_pkt_cnt,
		    rue_info, oo_wind_notify)

	hdr.rcv_data_seqno_ack_bitmap_1 = rcv_data_seqno_ack_bitmap_1
	hdr.rcv_data_seqno_ack_bitmap_2 = rcv_data_seqno_ack_bitmap_2
	hdr.rcv_req_seqno_ack_bitmap_1 = rcv_req_seqno_ack_bitmap_1
	hdr.rcv_req_seqno_ack_bitmap_2 = rcv_req_seqno_ack_bitmap_2
	hdr.rcv_req_seqno_bitmap = rcv_req_seqno_bitmap

	return hdr

class FalconNack(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("version", ctypes.c_uint32, 4),
		("rsvd1", ctypes.c_uint32, 4),
		("cnx_id", ctypes.c_uint32, 24),

		("rsvd2", ctypes.c_uint32, 27),
		("pkt_type", ctypes.c_uint32, 4),
		("rsvd3", ctypes.c_uint32, 1),

		("recv_data_wind_seqno", ctypes.c_uint32),
		("recv_req_wind_seqno", ctypes.c_uint32),
		("timestamp_t1", ctypes.c_uint32),
		("timestamp_t2", ctypes.c_uint32),

		("hop_count", ctypes.c_uint32, 4),
		("rx_buffer_occ", ctypes.c_uint32, 5),
		("ecn_rx_pkt_cnt", ctypes.c_uint32, 14),
		("rsvd4", ctypes.c_uint32, 9),

		("rsvd5", ctypes.c_uint32, 9),
		("rue_info", ctypes.c_uint32, 23),

		("nack_psn", ctypes.c_uint32),

		("nack_code", ctypes.c_uint8),

		("rsvd6", ctypes.c_uint8, 3),
		("rnr_nack_timeout", ctypes.c_uint8, 5),

		("window", ctypes.c_uint8, 1),
		("rsvd7", ctypes.c_uint8, 7),

		("ulp_nack_code", ctypes.c_uint8),
	]

def MakeNack(cnx_id, recv_data_wind_seqno,
	    recv_req_wind_seqno, timestamp_t1, timestamp_t2, hop_count,
	    rx_buffer_occ, ecn_rx_pkt_cnt, rue_info, nack_psn, nack_code,
	    rnr_nack_timeout, window, ulp_nack_code):
	hdr = FalconNack()

	hdr.version = FALCON_PROTO_VERSION
	hdr.rsvd1 = 0
	hdr.cnx_id = cnx_id

	hdr.rsvd2 = 0
	hdr.pkt_type = FalconPktType.FALCON_PKT_TYPE_NACK.value
	hdr.rsvd3 = 0

	hdr.recv_data_wind_seqno = recv_data_wind_seqno
	hdr.recv_req_wind_seqno = recv_req_wind_seqno
	hdr.timestamp_t1 = timestamp_t1
	hdr.timestamp_t2 = timestamp_t2

	hdr.hop_count = hop_count
	hdr.rx_buffer_occ = rx_buffer_occ
	hdr.ecn_rx_pkt_cnt = ecn_rx_pkt_cnt
	hdr.rsvd4 = 0

	hdr.rsvd5 = 0
	hdr.rue_info = rue_info

	hdr.nack_psn = nack_psn

	hdr.nack_code = nack_code

	hdr.rsvd6 = 0
	hdr.rnr_nack_timeout = rnr_nack_timeout

	hdr.window = window
	hdr.rsvd7 = 0

	hdr.ulp_nack_code = ulp_nack_code

	return hdr
