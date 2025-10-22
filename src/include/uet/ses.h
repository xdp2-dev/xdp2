/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2025 Tom Herbert
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef __UEC_SES_H__
#define __UEC_SES_H__

#include "xdp2/pmacro.h"
#include "xdp2/utility.h"

/* Structure header definitions and functions for UEC-SES (Semantic sublayer) */

enum uet_next_header_type {
	UET_HDR_NONE = 0x0,
	UET_HDR_REQUEST_SMALL = 0x1,
	UET_HDR_REQUEST_MEDIUM = 0x2,
	UET_HDR_REQUEST_STD = 0x3,
	UET_HDR_RESPONSE = 0x4,
	UET_HDR_RESPONSE_DATA = 0x5,
	UET_HDR_RESPONSE_DATA_SMALL = 0x6,
};

static inline const char *uet_next_header_type_to_text(
					enum uet_next_header_type type)
{
        switch (type) {
	case UET_HDR_NONE:
		return "None";
	case UET_HDR_REQUEST_SMALL:
		return "Request small";
	case UET_HDR_REQUEST_MEDIUM:
		return "Request medium";
	case UET_HDR_REQUEST_STD:
		return "Request standard";
	case UET_HDR_RESPONSE:
		return "Response";
	case UET_HDR_RESPONSE_DATA:
		return "Response data";
	case UET_HDR_RESPONSE_DATA_SMALL:
		return "Response data small";
	default:
		return "Unknown type";
	}
}

#define __XDP_SES_REQUEST_VENDOR_OPCODE_ENUM2(VNUM, VAL)		\
	XDP2_JOIN2(UET_SES_REQUEST_OPCODE_VENDOR_DEFINED_, VNUM) = VAL,

#define __XDP_SES_REQUEST_VENDOR_OPCODE_ENUM(PAIR)			\
	__XDP_SES_REQUEST_VENDOR_OPCODE_ENUM2 PAIR

enum uet_ses_request_opcode {
	UET_SES_REQUEST_OPCODE_NO_OP = 0x0,
	UET_SES_REQUEST_OPCODE_WRITE = 0x1,
	UET_SES_REQUEST_OPCODE_READ = 0x2,
	UET_SES_REQUEST_OPCODE_ATOMIC = 0x3,
	UET_SES_REQUEST_OPCODE_FETCHING_ATOMIC = 0x4,
	UET_SES_REQUEST_OPCODE_SEND = 0x5,
	UET_SES_REQUEST_OPCODE_RENDEZVOUS_SEND = 0x6,
	UET_SES_REQUEST_OPCODE_DATAGRAM_SEND = 0x7,
	UET_SES_REQUEST_OPCODE_DEFERRABLE_SEND = 0x8,
	UET_SES_REQUEST_OPCODE_TAGGED_SEND = 0x9,
	UET_SES_REQUEST_OPCODE_RENDEZVOUS_TSEND = 0xa,
	UET_SES_REQUEST_OPCODE_DEFERRABLE_TSEND = 0xb,
	UET_SES_REQUEST_OPCODE_DEFERRABLE_RTR = 0xc,
	UET_SES_REQUEST_OPCODE_TSEND_ATOMIC = 0xd,
	UET_SES_REQUEST_OPCODE_TSEND_FETCH_ATOMIC = 0xe,
	UET_SES_REQUEST_OPCODE_MSG_ERROR = 0xf,

	/* Reserved 0x10-0x2f */

	XDP2_PMACRO_APPLY_ALL(__XDP_SES_REQUEST_VENDOR_OPCODE_ENUM,
			      (0, 0x30), (1, 0x31), (2, 0x32), (3, 0x33),
			      (4, 0x34), (5, 0x35), (6, 0x36), (7, 0x37),
			      (8, 0x38), (9, 0x39), (10, 0x3a), (11, 0x3b),
			      (12, 0x3c), (13, 0x3d), (14, 0x3e))

	UET_SES_REQUEST_OPCODE_EXTENDED = 0x3f,
};

#define __XDP_SES_REQUEST_VENDOR_OPCODE_CASE(VNUM)			\
	case XDP2_JOIN2(UET_SES_REQUEST_OPCODE_VENDOR_DEFINED_, VNUM):	\
		return "Vendor defined #" #VNUM;

