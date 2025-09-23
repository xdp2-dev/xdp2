<img src="../images/xdp2.png" alt="XDP2 logo"/>

Ultra Ethernet Transport
========================

This document describes the (initial) implementation of Ultra Ethernet
Transport of UET. This is an ongoing project. The ultimate goal is to provide
a first class open source implementation of UET.

Current support
===============

The features currently supported are:

* Standard include files with header formats and constant definitions for
  PDS and SES
* Packet tools to make a variety of PDS and SES protocol format
  samples in .pcap fiels
* Support in **parse_dump** to parse UET packets

Packet formats
==============

Packet formats for Packet Delivery Sub-layer, or PDS, are in
[include/uet/pds.h](../../src/include/uet/pds.h), and packet
formats for Semantic Sub-layer, or SES, are in
[include/uet/ses.h](../../src/include/uet/ses.h).

Conformance
-----------

The packet formats should cover all those described in Section 3
of the [Ultra Ethernet Specification v1.0.1](https://ultraethernet.org/wp-content/uploads/sites/20/2025/09/Updated-9.5.25-UE-Specification-1.0.1.pdf)

UDP port number
---------------

The UDP port number for UET is 4739 per
[IANA assigned UDP port numbers](https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml?search=ultra+etherne).
The port number for UET is defined as *UET_UDP_PORT_NUM* in
[pds.h](../../src/include/uet/pds.h). The value may be overridden by setting
*config-defines*. For instance, the port number can be changed to 22222 by
running **configure** in the src directory as:
```
./configure -config-defines "-DUET_UDP_PORT_NUM=22222"
```

parse_dump
==========

Parse dump support is added via
[test/parse_dump/uet_parse.h](../../src/test/parse_dump/uet_parse.h).
That file creates all the parse nodes for PDS and SES. The entry point
into UET parsing is **uet_base**, that is set as the target node for the
UET UDP port number in the main UDP parse_dump protocol table (in
[parser.c](../../src/test/parse_dump/parser.c)).

pcap samples
============

.pcap files with samples of UET packets are built from tools in
[tools/packets/uet](../../src/tools/packets/uet). Two .pcap files
are created. **uet_pds.pcap** contains sample that focus on the various
PDS packet formats, and  **uet_ses.pcap** contains sample that focus on the
various SES formats.

Running parse_dump
==================

**parse_dump** can be run with the .pcap files from the UET packets tool
[directory](../../src/tools/packets/uet).
From src/test/parse_dump we can parse packets from the PDS .pcap files:
```
$ ./parse_dump -c 1 -v 10 -U ../../tools/packets/uet/uet_pds.pcap
------------------------------------
Nodes:
	Ether node
	IP overlay node
	IPv4 check node
	IP overlay node by key
	IPv4 node
	UDP node
	UET PDS RUD request CC
		Type: RUD request with congestion control (13)
		Next header: Request standard (3)
		Retransmission: yes
		Ack requested: no
		SYN: no
		CLear PSN offset: 4660 (0x1234)
		PSN: 2557891634 (0x98765432)
		SPDcid: 13398 (0x3456)
		DPDcid: 39612 (0x9abc)
		--- CC
		CCC ID: 119 (0x77)
		Credit target 8943462 (0x887766)
	UET SES Read std SOM
		Version: 0
		Opcode: Read (2)
		Delivery complete: yes
		Initiator error: no
		Relative addressing: yes
		Header data present: no
		End of message: yes
		Start of message: yes
		Message ID: 4660 (0x1234)
		RI generation: 119 (0x77)
		Job ID: 11259375 (0xabcdef)
		PID on FEP: 1656 (0x678)
		Resource index: 2475 (0x9ab)
		--- SOM
		Buffer offset: 18364758544493064720 (0xfedcba9876543210)
		Initiator: 4275878552 (0xfedcba98)
		Memory key/match bits: 1234605616436508552 (0x1122334455667788)
		Payload length: 7645 (0x1ddd)
		Message offset: 4275878552 (0xfedcba98)
		Request length: 2575857510 (0x99887766)
		--- Read
	** Okay node
Frame #0:
	Ethertype: 0x0800
	IPv4: 192.168.1.2:35433->192.168.1.2:22222
	IP protocol 17 (udp)
	Hash 0xc8b67061
Summary:
	Return code: XDP2 stop okay
	Last node: uet_ses_request_som_read_std
	Counters:
		Counter #0: 1
		Counter #1: 1
		Counter #2: 0
```

And we can parse packets from the PDS .pcap files:
```
./parse_dump -c 1 -v 10 -U ../../tools/packets/uet/uet_ses.pcap
------------------------------------
Nodes:
	Ether node
	IP overlay node
	IPv4 check node
	IP overlay node by key
	IPv4 node
	UDP node
	UET PDS RUD request
		Type: RUD Request (RUD_REQ) (2)
		Next header: Request small (1)
		Retransmission: yes
		Ack requested: no
		SYN: no
		CLear PSN offset: 4660 (0x1234)
		PSN: 2557891634 (0x98765432)
		SPDcid: 13398 (0x3456)
		DPDcid: 39612 (0x9abc)
	UET SES request small Atomic
		Version: 0
		Opcode: Atomic (3)
		Delivery complete: yes
		Initiator error: no
		Relative addressing: yes
		Header data present: no
		End of message: yes
		Start of message: yes
		Message ID: 4660 (0x1234)
		RI generation: 119 (0x77)
		Job ID: 11259375 (0xabcdef)
		PID on FEP: 1656 (0x678)
		Resource index: 2475 (0x9ab)
		Buffer offset: 10986060917299893298 (0x9876543298765432)
	UET SES atomic operation
	Atomic extension header
		Atomic code: Bitwise AND (8)
		Atomic datatype: float (10)
		Cacheable: yes
		CPU coherent: yes
		Vendor defined: 5
	** Okay node
Frame #0:
	Ethertype: 0x0800
	IPv4: 192.168.1.1:8675->192.168.1.2:22222
	IP protocol 17 (udp)
	Hash 0xd30e3def
Summary:
	Return code: XDP2 stop okay
	Last node: uet_ses_atomic_node
	Counters:
		Counter #0: 1
		Counter #1: 1
		Counter #2: 0
```


