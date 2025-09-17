import ctypes
from enum import Enum

# Python helpers for UET-PDS protocol format definitions

class UetPdsPktType(Enum):
	UET_PDS_TYPE_TSS = 1
	UET_PDS_TYPE_RUD_REQ = 2
	UET_PDS_TYPE_ROD_REQ = 3
	UET_PDS_TYPE_RUDI_REQ = 4
	UET_PDS_TYPE_RUDI_RESP = 5
	UET_PDS_TYPE_UUD_REQ = 6
	UET_PDS_TYPE_ACK = 7
	UET_PDS_TYPE_ACK_CC = 8
	UET_PDS_TYPE_ACK_CCX = 9
	UET_PDS_TYPE_NACK = 10
	UET_PDS_TYPE_CONTROL = 11
	UET_PDS_TYPE_NACK_CCX = 12
	UET_PDS_TYPE_RUD_CC_REQ = 13
	UET_PDS_TYPE_ROD_CC_REQ =14

class UetNextHeaderType(Enum):
	UET_HDR_NONE = 0x0
	UET_HDR_REQUEST_SMALL = 0x1
	UET_HDR_REQUEST_MEDIUM = 0x2
	UET_HDR_REQUEST_STD = 0x3
	UET_HDR_RESPONSE = 0x4
	UET_HDR_RESPONSE_DATA = 0x5
	UET_HDR_RESPONSE_DATA_SMALL = 0x6

# Common prologue header

class UetPdsPrologue(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("type", ctypes.c_uint16, 5),
		("next_hdr_ctrl", ctypes.c_uint16, 4),
		("flags", ctypes.c_uint16, 7),
	]

def FillPrologue(hdr, type, next_hdr_ctrl, flags):
	hdr.type = type
	hdr.next_hdr_ctrl = next_hdr_ctrl
	hdr.flags = flags

# RUD/ROD request packets

class UetPdsRudRodRequest(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("type", ctypes.c_uint16, 5),
		("next_hdr", ctypes.c_uint16, 4),
		("rsvd1", ctypes.c_uint16, 2),
		("retrans", ctypes.c_uint16, 1),
		("ackreq", ctypes.c_uint16, 1),
		("syn", ctypes.c_uint16, 1),
		("rsvd2", ctypes.c_uint16, 2),

		("clear_psn_offset", ctypes.c_uint16),
		("psn", ctypes.c_uint32),
		("spdcid", ctypes.c_uint16),

		("dpdcid", ctypes.c_uint16),
	]

class UetPdsRudRodRequestSyn(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("type", ctypes.c_uint16, 5),
		("next_hdr", ctypes.c_uint16, 4),
		("rsvd1", ctypes.c_uint16, 2),
		("retrans", ctypes.c_uint16, 1),
		("ackreq", ctypes.c_uint16, 1),
		("syn", ctypes.c_uint16, 1),
		("rsvd2", ctypes.c_uint16, 2),

		("clear_psn_offset", ctypes.c_uint16),
		("psn", ctypes.c_uint32),
		("spdcid", ctypes.c_uint16),

		("use_rsv_pdc", ctypes.c_uint16, 1),
		("rsvd3", ctypes.c_uint16, 3),
		("psn_offset", ctypes.c_uint16, 12),
	]

class UetPdsRudRodRequestCc(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("req", UetPdsRudRodRequest),
		("ccc_id", ctypes.c_uint32, 8),
		("credit_target", ctypes.c_uint32, 24),
	]

class UetPdsRudRodRequestSynCc(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("req", UetPdsRudRodRequestSyn),
		("ccc_id", ctypes.c_uint32, 8),
		("credit_target", ctypes.c_uint32, 24),
	]

def FillRudRodRequest(hdr, next_hdr, retrans, ackreq, syn, clear_psn_offset,
		      psn, spdcid):
	hdr.next_hdr = next_hdr
	hdr.rsvd1 = 0
	hdr.retrans = retrans
	hdr.ackreq = ackreq
	hdr.syn = syn
	hdr.rsvd2 = 0

	hdr.clear_psn_offset = clear_psn_offset
	hdr.psn = psn

	hdr.spdcid = spdcid

def MakeRodRequest(next_hdr, retrans, ackreq, clear_psn_offset, psn, spdcid,
		   dpdcid):
	hdr = UetPdsRudRodRequest()

	hdr.type = UetPdsPktType.UET_PDS_TYPE_ROD_REQ.value
	hdr.dpdcid = dpdcid

	FillRudRodRequest(hdr, next_hdr, retrans, ackreq, 0, clear_psn_offset,
			  psn, spdcid)

	return hdr

