<img src="../../../documentation/images/xdp2.png" alt="XDP2 logo"/>

XDP Flow Tracker Combo
======================

This is a an example of using the same XDP2 parser compiled in both
a regular program and an XDP program. The regular program to parse
TCP and UDP packet over IPv4 and IPv6 and then print tuple information

Building
--------

The build process needs the LLVM library. Set **LD_LIBRARY_PATH** accordingly
like below (replace */opt/riscv64/lib* with the appropriate path):
```
export LD_LIBRARY_PATH=/opt/riscv64/lib
```

Build in the *samples/xdp/flow_tracker_combo* directory like below
(replace *~/xdp2/install* with the proper path name to the XDP2 install
directory):
```
make XDP2DIR=~/xdp2/install
```

Running the XDP program
-----------------------

Load the XDP program. In this example we use the loopback interface (lo):

```
$ sudo ip link set dev lo xdp obj flow_tracker.xdp.o
$
```

Now get the map information:

```
$ sudo bpftool map -f
2: prog_array  name hid_jmp_table  flags 0x0
	key 4B  value 4B  max_entries 1024  memlock 8576B
	owner_prog_type tracing  owner jited
3: hash  flags 0x0
	key 9B  value 1B  max_entries 500  memlock 46944B
	pinned /sys/fs/bpf/snap/snap_snap-store_ubuntu-software
6: percpu_array  name ctx_map  flags 0x0
	key 4B  value 232B  max_entries 2  memlock 7824B
	pinned /sys/fs/bpf/tc/globals/ctx_map
7: prog_array  name parsers  flags 0x0
	key 4B  value 4B  max_entries 1  memlock 392B
	owner_prog_type xdp  owner jited
	pinned /sys/fs/bpf/tc/globals/parsers
8: hash  name flowtracker  flags 0x0
	key 16B  value 8B  max_entries 32  memlock 5568B
	pinned /sys/fs/bpf/tc/globals/flowtracker
9: array  name flow_tra.data  flags 0x400
	key 4B  value 8B  max_entries 1  memlock 8192B
	btf_id 211
10: array  name flow_tra.rodata  flags 0x480
	key 4B  value 968B  max_entries 1  memlock 8192B
	btf_id 211  frozen
```

The flow tracker map is */sys/fs/bpf/tc/globals/flowtracker* which has ID #8.
We can now dump the flow tracker map:

```
$ sudo bpftool map dump id 8
key: 7f 00 00 01 7f 00 00 01  ba 82 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 12 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 82 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 4e 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 4c 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 14 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 2a 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 14 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 2a 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 34 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 76 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 40 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 34 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 84 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 70 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 68 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 36 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 36 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 40 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 5a 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 70 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 68 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 8a 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 76 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 1a 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 8a 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 4c 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 1a 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 12 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 84 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b ba 5a 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  ba 4e 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
Found 32 elements
```

This shows that we consumed all the thirty-two entries so that the table is
full.

The first four bytes of the key are the source address, and the next for are
the destination address. For all entries both of these are set to 0x7f000001
which is the loopback address. The next two bytes are the source port followed
by the destination port. These are little endian so that for example the
source port of the last entry is 0x4eba (20154) and the destination port is
0x063b (1595). The eight bytes of the value count the number of hits for
the flow (the value shown is little endian).


Running the plain program
-------------------------

Set **LD_LIBRARY_PATH** to include the lib directory in the XDP2 install path
like below (replace *~/xdp2/install/lib* with the appropriate path)

```
export LD_LIBRARY_PATH=~/xdp2/install/lib
```

Run the program from the command line like below (replace the pcap file as
desired):
```
$ ./flow_parser ~/xdp2/data/pcaps//tcp_ipv6.pcap
IPv6: ::1:51648->::1:631
IPv6: ::1:631->::1:51648
IPv6: ::1:51648->::1:631
IPv6: ::1:51648->::1:631
IPv6: ::1:631->::1:51648
IPv6: ::1:631->::1:51648
IPv6: ::1:51650->::1:631
IPv6: ::1:631->::1:51650
IPv6: ::1:51650->::1:631
IPv6: ::1:51650->::1:631
IPv6: ::1:631->::1:51650
```

This shows the four tuple for the TCP packets in the pcap file

We can also run the optimized parser variant (which should give the same
output).

```
$ ./flow_parser -O ~/xdp2/data/pcaps//tcp_ipv6.pcap
IPv6: ::1:51648->::1:631
IPv6: ::1:631->::1:51648
IPv6: ::1:51648->::1:631
IPv6: ::1:51648->::1:631
IPv6: ::1:631->::1:51648
IPv6: ::1:631->::1:51648
IPv6: ::1:51650->::1:631
IPv6: ::1:631->::1:51650
IPv6: ::1:51650->::1:631
IPv6: ::1:51650->::1:631
IPv6: ::1:631->::1:51650
IPv6: ::1:631->::1:51650
```
