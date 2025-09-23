<img src="../images/xdp2.png" alt="XDP2 logo"/>

Scale Up Ethernet (SUE)
=======================

This document describes the (initial) implementation of the Scale Up
Ethernet, or SUE, protocol. This is an ongoing project. The ultimate goal
is to provide a first class open source implementation of SUE although it
should be noted the SUE specification is not complete.

Current support
===============

The features currently supported are:

* Standard include files with header formats and constant definitions for
  SUE
* Packet tools to make a variety of SUE protocol format
  samples in .pcap files
* Support in **parse_dump** to parse SUE packets

Packet formats
==============

Packet formats for SUE are in
[include/sue/sue.h](../../src/include/sue/sue.h).

Conformance
-----------

The packet formats should cover all those described in Section 6.3.3 for
the SUE Reliability Header in
[Scale Up Ethernet (SUE)](https://docs.broadcom.com/doc/scale-up-ethernet-framework)

UDP port number
---------------

There does not see to be a UDP port number assignment for SUE yet, so we
set a default value of 7777. The port number for SUE is defined as
*SUE_UDP_PORT_NUM* in
[sue.h](../../src/include/sue/sue.h). The value may be overridden by
setting *config-defines*. For instance, the port number can be changed to
4444 by running **configure** in the src directory as:
```
./configure -config-defines "-DSUE_UDP_PORT_NUM=44444"
```
parse_dump
==========

Parse dump support is added via
[test/parse_dump/sue_parse.h](../../src/test/parse_dump/sue_parse.h).
That file creates all the parse nodes for SUE. The entry point
into SUE parsing is **sue_base_node**, that is set as the target node
for the SUE UDP port number in the main UDP parse_dump protocol table (in
[parser.c](../../src/test/parse_dump/parser.c)).

pcap samples
============

.pcap files with samples of SUE packets are built from tools in
[tools/packets/sue](../../src/tools/packets/sue).
**sue.pcap** contains sample for SUE packets.

Running parse_dump
==================

**parse_dump** can be run with the .pcap files from the SUE packets tool
[directory](../../src/tools/packets/sue).
From **src/test/parse_dump** we can parse packets from the SUE .pcap files:
```
$ ./parse_dump -c 1 -v 10 -U ../../tools/packets/sue/sue.pcap
------------------------------------
Nodes:
	Ether node
	IP overlay node
	IPv4 check node
	IP overlay node by key
	IPv4 node
	UDP node
	SUE base node
	SUE version 0
		SUE APSN is Invalid2
		Version: 0
		Opcode: Invalid2 (3)
		XPUID: 291 (0x123)
		PSN: 26505 (6789)
		Virtual channel: 3 (3)
		Partition: 837 (345)
		ACK PSN: 17767 (4567)
	** Okay node
Frame #0:
	Ethertype: 0x0800
	IPv4: 192.168.1.1:8675->192.168.1.2:44444
	IP protocol 17 (udp)
	Hash 0x291da34e
Summary:
	Return code: XDP2 stop okay
	Last node: sue_rh_invalid2
	Counters:
		Counter #0: 1
		Counter #1: 1
		Counter #2: 0
```