def MakeRudRequest(next_hdr, retrans, ackreq, clear_psn_offset, psn, spdcid,
		   dpdcid):
	hdr = UetPdsRudRodRequest()

	hdr.type = UetPdsPktType.UET_PDS_TYPE_RUD_REQ.value
	hdr.dpdcid = dpdcid

	FillRudRodRequest(hdr, next_hdr, retrans, ackreq, 0, clear_psn_offset,
			  psn, spdcid)

	return hdr

def MakeRodRequestSyn(next_hdr, retrans, ackreq, clear_psn_offset, psn, spdcid,
		      use_rsv_pdc, psn_offset):
	hdr = UetPdsRudRodRequestSyn()

	hdr.type = UetPdsPktType.UET_PDS_TYPE_ROD_REQ.value
	hdr.use_rsv_pdc = use_rsv_pdc
	hdr.psn_offset = psn_offset

	FillRudRodRequest(hdr, next_hdr, retrans, ackreq, 1, clear_psn_offset,
			  psn, spdcid)

	return hdr

def MakeRudRequestSyn(next_hdr, retrans, ackreq, clear_psn_offset, psn, spdcid,
		      use_rsv_pdc, psn_offset):
	hdr = UetPdsRudRodRequestSyn()

	hdr.type = UetPdsPktType.UET_PDS_TYPE_RUD_REQ.value
	hdr.use_rsv_pdc = use_rsv_pdc
	hdr.psn_offset = psn_offset

	FillRudRodRequest(hdr, next_hdr, retrans, ackreq, 1, clear_psn_offset,
			  psn, spdcid)

	return hdr

def MakeRodRequestCc(next_hdr, retrans, ackreq, clear_psn_offset, psn, spdcid,
		     dpdcid, ccc_id, credit_target):
	hdr = UetPdsRudRodRequestCc()

	hdr.req.type = UetPdsPktType.UET_PDS_TYPE_ROD_CC_REQ.value
	hdr.req.dpdcid = dpdcid
	hdr.ccc_id = ccc_id
	hdr.credit_target = credit_target

	FillRudRodRequest(hdr.req, next_hdr, retrans, ackreq, 0,
			  clear_psn_offset, psn, spdcid)

	return hdr

def MakeRudRequestCc(next_hdr, retrans, ackreq, clear_psn_offset, psn, spdcid,
		     dpdcid, ccc_id, credit_target):
	hdr = UetPdsRudRodRequestCc()

	hdr.req.type = UetPdsPktType.UET_PDS_TYPE_RUD_CC_REQ.value
	hdr.req.dpdcid = dpdcid
	hdr.ccc_id = ccc_id
	hdr.credit_target = credit_target

	FillRudRodRequest(hdr.req, next_hdr, retrans, ackreq, 0,
			  clear_psn_offset, psn, spdcid)

	return hdr

def MakeRodRequestSynCc(next_hdr, retrans, ackreq, clear_psn_offset, psn,
			spdcid, use_rsv_pdc, psn_offset, ccc_id, credit_target):
	hdr = UetPdsRudRodRequestSynCc()

	hdr.req.type = UetPdsPktType.UET_PDS_TYPE_ROD_CC_REQ.value
	hdr.req.use_rsv_pdc = use_rsv_pdc
	hdr.req.psn_offset = psn_offset
	hdr.ccc_id = ccc_id
	hdr.credit_target = credit_target

	FillRudRodRequest(hdr.req, next_hdr, retrans, ackreq, 1,
			  clear_psn_offset, psn, spdcid)

	return hdr

def MakeRudRequestSynCc(next_hdr, retrans, ackreq, clear_psn_offset, psn,
			spdcid, use_rsv_pdc, psn_offset, ccc_id, credit_target):
	hdr = UetPdsRudRodRequestSynCc()

	hdr.req.type = UetPdsPktType.UET_PDS_TYPE_RUD_CC_REQ.value
	hdr.req.use_rsv_pdc = use_rsv_pdc
	hdr.req.psn_offset = psn_offset
	hdr.ccc_id = ccc_id
	hdr.credit_target = credit_target

	FillRudRodRequest(hdr.req, next_hdr, retrans, ackreq, 1,
			  clear_psn_offset, psn, spdcid)

	return hdr

# ACK packets

