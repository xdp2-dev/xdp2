<img src="../images/xdp2.png" alt="XDP2 logo"/>

Falcon Transport
================

This document describes the (initial) implementation of the Falcon Transport
protocol. This is an ongoing project. The ultimate goal is to provide
a first class open source implementation of Falcon.

Current support
===============

The features currently supported are:

* Standard include files with header formats and constant definitions for
  Falcon
* Packet tools to make a variety of Falcon protocol format
  samples in .pcap files
* Support in **parse_dump** to parse Falcon packets

Packet formats
==============

Packet formats for Falcon are in
[include/falcon/falcon.h](../../src/include/falcon/falcon.h).

Conformance
-----------

The packet formats should cover all those described in Section 7 of the
[Falcon Transport Protocol Specification Revision 0.9](https://github.com/opencomputeproject/OCP-NET-Falcon/blob/main/OCP%20Specification_%20Falcon%20Transport%20Protocol_Rev0.9%202024-March-04.pdf)

UDP port number
---------------

There does not see to be a UDP port number assignment for Falcon yet, so we
set a default value of 7777. The port number for Falcon is defined as
*FALCON_UDP_PORT_NUM* in
[falcon.h](../../src/include/falcon/falcon.h). The value may be overridden by
setting *config-defines*. For instance, the port number can be changed to
33333 by running **configure** in the src directory as:
```
./configure -config-defines "-DFALCON_UDP_PORT_NUM=33333"
```
parse_dump
==========

Parse dump support is added via
[test/parse_dump/falcon_parse.h](../../src/test/parse_dump/falcon_parse.h).
That file creates all the parse nodes for Falcon. The entry point
into Falcon parsing is **falcon_base_node**, that is set as the target node
for the Falcon UDP port number in the main UDP parse_dump protocol table (in
[parser.c](../../src/test/parse_dump/parser.c)).

pcap samples
============

.pcap files with samples of Falcon packets are built from tools in
[tools/packets/falcon](../../src/tools/packets/falcon).
**falcon.pcap** contains sample for Falcon packets.

Running parse_dump
==================

**parse_dump** can be run with the .pcap files from the Falcon packets tool
[directory](../../src/tools/packets/falcon).
From **src/test/parse_dump** we can parse packets from the Falcon .pcap file:
```
$ ./parse_dump -c 1 -v 10 -U ../../tools/packets/falcon/falcon.pcap
------------------------------------
Nodes:
	Ether node
	IP overlay node
	IPv4 check node
	IP overlay node by key
	IPv4 node
	UDP node
	Falcon base node
	Falcon version 1
		Falcon pull data
		Version: 1
		Dest cid: 1193046 (0x123456)
		Dest function: 9022413 (0x89abcd)
		Protocol type: NVMe (3)
		Packet type: Pull Data (3)
		Ack req: no
		Recv data psn: 2271555738 (0x8765309a)
		Recv req psn: 4275878552 (0xfedcba98)
		Psn: 826430308 (0x31425364)
		Req seqno: 4293844428 (0xffeeddcc)
	** Okay node
Frame #0:
	Ethertype: 0x0800
	IPv4: 192.168.1.1:8675->192.168.1.2:33333
	IP protocol 17 (udp)
	Hash 0x8e444435
Summary:
	Return code: XDP2 stop okay
	Last node: falcon_pull_data
	Counters:
		Counter #0: 1
		Counter #1: 1
		Counter #2: 0

```