static inline const char *uet_ses_request_opcode_to_text(
					enum uet_ses_request_opcode opcode)
{
	switch (opcode) {
	case UET_SES_REQUEST_OPCODE_NO_OP:
		return "No operation";
	case UET_SES_REQUEST_OPCODE_WRITE:
		return "Write";
	case UET_SES_REQUEST_OPCODE_READ:
		return "Read";
	case UET_SES_REQUEST_OPCODE_ATOMIC:
		return "Atomic";
	case UET_SES_REQUEST_OPCODE_FETCHING_ATOMIC:
		return "Fetching atomic";
	case UET_SES_REQUEST_OPCODE_SEND:
		return "Send";
	case UET_SES_REQUEST_OPCODE_RENDEZVOUS_SEND:
		return "Rendezvous send";
	case UET_SES_REQUEST_OPCODE_DATAGRAM_SEND:
		return "Datagram send";
	case UET_SES_REQUEST_OPCODE_DEFERRABLE_SEND:
		return "Deferrable send";
	case UET_SES_REQUEST_OPCODE_TAGGED_SEND:
		return "Tagged send";
	case UET_SES_REQUEST_OPCODE_RENDEZVOUS_TSEND:
		return "Rendezvous tagged send";
	case UET_SES_REQUEST_OPCODE_DEFERRABLE_TSEND:
		return "Deferrable tagged send";
	case UET_SES_REQUEST_OPCODE_DEFERRABLE_RTR:
		return "Deferred send ready to restart";
	case UET_SES_REQUEST_OPCODE_TSEND_ATOMIC:
		return "Tagged send atomic";
	case UET_SES_REQUEST_OPCODE_TSEND_FETCH_ATOMIC:
		return "Tagged send fetch atomic";
	case UET_SES_REQUEST_OPCODE_MSG_ERROR:
		return "Terminate message in error";

	/* Reserved 0x10-0x2f */

	XDP2_PMACRO_APPLY_ALL(__XDP_SES_REQUEST_VENDOR_OPCODE_CASE,
			      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14)

	case UET_SES_REQUEST_OPCODE_EXTENDED:
		return "Opcode escaped to extend opcode (TBD)";

	default:
		return "Unknown opcode";
	}
}

enum uet_ses_list_delivered_to_type  {
	UET_SES_LIST_DELIVERED_EXPECTED = 0,
	UET_SES_LIST_DELIVERED_OVERFLOW = 1,
	UET_SES_LIST_DELIVERED_VENDOR_DEFINED1 = 2,
	UET_SES_LIST_DELIVERED_VENDOR_DEFINED2 = 3,
};

static inline const char *uet_ses_list_delivered_to_text(
		enum uet_ses_list_delivered_to_type type)
{
	switch (type) {
	case UET_SES_LIST_DELIVERED_EXPECTED:
		return "expected";
	case UET_SES_LIST_DELIVERED_OVERFLOW:
		return "overflow";
	case UET_SES_LIST_DELIVERED_VENDOR_DEFINED1:
		return "Vendor defined 1";
	case UET_SES_LIST_DELIVERED_VENDOR_DEFINED2:
		return "Vendor defined 2";
	default:
		return "unknown";
	}
}

#define __XDP_SES_RESPONSE_TYPE_VENDOR_ENUM2(VNUM, VAL)		\
	XDP2_JOIN2(UET_SES_RESPONSE_TYPE_VENDOR_DEFINED_, VNUM) = VAL,

#define __XDP_SES_RESPONSE_TYPE_VENDOR_ENUM(PAIR)			\
	__XDP_SES_RESPONSE_TYPE_VENDOR_ENUM2 PAIR

enum uet_ses_response_type {
	UET_SES_RESPONSE_TYPE_DEFAULT = 0x0,
	UET_SES_RESPONSE_TYPE_OTHER = 0x1,
	UET_SES_RESPONSE_TYPE_WITH_DATA = 0x2,
	UET_SES_RESPONSE_TYPE_NONE = 0x3,

	/* Reserved 0x5-0x2f */

	XDP2_PMACRO_APPLY_ALL(__XDP_SES_RESPONSE_TYPE_VENDOR_ENUM,
			      (0, 0x30), (1, 0x31), (2, 0x32), (3, 0x33),
			      (4, 0x34), (5, 0x35), (6, 0x36), (7, 0x37),
			      (8, 0x38), (9, 0x39), (10, 0x3a), (11, 0x3b),
			      (12, 0x3c), (13, 0x3d), (14, 0x3e), (15, 0x3f))
};

#define __XDP_SES_RESPONSE_TYPE_VENDOR_CASE(VNUM)			\
	case XDP2_JOIN2(UET_SES_RESPONSE_TYPE_VENDOR_DEFINED_, VNUM):	\
		return "Vendor defined #" #VNUM;

static inline const char *uet_ses_reponse_type_to_text(
					enum uet_ses_response_type type)
{
	switch (type) {
	case UET_SES_RESPONSE_TYPE_DEFAULT:
		return "Default response";
	case UET_SES_RESPONSE_TYPE_OTHER:
		return "Other than default response";
	case UET_SES_RESPONSE_TYPE_WITH_DATA:
		return "Response carrying data";
	case UET_SES_RESPONSE_TYPE_NONE:
		return "No semantic response availbale";

	XDP2_PMACRO_APPLY_ALL(__XDP_SES_RESPONSE_TYPE_VENDOR_CASE,
			      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
			      14, 15)

	default:
		return "Unknown type";
	}
}

#define __XDP_SES_RETURN_VENDOR_CODE_ENUM2(VNUM, VAL)			\
	XDP2_JOIN2(UET_SES_RETURN_CODE_VENDOR_DEFINED_, VNUM) = VAL,

#define __XDP_SES_RETURN_VENDOR_CODE_ENUM(PAIR)				\
	__XDP_SES_RETURN_VENDOR_CODE_ENUM2 PAIR