class UetPdsAckRequestType(Enum):
	UET_PDS_ACK_REQUEST_NO_REQUEST = 0
	UET_PDS_ACK_REQUEST_CLEAR = 1
	UET_PDS_ACK_REQUEST_CLOSE = 2

class UetPdsAck(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("type", ctypes.c_uint16, 5),
		("next_hdr", ctypes.c_uint16, 4),
		("rsvd1", ctypes.c_uint16, 1),
		("ecn_marked", ctypes.c_uint16, 1),
		("retrans", ctypes.c_uint16, 1),
		("probe", ctypes.c_uint16, 1),
		("request", ctypes.c_uint16, 2),
		("rsvd2", ctypes.c_uint16, 1),

		("ack_psn_offset_probe_opaque", ctypes.c_uint16),
		("cack_psn", ctypes.c_uint32),
		("spdcid", ctypes.c_uint16),
		("dpdcid", ctypes.c_uint16),
	]
packets = []

# Congestion control types
class UetPdsCcType(Enum):
	UET_PDS_CC_TYPE_NSCC = 0
	UET_PDS_CC_TYPE_CREDIT = 1

class UetPdsAckCcNscc(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("ack", UetPdsAck),
		("cc_type", ctypes.c_uint8, 4),
		("cc_flags", ctypes.c_uint8, 4),
		("mpr", ctypes.c_uint8),
		("sack_psn_offset", ctypes.c_uint16),
		("sack_bitmap", ctypes.c_uint64),

		("service_time", ctypes.c_uint64, 16),
		("restore_cwnd", ctypes.c_uint64, 1),
		("rcv_cwnd_pend", ctypes.c_uint64, 7),
		("rcvd_bytes", ctypes.c_uint64, 24),
		("ooo_count", ctypes.c_uint64, 16),
	]

class UetPdsAckCcCredit(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("ack", UetPdsAck),
		("cc_type", ctypes.c_uint8, 4),
		("cc_flags", ctypes.c_uint8, 4),
		("mpr", ctypes.c_uint8),
		("sack_psn_offset", ctypes.c_uint16),
		("sack_bitmap", ctypes.c_uint64),

		("credit", ctypes.c_uint32, 24),
		("rsvd1", ctypes.c_uint32, 8),
		("rsvd2", ctypes.c_uint16),
		("ooo_count", ctypes.c_uint16),
	]

class UetPdsAckCcx(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("ack", UetPdsAck),
		("ccx_type", ctypes.c_uint8, 4),
		("cc_flags", ctypes.c_uint8, 4),
		("mpr", ctypes.c_uint8),
		("sack_psn_offset", ctypes.c_uint16),
		("sack_bitmap", ctypes.c_uint64),
		("ack_cc_state", ctypes.c_uint64),
	]

def FillAck(hdr, next_hdr, ecn_marked, retrans, probe, request, cack_psn,
	    ack_psn_offset_probe_opaque, spdcid, dpdcid):
	hdr.next_hdr = next_hdr
	hdr.ecn_marked = ecn_marked
	hdr.retrans = retrans
	hdr.probe = probe
	hdr.request = request
	hdr.cack_psn = cack_psn
	hdr.spdcid = spdcid
	hdr.dpdcid = dpdcid
	hdr.ack_psn_offset_probe_opaque = ack_psn_offset_probe_opaque

def MakeAck(next_hdr, ecn_marked, retrans, probe, request, cack_psn,
	    ack_psn_offset_probe_opaque, spdcid, dpdcid):
	hdr = UetPdsAck()

	hdr.type = UetPdsPktType.UET_PDS_TYPE_ACK.value

	FillAck(hdr, next_hdr, ecn_marked, retrans, probe, request, cack_psn,
		ack_psn_offset_probe_opaque, spdcid, dpdcid)

	return hdr

def MakeAckCcNscc(next_hdr, ecn_marked, retrans, probe, request, cack_psn,
		  ack_psn_offset_probe_opaque, spdcid, dpdcid, cc_flags, mpr,
		  sack_psn_offset, sack_bitmap, service_time, restore_cwnd,
          rcv_cwnd_pend, rcvd_bytes, ooo_count):

	hdr = UetPdsAckCcNscc()

	hdr.ack.type = UetPdsPktType.UET_PDS_TYPE_ACK_CC.value

	FillAck(hdr.ack, next_hdr, ecn_marked, retrans, probe, request,
		cack_psn, ack_psn_offset_probe_opaque, spdcid, dpdcid)

	hdr.cc_type = UetPdsCcType.UET_PDS_CC_TYPE_NSCC.value
	hdr.cc_flags = cc_flags
	hdr.mpr = mpr
	hdr.sack_psn_offset = sack_psn_offset
	hdr.sack_bitmap = sack_bitmap

	hdr.service_time = service_time
	hdr.restore_cwnd = restore_cwnd
	hdr.rcv_cwnd_pend = rcv_cwnd_pend
	hdr.rcvd_bytes = rcvd_bytes
	hdr.ooo_count = ooo_count

	return hdr

