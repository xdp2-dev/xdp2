<img src="images/xdp2.png" alt="XDP2 logo"/>

Parse dump program
==================

Parse-dump in [test/parse_dump](../src/test/parse_dump) is a program
for exercising the XDP2 parser. [parser.c](../src/test/parse_dump/parser.c)
defines ans XP2 parser for many protocols. A set of pcap files for testing
is in [data/pcaps](../data/pcaps).


The usage of the parse-dump program is:
```

Run: ./parse_dump [ -c <test-count> ] [ -v <verbose> ]
                  [ -I <report-interval> ] [ -C <cli_port_num> ]
                  [-R] [-d] [ -P <prompt-color>] [-U] <pcap_file> ...
```
Arguments are:
* **-c \<test-count\>** gives the number of tests to run,
* **-v \<verbose\>** is the verbose level
* **-I \<report-interval\>** gives the interval to report the test count ran
* **-C \<cli-port-num\>** gives the port number for the CLI
* **-R** indicates to seed the random number generator
* **-d** enable parser debug
* **-P <prompt-color>** prompt color for CLI
* **-U** use terminal colors for output
* **\<pacp_file\>** a list of one or more pcap files

Examples
========

We assume that parse_dump is run from the test/parse_dump directory.
parse_dump is also installed in install/bin/parse_dump so that directory
could be used as well.

Parse simple TCP/IPv6
----------------------

Parsing TCP/IPv6 packets is shown below:

```
$ ./parse_dump -v 10 ~/xdp2/data/pcaps/tcp_ipv6.pcap 
------------------------------------
Nodes:
	Ether node
	IP overlay node
	IPv6 check node
	IP overlay node by key
	IPv6 node
	TCP node
		TCP MSS option 2, length 4
		TCP sack permitted option 4, length 2
		TCP Timestamp option 8, length 10
		TCP window scaling option 3, length 3
	** Okay node
Frame #0:
	Ethertype: 0x86dd
	IPv6: ::1:51648->::1:631
	IP protocol 6 (tcp)
	Hash 0xb70b38bb
Summary:
	Return code: XDP2 stop okay
	Last node: tcp_node
	TCP timestamps value: 1887522685, echo 0
	TCP MSS: 65476
	TCP window scaling: 0
	Counters:
		Counter #0: 0
		Counter #1: 0
		Counter #2: 0
------------------------------------
Nodes:
	Ether node
	IP overlay node
	IPv6 check node
	IP overlay node by key
	IPv6 node
	TCP node
		TCP MSS option 2, length 4
		TCP sack permitted option 4, length 2
		TCP Timestamp option 8, length 10
		TCP window scaling option 3, length 3
	** Okay node
Frame #0:
	Ethertype: 0x86dd
	IPv6: ::1:631->::1:51648
	IP protocol 6 (tcp)
	Hash 0x29b12cd8
Summary:
	Return code: XDP2 stop okay
	Last node: tcp_node
	TCP timestamps value: 1887522685, echo 1887522685
	TCP MSS: 65476
	TCP window scaling: 0
	Counters:
		Counter #0: 0
		Counter #1: 0
		Counter #2: 0

<<< and so on >>>
```

Verbose output is set to 10 which gives a fair amount of detail. The
*Nodes:* section list the parse nodes visited and the sub-nodes like show
above for TCP options.

*Frame #* sections give the information collected for each metadata frame
in a packet. The *Summary:* sections provides a summary of the data
collected.

IPinIP encapsulation
---------------------

This show an example of an encapsulation protocol.
```

$ ./parse_dump -v 10 ~/xdp2/data/pcaps/ipip.pcap 
------------------------------------
Nodes:
	Ether node
	IP overlay node
	IPv4 check node
	IP overlay node by key
	IPv4 node
	IPv4 check node
	** At encapsulation node
	IPv4 node
	ICMPv4 node
	** Okay node
Frame #0:
	Ethertype: 0x0800
	IPv4: 10.0.0.1->10.0.0.2
	IP protocol 4 (ipencap)
	Hash 0x2d7c9824
Frame #1:
	IPv4: 1.1.1.1->2.2.2.2
	IP protocol 1 (icmp)
	Hash 0x8c9d1100
Summary:
	Return code: XDP2 stop okay
	Last node: icmpv4_node
	ICMP:
		Type: 8
		Code: 0
		Echo Request ID: 4 Seq: 0
	Counters:
		Counter #0: 2
		Counter #1: 1
		Counter #2: 0
```


PPTP
----

GRE-PPPT is an example of a more complex protocol layering.

```
$ ./parse_dump -v 10 ~/xdp2/data/pcaps/gre-pptp.pcap 
------------------------------------
Nodes:
	Ether node
	VLAN e8021Q
	IP overlay node
	IPv6 check node
	IP overlay node by key
	IPv6 node
	IPv4 check node
	** At encapsulation node
	IPv4 node
	GRE node
	GREv1 node
		GRE PPTP key
		GRE PPTP seq
		GRE PPTP ack
	** At encapsulation node
	PPP node
	IP overlay node
	IPv4 check node
	IP overlay node by key
	IPv4 node
	UDP node
	** Okay node
Frame #0:
	Ethertype: 0x8100
	IPv6: 2402:f000:1:8e01::5555->2607:fcd0:100:2300::b108:2a6b
	IP protocol 4 (ipencap)
	Hash 0x64c860bf
	VLAN #0
		VLAN ID: 100
		DEI: 0
		Priority: 0
		TCI: 0
		TPID: 0x88a8
		ENC PROTO: 0x86dd
Frame #1:
	IPv4: 16.0.0.200->192.52.166.154
	IP protocol 47 (gre)
	Hash 0x0a7fbe1d
	GRE version 1
		Payload length: 103
		Call ID: 6016
		Sequence: 430001
		Ack: 539254
Frame #2:
	IPv4: 172.16.44.3:40768->8.8.8.8:53
	IP protocol 17 (udp)
	Hash 0xa8d7c087
Summary:
	Return code: XDP2 stop okay
	Last node: udp_node
	Counters:
		Counter #0: 2
		Counter #1: 1
		Counter #2: 0

<<< and so on >>>
```