enum uet_ses_return_code {
	UET_SES_RETURN_CODE_NULL = 0x0,
	UET_SES_RETURN_CODE_OK = 0x1,
	UET_SES_RETURN_CODE_BAD_GENERATION = 0x2,
	UET_SES_RETURN_CODE_DISABLED = 0x3,
	UET_SES_RETURN_CODE_DISABLED_GEN = 0x4,
	UET_SES_RETURN_CODE_NO_MATCH = 0x5,
	UET_SES_RETURN_CODE_UNSUPPORTED_OP = 0x6,
	UET_SES_RETURN_CODE_UNSUPPORTED_SIZE = 0x7,
	UET_SES_RETURN_CODE_AT_INVALID = 0x8,
	UET_SES_RETURN_CODE_AT_PERM = 0x9,
	UET_SES_RETURN_CODE_AT_ATS_ERROR = 0xa,
	UET_SES_RETURN_CODE_AT_NO_TRANS = 0xb,
	UET_SES_RETURN_CODE_AT_OUT_OF_RANGE = 0xc,
	UET_SES_RETURN_CODE_HOST_POISONED = 0xd,
	UET_SES_RETURN_CODE_HOST_UNSUCCESS_CMPL = 0xe,
	UET_SES_RETURN_CODE_AMO_UNSUPPORTED_OP = 0xf,
	UET_SES_RETURN_CODE_AMO_UNSUPPORTED_DT = 0x10,
	UET_SES_RETURN_CODE_AMO_UNSUPPORTED_SIZE = 0x11,
	UET_SES_RETURN_CODE_AMO_UNALIGNED = 0x12,
	UET_SES_RETURN_CODE_AMO_FP_NAN = 0x13,
	UET_SES_RETURN_CODE_AMO_FP_UNDERFLOW = 0x14,
	UET_SES_RETURN_CODE_AMO_FP_OVERFLOW = 0x15,
	UET_SES_RETURN_CODE_AMO_FP_INEXACT = 0x16,
	UET_SES_RETURN_CODE_PERM_VIOLATION = 0x17,
	UET_SES_RETURN_CODE_OP_VIOLATION = 0x18,
	UET_SES_RETURN_CODE_BAD_INDEX = 0x19,
	UET_SES_RETURN_CODE_BAD_PID = 0x1a,
	UET_SES_RETURN_CODE_BAD_JOB_ID = 0x1b,
	UET_SES_RETURN_CODE_BAD_MKEY = 0x1c,
	UET_SES_RETURN_CODE_BAD_ADDR = 0x1d,
	UET_SES_RETURN_CODE_CANCELLED = 0x1e,
	UET_SES_RETURN_CODE_UNDELIVERABLE = 0x1f,
	UET_SES_RETURN_CODE_UNCOR = 0x20,
	UET_SES_RETURN_CODE_UNCOR_TRNSNT = 0x21,
	UET_SES_RETURN_CODE_TOO_LONG = 0x22,
	UET_SES_RETURN_CODE_INITIATOR_ERROR = 0x23,
	UET_SES_RETURN_CODE_DROPPED = 0x24,

	/* Reserved 0x30-0x37 */

	XDP2_PMACRO_APPLY_ALL(__XDP_SES_RETURN_VENDOR_CODE_ENUM,
			      (0, 0x30), (1, 0x31), (2, 0x32), (3, 0x33),
			      (4, 0x34), (5, 0x35), (6, 0x36), (7, 0x37))

	/* Reserved with data 0x38-0x3d */

	UET_SES_RETURN_CODE_EXTENDED = 0x3e,

	/* Reserved 0x3f */
};

#define __XDP_SES_RETURN_VENDOR_CODE_CASE(VNUM)				\
	case XDP2_JOIN2(UET_SES_RETURN_CODE_VENDOR_DEFINED_, VNUM):	\
		return "Vendor defined #" #VNUM;

