<img src="../images/SUPERp.png" alt="XDP2 logo"/>

Scale UP EtheRnet Protocol (SUPERp)
===================================

This document describes the implementation of Scale UP EtheRnet Protocol
(SUPERp). SUPERp is described [here](../pdf/SUPERp.pdf).

Current support
===============

The features currently supported are:

* Standard include files with header formats and constant definitions for
 SUPERp
* Packet tools to make a variety SUPERp of protocol format
  samples in .pcap files
* Support in **parse_dump** to parse SUPERp packets

Packet formats
==============

The Packets formats for SUPERp are in
[include/superp/superp.h](../../src/include/superp/superp.h).

Conformance
-----------

The packet format covers that described in
[SUPERp](../pdf/SUPERp.pdf).

Protocol layers
---------------

SUPERp is split into three protocol layers. The Packet Delivery Layer, or
PDL, provides packet transport and reliability. The Transaction Layer, or
TAL, manages initiator to target transactions. Following the transition
layer are zero or more operations.

Encapsulation
-------------

SUPERp, starting with the PDL header, is encapsulated in a UDP packet or
[SUNH](./sunh.md). By default the UDP Destination Port for SUPERp is 4444, and
the protocol number that is set in the Next Header field of SUNH header is
253 (an experimental IP protocol number). An alternate encapsulation called
SUPERp-lite may be used if the Ethernet layer is lossless in which case the
PDL header can be eliminated and a TAL header is the first header after
Ethernet (a different EtherType would be used in this case).
be

parse_dump
==========

Parse dump support is added via
[include/superp/parser_test.h](../../src/include/superp/parser_test.h).
That file creates all the parse nodes for PDL, TAL, and various operation
types. The file
[parser.c](../../src/test/parse_dump/parser.c)) has been updated to parse
SUPERp packets based on UDP destination port or IP protocol number (in SUNH
encapsulation).

pcap samples
============

.pcap files with samples of SUNH packets are built from tools in
[tools/packets/sunh](../../src/tools/packets/sunh). One .pcap files
is created, **sunh.pcap**, that contains sample packets with the SUNH header.

Running parse_dump
==================

**parse_dump** can be run with the .pcap files from the SUPERp packets tool
[directory](../../src/tools/packets/superp).
From src/test/parse_dump we can parse SUPERp packets:
```
$  ./parse_dump -c 1 -v 10 -U ../../tools/packets/superp/superp.pcap
------------------------------------
File ../../tools/packets/superp/superp.pcap, packet #7
Nodes:
	Ether node
	SNUH
		Source address: 4660 (0x1234)
		Destination address: 17185 (0x4321)
		Next header: 253 (0xfd)
		Traffic class: 35 (0x23)
			DiffServ: 4 (0x4), ECN: 3 (0x3)
		Hop limit 14 (0xe)
		Flow label 3294 (0xcde)
	Packet Delivery Layer (PDL)
		Destination CID: 4660 (0x1234)
		Receive window: 22136 (0x5678)
		Packet Sequence Number (PSN): 305419896 (0x12345678)
		ACK Packet Sequence Number (ACK PSN): 2271560481 (0x87654321)
		SACK bitmap (SACK bitmap): 2309737967 (0x89abcdef)
	Transaction Layer (TAL)
		End of message: yes
		Num operations: 5 (0x5)
		Opcode: Send to Queue Pair (12)
		Sequence Number (PSN): 4660 (0x1234)
		Transaction ID (XID): 34661 (0x8765)
		ACK Transaction ID (ACK-XID): 26505 (0x6789)
	Operation #0: SEND TO QP
		Queue pair 8873283 (0x876543)
		Block size: 128, Block offset: 66
		First data 240 (0xf0)
	Operation #1: SEND TO QP
		Queue pair 11259375 (0xabcdef)
		Block size: 128, Block offset: 194
		First data 241 (0xf1)
	Operation #2: SEND TO QP
		Queue pair 2311527 (0x234567)
		Block size: 128, Block offset: 322
		First data 242 (0xf2)
	Operation #3: SEND TO QP
		Queue pair 16702650 (0xfedcba)
		Block size: 128, Block offset: 450
		First data 243 (0xf3)
	Operation #4: SEND TO QP
		Queue pair 8811824 (0x867530)
		Block size: 128, Block offset: 578
		First data 244 (0xf4)
	** Okay node
Frame #0:
	Ethertype: 0x885b
	SUNH: 18'52->67'33
	Hash 0x1160e704
Summary:
	Return code: XDP2 stop okay
	Last node: superp_tal_send_to_qp_node
	Counters:
		Counter #0: 0
		Counter #1: 0
		Counter #2: 0
```
