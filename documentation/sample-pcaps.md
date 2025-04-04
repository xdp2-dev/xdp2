Sample pcaps
============

Different compiler versions (gcc, clang) can have impact on the
optimization of the code. We recommend newer versions of
gcc or clang.

The directory src/test/parser contains the script *perfscript.sh*
which will run performance testing on several pcaps and compares
results for flow dissector, parselite, xdp2, and xdp2 optimized
by the compiler tool.

The script runs each parser against pcaps found in directory
data/pcaps/

# Methodology

The test is run 3 times for each pcap file and parser, the first two
result are ignored and the last one is reported.

Each test execution run each packet 1 million times and averaged the time it
took to run the parser call.

# Sample pcaps

There are several test pcaps included. Each pcap has different
packet counts.

## ICMPv4

PCAP icmp_ipv4 has 6 packets with the following protocol stack:

```
/--------\
|Ethernet|
\--------/
/--------\
|  IPv4  |
\--------/
/--------\
|  ICMP  |
\--------/
```

## ICMPv6

PCAP icmp_ipv6 has 6 packets with the following protocol stack:

```
/--------\
|Ethernet|
\--------/
/--------\
|  IPv6  |
\--------/
/--------\
|  ICMP  |
\--------/
```

## TCPv4

PCAP tcp_ipv4 has 11 packets with the following protocol stack:

```
/--------\
|Ethernet|
\--------/
/--------\
|  IPv4  |
\--------/
/--------\
|  TCP   |
\--------/
```

## TCPv6

PCAP tcp_ipv6 has 12 packets with the following protocol stack:

```
/--------\
|Ethernet|
\--------/
/--------\
|  IPv6  |
\--------/
/--------\
|  TCP   |
\--------/
```

## 6in4 PCAP

PCAP 6in4 has 20 packets with the following protocol stack:

```
/--------\
|Ethernet|
\--------/
/--------\
| 802.1Q |
\--------/
/--------\
| PPPoE  |
\--------/
/--------\
|   PPP  |
\--------/
/--------\
|  IPv4  |
\--------/
/--------\
|  IPv6  |
\--------/
/--------\
|  TCP   |
\--------/
```

## 6to4 PCAP

PCAP 6to4 has 5 packets with the following protocol stack:

```
/--------\
|Ethernet|
\--------/
/--------\
| PPPoE  |
\--------/
/--------\
|   PPP  |
\--------/
/--------\
|  IPv4  |
\--------/
/--------\
|  IPv6  |
\--------/
/--------\
|  TCP   |
\--------/
```

## IPIP PCAP

PCAP IPIP has 10 packets with the following protocol stack:

```
/--------\
|Ethernet|
\--------/
/--------\
|  IPv4  |
\--------/
/--------\
|  IPv4  |
\--------/
/--------\
| ICMPv4 |
\--------/
```

## L2TP PCAP

PCAP L2TP has 38 packets with the following protocol stack:

```
/--------\
|Ethernet|
\--------/
/--------\
|  IPv4  |
\--------/
/--------\
|  L2TP  |
\--------/
```

## L7_L2TP PCAP

PCAP L7_L2TP has 4 packets with the following protocol stack:

```
/--------\
|Ethernet|
|--------|
|  PPPoE |
|--------|
|  PPP   |
|--------|
|  IPv4  |
|--------|
|  UDP   |
|--------|
|  L2TP  |
|--------|
|  PPP   |
\--------/
```

## QinQ PCAP

PCAP QinQ has 2 packets with the following protocol stack:

```
/--------\
|Ethernet|
|--------|
| 802.1Q |
|--------|
| 802.1Q |
|--------|
|  ARP   |
\--------/
```

## VLAN ICMP PCAP

PCAP VLAN ICMP has 1 packet with the following protocol stack:

```
/--------\
|Ethernet|
|--------|
| 802.1Q |
|--------|
| 802.1Q |
|--------|
|  IPv4  |
|--------|
| ICMPv4 |
\--------/
```

## VXLAN PCAP

PCAP VXLAN has 2 packets with the following protocol stack:

```
/--------\
|Ethernet|
|--------|
|  IPv4  |
|--------|
|  UDP   |
|--------|
| VXLAN  |
|--------|
|Ethernet|
|--------|
|  ARP   |
\--------/
```

And 8 packets with the following protocol stack:

```
/--------\
|Ethernet|
|--------|
|  IPv4  |
|--------|
|  UDP   |
|--------|
| VXLAN  |
|--------|
|Ethernet|
|--------|
|  IPv4  |
|--------|
| ICMPv4 |
\--------/
```