static inline const char *uet_ses_return_code_to_text(
					enum uet_ses_return_code code)
{
	switch (code) {
	case UET_SES_RETURN_CODE_NULL:
		return "NULL code";
	case UET_SES_RETURN_CODE_OK:
		return "Okay";
	case UET_SES_RETURN_CODE_BAD_GENERATION:
		return "Bad generation in request";
	case UET_SES_RETURN_CODE_DISABLED:
		return "Target reource is disabled";
	case UET_SES_RETURN_CODE_DISABLED_GEN:
		return "Target reource is disabled and support index gen";
	case UET_SES_RETURN_CODE_NO_MATCH:
		return "Message not matched";
	case UET_SES_RETURN_CODE_UNSUPPORTED_OP:
		return "Unsupported network message type";
	case UET_SES_RETURN_CODE_UNSUPPORTED_SIZE:
		return "Message size too big";
	case UET_SES_RETURN_CODE_AT_INVALID:
		return "Invalid address translation context";
	case UET_SES_RETURN_CODE_AT_PERM:
		return "Address translation permission failure";
	case UET_SES_RETURN_CODE_AT_ATS_ERROR:
		return "ATS translation request error";
	case UET_SES_RETURN_CODE_AT_NO_TRANS:
		return "Unable to obtain a translation";
	case UET_SES_RETURN_CODE_AT_OUT_OF_RANGE:
		return "Virtual address is out of range at translation";
	case UET_SES_RETURN_CODE_HOST_POISONED:
		return "Host read access poisoned";
	case UET_SES_RETURN_CODE_HOST_UNSUCCESS_CMPL:
		return "Host read unsuccessful completion";
	case UET_SES_RETURN_CODE_AMO_UNSUPPORTED_OP:
		return "Unsupported AMO message type";
	case UET_SES_RETURN_CODE_AMO_UNSUPPORTED_DT:
		return "Invalid datatype at target";
	case UET_SES_RETURN_CODE_AMO_UNSUPPORTED_SIZE:
		return "AMO op not integral multiple of datatype size";
	case UET_SES_RETURN_CODE_AMO_UNALIGNED:
		return "AMO op no natively aligned to datatype size";
	case UET_SES_RETURN_CODE_AMO_FP_NAN:
		return "AMO operation generated a NaN";
	case UET_SES_RETURN_CODE_AMO_FP_UNDERFLOW:
		return "AMO operation generated and underflow";
	case UET_SES_RETURN_CODE_AMO_FP_OVERFLOW:
		return "AMO operation generated and overflow";
	case UET_SES_RETURN_CODE_AMO_FP_INEXACT:
		return "AMO operation generated an inexact exception";
	case UET_SES_RETURN_CODE_PERM_VIOLATION:
		return "Message processing encountered a perm violation";
	case UET_SES_RETURN_CODE_OP_VIOLATION:
		return "Message processing encountered an op violation";
	case UET_SES_RETURN_CODE_BAD_INDEX:
		return "Unconfigured index encountered";
	case UET_SES_RETURN_CODE_BAD_PID:
		return "PID was not found at target";
	case UET_SES_RETURN_CODE_BAD_JOB_ID:
		return "JobID was not found at target";
	case UET_SES_RETURN_CODE_BAD_MKEY:
		return "Memory key doesn't map to a buffer";
	case UET_SES_RETURN_CODE_BAD_ADDR:
		return "Invalid address";
	case UET_SES_RETURN_CODE_CANCELLED:
		return "Target cancelled in-flight message";
	case UET_SES_RETURN_CODE_UNDELIVERABLE:
		return "Message could not be delivered";
	case UET_SES_RETURN_CODE_UNCOR:
		return "Uncorrectable error detected";
	case UET_SES_RETURN_CODE_UNCOR_TRNSNT:
		return "Transient uncorrectable error detected";
	case UET_SES_RETURN_CODE_TOO_LONG:
		return "Message too long";
	case UET_SES_RETURN_CODE_INITIATOR_ERROR:
		return "Iniator error reflected";
	case UET_SES_RETURN_CODE_DROPPED:
		return "Message dropped for other reason";

	XDP2_PMACRO_APPLY_ALL(__XDP_SES_RETURN_VENDOR_CODE_CASE,
			      0, 1, 2, 3, 4, 5, 6, 7)

	case UET_SES_RETURN_CODE_EXTENDED:
		return "Code escaped to extend opcode (TBD)";

	default:
		return "Unknown code";
	}
}

#define __XDP_SES_AMO_VENDOR_ENUM2(VNUM, VAL)				\
	XDP2_JOIN2(UET_SES_AMO_VENDOR_DEFINED_, VNUM) = VAL,

#define __XDP_SES_AMO_VENDOR_ENUM(PAIR) __XDP_SES_AMO_VENDOR_ENUM2 PAIR

/* Atomic opcodes */
enum uet_ses_amo_opcode {
	UET_SES_AMO_MIN = 0x0,
	UET_SES_AMO_MAX = 0x1,
	UET_SES_AMO_SUM = 0x2,
	UET_SES_AMO_DIFF = 0x3,
	UET_SES_AMO_PROD = 0x4,
	UET_SES_AMO_LOR  = 0x5,
	UET_SES_AMO_LAND = 0x6,
	UET_SES_AMO_BOR = 0x7,
	UET_SES_AMO_BAND = 0x8,
	UET_SES_AMO_LXOR = 0x9,
	UET_SES_AMO_BXOR = 0xa,
	UET_SES_AMO_READ = 0xb,
	UET_SES_AMO_WRITE = 0xc,
	UET_SES_AMO_CSWAP = 0xd,
	UET_SES_AMO_CSWAP_NE = 0x0e,
	UET_SES_AMO_CSWAP_LE = 0xf,
	UET_SES_AMO_CSWAP_LT = 0x10,
	UET_SES_AMO_CSWAP_GE = 0x11,
	UET_SES_AMO_CSWAP_GT = 0x12,
	UET_SES_AMO_MSWAP = 0x13,
	UET_SES_AMO_INVAL = 0x14,

	/* Reserved 0x15-0xdf */

	XDP2_PMACRO_APPLY_ALL(__XDP_SES_AMO_VENDOR_ENUM,
			      (0, 0xe0), (1, 0xe1), (2, 0xe2), (3, 0xe3),
			      (4, 0xe4), (5, 0xe5), (6, 0xe6), (7, 0xe7),
			      (8, 0xe8), (9, 0xe9), (10, 0xea), (11, 0xeb),
			      (12, 0xec), (13, 0xed), (14, 0xee), (15, 0xef),
			      (16, 0xf0), (17, 0xf1), (18, 0xf2), (19, 0xf3),
			      (20, 0xf4), (21, 0xf5), (22, 0xf6), (23, 0xf7),
			      (24, 0xf8), (25, 0xf9), (26, 0xfa), (27, 0xfb),
			      (28, 0xfc), (29, 0xfd), (30, 0xfe))