def MakeAckCcCredit(next_hdr, ecn_marked, retrans, probe, request, cack_psn,
		    ack_psn_offset_probe_opaque,
		    spdcid, dpdcid, cc_flags, mpr, sack_psn_offset,
		    sack_bitmap, credit, ooo_count):

	hdr = UetPdsAckCcCredit()

	hdr.ack.type = UetPdsPktType.UET_PDS_TYPE_ACK_CC.value

	FillAck(hdr.ack, next_hdr, ecn_marked, retrans, probe, request,
		cack_psn, ack_psn_offset_probe_opaque, spdcid, dpdcid)

	hdr.cc_type = UetPdsCcType.UET_PDS_CC_TYPE_CREDIT.value
	hdr.cc_flags = cc_flags
	hdr.mpr = mpr
	hdr.sack_psn_offset = sack_psn_offset
	hdr.sack_bitmap = sack_bitmap

	hdr.credit = credit
	hdr.ooo_count = ooo_count

	return hdr

def MakeAckCcx(next_hdr, ecn_marked, retrans, probe, request, cack_psn,
	           ack_psn_offset_probe_opaque, spdcid, dpdcid, mpr,
               sack_psn_offset, sack_bitmap, ccx_type, ack_cc_state):

	hdr = UetPdsAckCcx()

	hdr.ack.type = UetPdsPktType.UET_PDS_TYPE_ACK_CCX.value

	FillAck(hdr.ack, next_hdr, ecn_marked, retrans, probe, request,
		cack_psn, ack_psn_offset_probe_opaque, spdcid, dpdcid)

	hdr.mpr = mpr
	hdr.sack_psn_offset = sack_psn_offset
	hdr.sack_bitmap = sack_bitmap
	hdr.ccx_type = ccx_type
	hdr.ack_cc_state = ack_cc_state

	return hdr

# Control packets

class UetPdsAckControlPktType(Enum):
	UET_PDS_CTL_TYPE_NOOP = 0
	UET_PDS_CTL_TYPE_ACK_REQ = 1
	UET_PDS_CTL_TYPE_CLEAR_CMD = 2
	UET_PDS_CTL_TYPE_CLEAR_REQ = 3
	UET_PDS_CTL_TYPE_CLOSE_CMD = 4
	UET_PDS_CTL_TYPE_CLOSE_REQ = 5
	UET_PDS_CTL_TYPE_PROBE = 6
	UET_PDS_CTL_TYPE_CREDIT = 7
	UET_PDS_CTL_TYPE_CREDIT_REQ = 8
	UET_PDS_CTL_TYPE_NEGOTIATION = 9

class UetPdsControlPkt(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("type", ctypes.c_uint16, 5),
		("ctl_type", ctypes.c_uint16, 4),
		("rsvd1", ctypes.c_uint16, 1),
		("rsvd_isrod", ctypes.c_uint16, 1),
		("retrans", ctypes.c_uint16, 1),
		("ack_req", ctypes.c_uint16, 1),
		("syn", ctypes.c_uint16, 1),
		("rsvd2", ctypes.c_uint16, 2),

		("probe_opaque", ctypes.c_uint16),
		("psn", ctypes.c_uint32),
		("spdcid", ctypes.c_uint16),
		("dpdcid", ctypes.c_uint16),
	]

class UetPdsControlPktSyn(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("type", ctypes.c_uint16, 5),
		("ctl_type", ctypes.c_uint16, 4),
		("rsvd1", ctypes.c_uint16, 1),
		("rsvd_isrod", ctypes.c_uint16, 1),
		("retrans", ctypes.c_uint16, 1),
		("ack_req", ctypes.c_uint16, 1),
		("syn", ctypes.c_uint16, 1),
		("rsvd2", ctypes.c_uint16, 2),

		("probe_opaque", ctypes.c_uint16),
		("psn", ctypes.c_uint32),
		("spdcid", ctypes.c_uint16),

		("use_rsv_pdc", ctypes.c_uint16, 1),
		("rsvd3", ctypes.c_uint16, 3),
		("psn_offset", ctypes.c_uint16, 12),
	]