Proto-bufs
----------

Here's an example showing parsing nested protobufs. 
```
$ ./parse_dump -v 10 ~/xdp2/data/pcaps/protobuf_in_udp.pcap 
------------------------------------
Nodes:
	Ether node
	IP overlay node
	IPv4 check node
	IP overlay node by key
	IPv4 node
	UDP node
	Protobufs2 node
		Protobuf name: 		
		Protobuf id: 123
		Protobuf email: 		
		Protobuf name: 		
		Protobuf id: 867
		Protobuf email: 		
	** Okay node
Frame #0:
	Ethertype: 0x0800
	IPv4: 192.168.1.100:1234->192.168.1.101:9999
	IP protocol 17 (udp)
	Hash 0xa8152f4d
Summary:
	Return code: XDP2 stop okay
	Last node: protobufs2_node
	Counters:
		Counter #0: 1
		Counter #1: 1
		Counter #2: 0
------------------------------------
Nodes:
	Ether node
	IP overlay node
	IPv4 check node
	IP overlay node by key
	IPv4 node
	UDP node
	Protobufs2 node
		Protobuf name: 		
		Protobuf id: 123
		Protobuf email: 		
			Protobufs1 phones
				Protobuf phone number: 		
				Protobuf phone type: 16
			Protobufs1 phones
				Protobuf phone number: 		
				Protobuf phone type: 16
			Protobufs1 phones
				Protobuf phone number: 		
				Protobuf phone type: 16
		Protobuf name: 		
		Protobuf id: 867
		Protobuf email: 		
			Protobufs1 phones
				Protobuf phone number: 		
				Protobuf phone type: 16
			Protobufs1 phones
				Protobuf phone number: 		
				Protobuf phone type: 16
			Protobufs1 phones
				Protobuf phone number: 		
				Protobuf phone type: 16
	** Okay node
Frame #0:
	Ethertype: 0x0800
	IPv4: 192.168.1.100:1234->192.168.1.101:9999
	IP protocol 17 (udp)
	Hash 0xa8152f4d
Summary:
	Return code: XDP2 stop okay
	Last node: protobufs2_node
	Counters:
		Counter #0: 1
		Counter #1: 1
		Counter #2: 0
```

High count tests
================

The **-c** option can be used to set a count for number of packets to
parse. Here's an example to parse ten million packets with no output.

```
./parse_dump -c 10000000 -I 1000000 ~/xdp2/data/pcaps/*.pcap
I: 0
I: 1000000
I: 2000000
I: 3000000
I: 4000000
I: 5000000
I: 6000000
I: 7000000
I: 8000000
I: 9000000
```

Optimized parser
================

When **--build-opt-parser** is configured (i.e. src/configure
--build-opt-parser) then the optimized parser is compiled. The C
code is compiled into *parser.p.c*. The **-O** option is used to invoke
the optimized parser like:

```
$ ./parse_dump -v 10 -O ~/xdp2/data/pcaps/tcp_ipv6.pcap 
------------------------------------
Nodes:
	Ether node
	IP overlay node
	IPv6 check node
	IP overlay node by key
	IPv6 node
	TCP node
		TCP MSS option 2, length 4
		TCP sack permitted option 4, length 2
		TCP Timestamp option 8, length 10
		TCP window scaling option 3, length 3
	** Okay node
Frame #0:
	Ethertype: 0x86dd
	IPv6: ::1:51648->::1:631
	IP protocol 6 (tcp)
	Hash 0xb70b38bb
Summary:
	Return code: XDP2 stop okay
	Last node: tcp_node
	TCP timestamps value: 1887522685, echo 0
	TCP MSS: 65476
	TCP window scaling: 0
	Counters:
		Counter #0: 0
		Counter #1: 0
		Counter #2: 0
```

The plain parser and optimized parser should always produce identical results.
This can be tested by:
```
$ echo; echo "*** Diffs";for i in ~/xdp2/data/pcaps/*.pcap; do bn=`basename $i`; ./parse_dump -v 10 $i > /tmp/pdiff.noopt; ./parse_dump -v 10 -O $i > /tmp/pdiff.opt; diff -q /tmp/pdiff.noopt /tmp/pdiff.opt > /dev/null; [ $? -eq 1 ] && echo $bn; done

*** Diffs
l7_l2tp.pcap
protobuf_in_udp.pcap
tcp_sack.pcap
vxlan.pcap
```

Note that there are four pcap files with a discrepancy between the optimized
and non-optimized parsers. This is a known bug.

We can also do a speed test to compare:
```
$ time ./parse_dump -c 100000000 -I 10000000 ~/xdp2/data/pcaps/*.pcap
I: 0
I: 10000000
I: 20000000
I: 30000000
I: 40000000
I: 50000000
I: 60000000
I: 70000000
I: 80000000
I: 90000000

real	0m27.548s
user	0m27.545s
sys	0m0.002s


$ time ./parse_dump -O -c 100000000 -I 10000000 ~/xdp2/data/pcaps/*.pcap
I: 0
I: 10000000
I: 20000000
I: 30000000
I: 40000000
I: 50000000
I: 60000000
I: 70000000
I: 80000000
I: 90000000

real	0m20.464s
user	0m20.458s
sys	0m0.005s
```