	/* Reserved 0xff */
};

#define __XDP_SES_AMO_CASE(VNUM)					\
	case XDP2_JOIN2(UET_SES_AMO_VENDOR_DEFINED_, VNUM):		\
		return "Vendor defined #" #VNUM;

static inline const char *uet_ses_amo_to_text(enum uet_ses_amo_opcode type)
{
	switch (type) {
	case UET_SES_AMO_MIN:
		return "Minimum";
	case UET_SES_AMO_MAX:
		return "Maximum";
	case UET_SES_AMO_SUM:
		return "Sum";
	case UET_SES_AMO_DIFF:
		return "Difference";
	case UET_SES_AMO_PROD:
		return "Product";
	case UET_SES_AMO_LOR:
		return "Logical OR";
	case UET_SES_AMO_LAND:
		return "Logical AND";
	case UET_SES_AMO_BOR:
		return "Biwise OR";
	case UET_SES_AMO_BAND:
		return "Bitwise AND";
	case UET_SES_AMO_LXOR:
		return "Logical XOR";
	case UET_SES_AMO_BXOR:
		return "Bitwise XOR";
	case UET_SES_AMO_READ:
		return "Atomic Read";
	case UET_SES_AMO_WRITE:
		return "Atomic Write";
	case UET_SES_AMO_CSWAP:
		return "Compare and swap if equal";
	case UET_SES_AMO_CSWAP_NE:
		return "Compare and swap if not equal";
	case UET_SES_AMO_CSWAP_LE:
		return "Compare and swap if less than or equal";
	case UET_SES_AMO_CSWAP_LT:
		return "Compare and swap if less than";
	case UET_SES_AMO_CSWAP_GE:
		return "Compare and swap if greater than or equal";
	case UET_SES_AMO_CSWAP_GT:
		return "Compare and swap if greater than";
	case UET_SES_AMO_MSWAP:
		return "Swap masked bits";
	case UET_SES_AMO_INVAL:
		return "Invalites cached target";
	XDP2_PMACRO_APPLY_ALL(__XDP_SES_AMO_CASE,
			      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
			      15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
			      27, 28, 29, 30)
	default:
		return "Unknown opcode";
	}
}

#define __XDP_SES_AMO_DATATYPE_VENDOR_ENUM2(VNUM, VAL)			\
	XDP2_JOIN2(UET_SES_AMO_DATATYPE_VENDOR_DEFINED_, VNUM) = VAL,

#define __XDP_SES_AMO_DATATYPE_VENDOR_ENUM(PAIR)			\
	__XDP_SES_AMO_DATATYPE_VENDOR_ENUM2 PAIR

/* Atomic datatypes */
enum uet_ses_amo_datatype {
	UET_SES_AMO_DATATYPE_INT8 = 0x0,
	UET_SES_AMO_DATATYPE_UINT8 = 0x1,
	UET_SES_AMO_DATATYPE_INT16 = 0x2,
	UET_SES_AMO_DATATYPE_UINT16 = 0x3,
	UET_SES_AMO_DATATYPE_INT32 = 0x4,
	UET_SES_AMO_DATATYPE_UINT32 = 0x5,
	UET_SES_AMO_DATATYPE_INT64 = 0x6,
	UET_SES_AMO_DATATYPE_UINT64 = 0x7,
	UET_SES_AMO_DATATYPE_INT128 = 0x8,
	UET_SES_AMO_DATATYPE_UINT128 = 0x9,
	UET_SES_AMO_DATATYPE_FLOAT = 0xa,
	UET_SES_AMO_DATATYPE_DOUBLE = 0xb,
	UET_SES_AMO_DATATYPE_FLOAT_COMPLEX = 0xc,
	UET_SES_AMO_DATATYPE_DOUBLE_COMPLEX = 0xd,
	UET_SES_AMO_DATATYPE_LONG_DOUBLE = 0xe,
	UET_SES_AMO_DATATYPE_LONG_DOUBLE_COMPLEX = 0xf,
	UET_SES_AMO_DATATYPE_BF16 = 0x10,
	UET_SES_AMO_DATATYPE_FP16 = 0x11,

	/* Reserved 0x12-0xdf */

	XDP2_PMACRO_APPLY_ALL(__XDP_SES_AMO_DATATYPE_VENDOR_ENUM,
			      (0, 0xe0), (1, 0xe1), (2, 0xe2), (3, 0xe3),
			      (4, 0xe4), (5, 0xe5), (6, 0xe6), (7, 0xe7),
			      (8, 0xe8), (9, 0xe9), (10, 0xea), (11, 0xeb),
			      (12, 0xec), (13, 0xed), (14, 0xee), (15, 0xef),
			      (16, 0xf0), (17, 0xf1), (18, 0xf2), (19, 0xf3),
			      (20, 0xf4), (21, 0xf5), (22, 0xf6), (23, 0xf7),
			      (24, 0xf8), (25, 0xf9), (26, 0xfa), (27, 0xfb),
			      (28, 0xfc), (29, 0xfd), (30, 0xfe))