def MakeControlPkt(ctl_type, rsvd_isrod, retrans, ack_req, probe_opaque,
                   psn, spdcid, dpdcid):
	hdr = UetPdsControlPkt()

	hdr.type = UetPdsPktType.UET_PDS_TYPE_CONTROL.value

	hdr.ctl_type = ctl_type
	hdr.rsvd_isrod = rsvd_isrod
	hdr.retrans = retrans
	hdr.ack_req = ack_req
	hdr.syn = 0
	hdr.probe_opaque = probe_opaque
	hdr.psn = psn
	hdr.spdcid = spdcid

	hdr.dpdcid = dpdcid

	return hdr

def MakeControlPktSyn(ctl_type, rsvd_isrod, retrans, probe_opaque, psn,
                      spdcid, use_rsv_pdc, psn_offset):
	hdr = UetPdsControlPktSyn()

	hdr.type = UetPdsPktType.UET_PDS_TYPE_CONTROL.value

	hdr.ctl_type = ctl_type
	hdr.rsvd_isrod = rsvd_isrod
	hdr.retrans = retrans
	hdr.syn = 1
	hdr.probe_opaque = probe_opaque
	hdr.psn = psn
	hdr.spdcid = spdcid

	hdr.use_rsv_pdc = use_rsv_pdc
	hdr.psn_offset = psn_offset

	return hdr

# RUDI Request and Response

class UetPdsRudiReqResp(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("type", ctypes.c_uint16, 5),
		("next_hdr", ctypes.c_uint16, 4),
		("rsvd1", ctypes.c_uint16, 1),
		("ecn_marked", ctypes.c_uint16, 1),
		("retrans", ctypes.c_uint16, 1),
		("rsvd2", ctypes.c_uint16, 4),

		("rsvd3", ctypes.c_uint16),
		("pkt_id", ctypes.c_uint32),
	]

def FillRudiReqResp(hdr, next_hdr, ecn_marked, retrans, pkt_id):
	hdr.next_hdr = next_hdr
	hdr.ecn_marked = ecn_marked
	hdr.retrans = retrans
	hdr.pkt_id = pkt_id

def MakeRudiRequest(next_hdr, ecn_marked, retrans, pkt_id):
	hdr = UetPdsRudiReqResp()

	hdr.type = UetPdsPktType.UET_PDS_TYPE_RUDI_REQ.value

	FillRudiReqResp(hdr, next_hdr, ecn_marked, retrans, pkt_id)

	return hdr

def MakeRudiResponse(next_hdr, ecn_marked, retrans, pkt_id):
	hdr = UetPdsRudiReqResp()

	hdr.type = UetPdsPktType.UET_PDS_TYPE_RUDI_RESP.value

	FillRudiReqResp(hdr, next_hdr, ecn_marked, retrans, pkt_id)

	return hdr

# NACKs

class UetPdsNackType(Enum):
	UET_PDS_NACK_TYPE_RUD_ROD = 0
	UET_PDS_NACK_TYPE_RUDI = 1

class UetPdsNackCode(Enum):
	UET_PDS_NACK_CODE_TRIMMED = 0x1
	UET_PDS_TRIMMED_LASTHOP = 0x2
	UET_PDS_NACK_CODE_TRIMMED_ACK = 0x3
	UET_PDS_NACK_CODE_NO_PDC_AVAIL = 0x4
	UET_PDS_NACK_CODE_NO_CCC_AVAIL = 0x5
	UET_PDS_NACK_CODE_NO_BITMAP = 0x6
	UET_PDS_NACK_CODE_NO_PKT_BUFFER = 0x7
	UET_PDS_NACK_CODE_NO_GTD_DEL_AVAIL = 0x8
	UET_PDS_NACK_CODE_NO_SES_MSG_AVAIL = 0x9
	UET_PDS_NACK_CODE_NO_RESOURCE = 0xa
	UET_PDS_NACK_CODE_PSN_OOR_WINDOW = 0xb
	UET_PDS_NACK_CODE_ROD_OOO = 0xd
	UET_PDS_NACK_CODE_INV_DPDCID = 0xe
	UET_PDS_NACK_CODE_PDC_HDR_MISMATCH = 0xf
	UET_PDS_NACK_CODE_CLOSING = 0x10
	UET_PDS_NACK_CODE_CLOSING_IN_ERR = 0x11
	UET_PDS_NACK_CODE_PKT_NOT_RCVD = 0x12
	UET_PDS_NACK_CODE_GTD_RESP_UNAVAIL = 0x13
	UET_PDS_NACK_CODE_ACK_WITH_DATA = 0x14
	UET_PDS_NACK_CODE_INVALID_SYN = 0x15
	UET_PDS_NACK_CODE_PDC_MODE_MISMATCH = 0x16
	UET_PDS_NACK_CODE_NEW_START_PSN = 0x17
	UET_PDS_NACK_CODE_RCVD_SES_PROCG = 0x18
	UET_PDS_NACK_CODE_UNEXP_EVENT = 0x19
	UET_PDS_NACK_CODE_RCVR_INFER_LOSS = 0x1a

	UET_PDS_NACK_CODE_EXP_NACK_NORMAL = 0xfd
	UET_PDS_NACK_CODE_EXP_NACK_ERR = 0xfe
	UET_PDS_NACK_CODE_EXP_NACK_FATAL = 0xff