	/* Reserved 0xff */
};

#define __XDP_SES_AMO_DATATYPE_CASE(VNUM)				\
	case XDP2_JOIN2(UET_SES_AMO_DATATYPE_VENDOR_DEFINED_, VNUM):	\
		return "Vendor defined #" #VNUM;

static inline const char *uet_ses_amo_datatype_to_text(
					enum uet_ses_amo_datatype type)
{
	switch (type) {
	case UET_SES_AMO_DATATYPE_INT8:
		return "int8";
	case UET_SES_AMO_DATATYPE_UINT8:
		return "unsigned int8";
	case UET_SES_AMO_DATATYPE_INT16:
		return "int16";
	case UET_SES_AMO_DATATYPE_UINT16:
		return "unsigned int16";
	case UET_SES_AMO_DATATYPE_INT32:
		return "int32";
	case UET_SES_AMO_DATATYPE_UINT32:
		return "unsigned int32";
	case UET_SES_AMO_DATATYPE_INT64:
		return "int64";
	case UET_SES_AMO_DATATYPE_UINT64:
		return "unsigned int64";
	case UET_SES_AMO_DATATYPE_INT128:
		return "int128";
	case UET_SES_AMO_DATATYPE_UINT128:
		return "unsigned int128";
	case UET_SES_AMO_DATATYPE_FLOAT:
		return "float";
	case UET_SES_AMO_DATATYPE_DOUBLE:
		return "double";
	case UET_SES_AMO_DATATYPE_FLOAT_COMPLEX:
		return "float";
	case UET_SES_AMO_DATATYPE_DOUBLE_COMPLEX:
		return "double complex";
	case UET_SES_AMO_DATATYPE_LONG_DOUBLE:
		return "long double";
	case UET_SES_AMO_DATATYPE_LONG_DOUBLE_COMPLEX:
		return "long double complex";
	case UET_SES_AMO_DATATYPE_BF16:
		return "BF16";
	case UET_SES_AMO_DATATYPE_FP16:
		return "FP16";

	XDP2_PMACRO_APPLY_ALL(__XDP_SES_AMO_DATATYPE_CASE,
			      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
			      15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
			      27, 28, 29, 30)
	default:
		return "Unknown data type";
	}
}

/* Extension headers (rendezvous and atomics) */

struct uet_ses_rendezvous_ext_hdr {
	__be32 eager_length;

#if defined(__BIG_ENDIAN_BITFIELD)
	__u32 ri_generation: 8;
	__u32 pid_on_fep: 12;
	__u32 resource_index: 12;
#else
	__u32 ri_generation: 8;
	__u32 pid_on_fep_high: 8;
	__u32 resource_index_high: 4;
	__u32 pid_on_fep_low: 4;
	__u32 resource_index_low: 8;
#endif
} __packed;

/* Helper functions to get/set pid_on_fep from bitfields */
static inline __u16 uet_ses_rendezvous_ext_hdr_get_pid_on_fep(const void *vhdr)
{
	const struct uet_ses_rendezvous_ext_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return hdr->pid_on_fep;
#else
	return (hdr->pid_on_fep_high << 4) + (hdr->pid_on_fep_low & 0xf);
#endif
}

static inline void uet_ses_rendezvous_ext_hdr_set_pid_on_fep(
						void *vhdr, __u16 val)
{
	struct uet_ses_rendezvous_ext_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	hdr->pid_on_fep = val;
#else
	hdr->pid_on_fep_high = val >> 4;
	hdr->pid_on_fep_low = val & 0xf;
#endif
}

/* Helper functions to get/set resource_index from bitfields */
static inline __u16 uet_ses_rendezvous_ext_hdr_get_resource_index(
							const void *vhdr)
{
	const struct uet_ses_rendezvous_ext_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return hdr->resource_index;
#else
	return (hdr->resource_index_high << 8) +
					(hdr->resource_index_low & 0xff);
#endif
}

static inline void uet_ses_rendezvous_ext_hdr_set_resource_index(
						void *vhdr, __u16 val)
{
	struct uet_ses_rendezvous_ext_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	hdr->resource_index = val;
#else
	hdr->resource_index_high = val >> 8;
	hdr->resource_index_low = val & 0xff;
#endif
}

#define UET_SES_OPCODE_VERSION_MASK __cpu_to_be16(0x3fc0)
#define UET_SES_MATCH_OPCODE(OPCODE) __cpu_to_be16((OPCODE) << 8)
#define UET_SES_MATCH_VERSION(VERSION) __cpu_to_be16((VERSION) << 6)
#define UET_SES_MATCH_OPCODE_VERSION(OPCODE, VERSION)			\
		UET_SES_MATCH_OPCODE(OPCODE) | UET_SES_MATCH_VERSION(VERSION)

/* SES base header. THe first sixteen bits for switching on opcode and
 * version
 */

struct uet_ses_base_hdr {
	union {
		struct {
#if defined(__BIG_ENDIAN_BITFIELD)
			__u8 rsvd1: 2;
			__u8 opcode: 6;
#else
			__u8 opcode: 6;
			__u8 rsvd1: 2;
#endif

#if defined(__BIG_ENDIAN_BITFIELD)
			__u8 version: 2;
			__u8 rsvd2: 6;
#else
			__u8 rsvd2: 6;
			__u8 version: 2;
#endif
			__u8 flags;
		};
		__u16 v;
	};
} __packed;

/* Common SES header */
struct uet_ses_common_hdr {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 rsvd1: 2;
	__u8 opcode: 6;
#else
	__u8 opcode: 6;
	__u8 rsvd1: 2;
#endif

#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 version: 2;
	__u8 delivery_complete: 1;
	__u8 initiator_error: 1;
	__u8 relative_addressing: 1;
	__u8 hdr_data_present: 1;
	__u8 end_of_msg: 1;
	__u8 start_of_msg: 1;
#else
	__u8 start_of_msg: 1;
	__u8 end_of_msg: 1;
	__u8 hdr_data_present: 1;
	__u8 relative_addressing: 1;
	__u8 initiator_error: 1;
	__u8 delivery_complete: 1;
	__u8 version: 2;
#endif

	__be16 message_id;

	__u32 ri_generation: 8;
	__u32 job_id: 24;

#if defined(__BIG_ENDIAN_BITFIELD)
	__u16 rsvd2: 4;
	__u16 pid_on_fep: 12;
#else
	__u16 pid_on_fep_high: 4;
	__u16 rsvd2: 4;
	__u16 pid_on_fep_low: 8;
#endif

#if defined(__BIG_ENDIAN_BITFIELD)
	__u16 rsvd3: 4;
	__u16 resource_index: 12;
#else
	__u16 resource_index_high: 4;
	__u16 rsvd3: 4;
	__u16 resource_index_low: 8;
#endif
} __packed;


/* Helper functions to get/set pid_on_fep from bitfields */
static inline __u16 uet_ses_common_get_pid_on_fep(const void *vhdr)
{
	const struct uet_ses_common_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return hdr->pid_on_fep;
#else
	return (hdr->pid_on_fep_high << 8) + (hdr->pid_on_fep_low & 0xff);
#endif
}

static inline void uet_ses_common_set_pid_on_fep(void *vhdr, __u16 val)
{
	struct uet_ses_common_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	hdr->pid_on_fep = val;
#else
	hdr->pid_on_fep_high = val >> 8;
	hdr->pid_on_fep_low = val & 0xff;
#endif
}

/* Helper functions to get/set resource_index from bitfields */
static inline __u16 uet_ses_common_get_resource_index(const void *vhdr)
{
	const struct uet_ses_common_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return hdr->resource_index;
#else
	return (hdr->resource_index_high << 8) +
					(hdr->resource_index_low & 0xff);
#endif
}

static inline void uet_ses_common_set_resource_index(void *vhdr, __u16 val)
{
	struct uet_ses_common_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	hdr->resource_index = val;
#else
	hdr->resource_index_high = val >> 8;
	hdr->resource_index_low = val & 0xff;
#endif
}

/* Standard request header */
struct uet_ses_request_std_hdr {
	struct uet_ses_common_hdr cmn;

	__be64 buffer_offset;
	__be32 initiator;
	union {
		__be64 memory_key;
		__be64 match_bits;
	};
	union {
		__be64 header_data; /* Start of message */
		struct {
			__u16 rsvd1;

#if defined(__BIG_ENDIAN_BITFIELD)
			__u16 rsvd2: 2;
			__u16 payload_length: 14;
#else
			__u16 payload_length_high: 6;
			__u16 rsvd2: 2;
			__u16 payload_length_low: 8;
#endif

			__be32 message_offset;
		};
	};

	__be32 request_length;
} __packed;

/* Helper functions to get/set payload length from bitfields */
static inline __u16 uet_ses_request_std_hdr_get_payload_length(
							const void *vhdr)
{
	const struct uet_ses_request_std_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return hdr->payload_length;
#else
	return (hdr->payload_length_high << 8) +
					(hdr->payload_length_low & 0x3ff);
#endif
}

static inline void uet_ses_request_std_hdr_set_payload_length(
							void *vhdr, __u16 val)
{
	struct uet_ses_request_std_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	hdr->payload_length = val;
#else
	hdr->payload_length_high = val >> 8;
	hdr->payload_length_low = val & 0x3ff;
#endif
}

/* Standard deferrable send header */
struct uet_ses_defer_send_std_hdr {
	struct uet_ses_common_hdr cmn;

	__be32 initiator_restart_token;
	__be32 target_restart_token;
	__be32 initiator;
	__be64 match_bits;
	__be64 header_data;
	__be32 request_length;
} __packed;

/* Standard ready-to-restart header */
struct uet_ses_ready_to_restart_std_hdr {
	struct uet_ses_common_hdr cmn;

	__be64 buffer_offset;
	__be32 initiator;
	__be32 initiator_restart_token;
	__be32 target_restart_token;
	__be64 header_data;
	__be32 request_length;
} __packed;