class UetPdsNack(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("type", ctypes.c_uint16, 5),
		("next_hdr", ctypes.c_uint16, 4),
		("rsvd1", ctypes.c_uint16, 1),
		("ecn_marked", ctypes.c_uint16, 1),
		("retrans", ctypes.c_uint16, 1),
		("nak_type", ctypes.c_uint16, 1),
		("rsvd2", ctypes.c_uint16, 3),

		("nak_code", ctypes.c_uint8),
		("vendor_code", ctypes.c_uint8),
		("nak_psn_pkt_id", ctypes.c_uint32),

		("spdcid", ctypes.c_uint16),
		("dpdcid", ctypes.c_uint16),
		("payload", ctypes.c_uint32),
	]

class UetPdsNackCcx(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("nack", UetPdsNack),

		("nccx_type", ctypes.c_uint64, 4),
		("nccx_ccx_state1", ctypes.c_uint64, 4),

		("nccx_ccx_state2", ctypes.c_uint64, 56),
	]

def FillNack(hdr, next_hdr, ecn_marked, retrans, nak_type, nak_code,
	     vendor_code, nak_psn_pkt_id, spdcid, dpdcid, payload):
	hdr.next_hdr = next_hdr
	hdr.ecn_marked = ecn_marked
	hdr.retrans = retrans
	hdr.nak_type = nak_type
	hdr.nak_code = nak_code
	hdr.vendor_code = vendor_code
	hdr.nak_psn_pkt_id = nak_psn_pkt_id
	hdr.spdcid = spdcid
	hdr.dpdcid = dpdcid
	hdr.payload = payload

def MakeNack(next_hdr, ecn_marked, retrans, nak_type, nak_code,
	     vendor_code, nak_psn_pkt_id, spdcid, dpdcid, payload):
	hdr = UetPdsNack()

	hdr.type = UetPdsPktType.UET_PDS_TYPE_NACK.value

	FillNack(hdr, next_hdr, ecn_marked, retrans, nak_type, nak_code,
		 vendor_code, nak_psn_pkt_id, spdcid, dpdcid, payload)

	return hdr

def MakeNackCcx(next_hdr, ecn_marked, retrans, nak_type, nak_code,
		vendor_code, nak_psn_pkt_id, spdcid, dpdcid, payload,
		nccx_type, nccx_ccx_state1, nccx_ccx_state2):
	hdr = UetPdsNackCcx()

	hdr.nack.type = UetPdsPktType.UET_PDS_TYPE_NACK_CCX.value

	FillNack(hdr.nack, next_hdr, ecn_marked, retrans, nak_type, nak_code,
		 vendor_code, nak_psn_pkt_id, spdcid, dpdcid, payload)

	hdr.nccx_type = nccx_type
	hdr.nccx_ccx_state1 = nccx_ccx_state1
	hdr.nccx_ccx_state2 = nccx_ccx_state2

	return hdr

# UUD request

class UetPdsUudReq(ctypes.BigEndianStructure):
	_pack_ = 1
	_fields_ = [
		("type", ctypes.c_uint16, 5),
		("next_hdr", ctypes.c_uint16, 4),
		("flags", ctypes.c_uint16, 4),

		("rsvd", ctypes.c_uint16, 4),
	]

def MakeUudReq(next_hdr):
	hdr = UetPdsUudReq()

	hdr.type = UetPdsPktType.UET_PDS_TYPE_UUD_REQ.value
	hdr.next_hdr = next_hdr

	return hdr