/* Sandard rendezvous header including rendezvous extension header */
struct uet_ses_rendezvous_std_hdr {
	struct uet_ses_common_hdr cmn;

	__be64 buffer_offset;
	__be32 initiator;
	__be64 match_bits;

	struct uet_ses_rendezvous_ext_hdr ext_hdr;
} __packed;

/* Atomic extension  header */
struct uet_ses_atomic_op_ext_hdr {
	__u8 atomic_code;
	__u8 atomic_datatype;
	union {
		__u8 semantic_control;
		struct {
#if defined(__BIG_ENDIAN_BITFIELD)
			__u8 ctrl_cacheable: 1;
			__u8 cpu_coherent: 1;
			__u8 rsvd1: 3;
			__u8 vendor_defined: 3;
#else
			__u8 vendor_defined: 3;
			__u8 rsvd1: 3;
			__u8 cpu_coherent: 1;
			__u8 ctrl_cacheable: 1;
#endif
		};
	};
	__u8 rsvd2;
} __packed;

/* Atomic compare and swap extension  header */
struct uet_ses_atomic_cmp_and_swap_ext_hdr {
	struct uet_ses_atomic_op_ext_hdr cmn;

	__u8 compare_value[16];
	__u8 swap_value[16];
} __packed;

/* Small request header */
struct uet_ses_request_small_hdr {
	struct uet_ses_common_hdr cmn;

	__be64 buffer_offset;
} __packed;

/* Medium request header */
struct uet_ses_request_medium_hdr {
	struct uet_ses_common_hdr cmn;

	union {
		__be64 header_data;
		__be64 buffer_offset;
	};
	__be32 initiator;
	union {
		__be64 memory_key;
		__be64 match_bits;
	};
} __packed;

/* Common respnse header */
struct uet_ses_common_response_hdr {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 list: 2;
	__u8 opcode: 6;
#else
	__u8 opcode: 6;
	__u8 list: 2;
#endif

#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 ver: 2;
	__u8 return_code: 6;
#else
	__u8 return_code: 6;
	__u8 ver: 2;
#endif

	__u16 message_id;

	__u32 ri_generation: 8;
	__u32 job_id: 24;
} __packed;

/* No data respnse header */
struct uet_ses_nodata_response_hdr {
	struct uet_ses_common_response_hdr cmn;

	__be32 modified_length;
} __packed;

/* With data respnse header */
struct uet_ses_with_data_response_hdr {
	struct uet_ses_common_response_hdr cmn;

	__be16 read_request_message_id;

#if defined(__BIG_ENDIAN_BITFIELD)
	__u16 rsvd2: 4;
	__u16 payload_length: 12;
#else
	__u16 payload_length_high: 4;
	__u16 rsvd2: 4;
	__u16 payload_length_low: 8;
#endif

	__be32 modified_length;
	__be32 message_offset;
} __packed;

/* Helper functions to get/set pid_on_fep from bitfields */
static inline __u16 uet_ses_response_with_data_hdr_get_payload_length(
							const void *vhdr)
{
	const struct uet_ses_with_data_response_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return hdr->pid_on_fep;
#else
	return (hdr->payload_length_high << 8) +
					(hdr->payload_length_low & 0xff);
#endif
}

static inline void uet_ses_with_data_response_hdr_set_payload_length(
							void *vhdr, __u16 val)
{
	struct uet_ses_with_data_response_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	hdr->payload_length = val;
#else
	hdr->payload_length_high = val >> 8;
	hdr->payload_length_low = val & 0xff;
#endif
}

/* With small data respnse header */
struct uet_ses_with_small_data_response_hdr {
#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 list: 2;
	__u8 opcode: 6;
#else
	__u8 opcode: 6;
	__u8 list: 2;
#endif

#if defined(__BIG_ENDIAN_BITFIELD)
	__u8 ver: 2;
	__u8 return_code: 6;
#else
	__u8 return_code: 6;
	__u8 ver: 2;
#endif

#if defined(__BIG_ENDIAN_BITFIELD)
			__u16 rsvd1: 2;
			__u16 payload_length: 14;
#else
			__u16 payload_length_high: 6;
			__u16 rsvd1: 2;
			__u16 payload_length_low: 8;
#endif
	__u32 rsvd2: 8;
	__u32 job_id: 24;

	__be32 original_request_psn;
} __packed;

/* Helper functions to get/set payload length from bitfields */
static inline __u16 uet_ses_with_small_data_response_hdr_get_payload_length(
							const void *vhdr)
{
	const struct uet_ses_with_small_data_response_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	return hdr->payload_length;
#else
	return (hdr->payload_length_high << 8) +
					(hdr->payload_length_low & 0x3ff);
#endif
}

static inline void uet_ses_with_small_data_response_hdr_set_payload_length(
							void *vhdr, __u16 val)
{
	struct uet_ses_with_small_data_response_hdr *hdr = vhdr;

#if defined(__BIG_ENDIAN_BITFIELD)
	hdr->payload_length = val;
#else
	hdr->payload_length_high = val >> 8;
	hdr->payload_length_low = val & 0x3ff;
#endif
}

#endif /* __UEC_SES_H__ */
