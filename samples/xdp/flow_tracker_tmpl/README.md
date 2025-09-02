<img src="../../../documentation/images/xdp2.png" alt="XDP2 logo"/>

XDP Flow Tracker with Parser Program Template
=============================================

Simple XDP flow tracker using a template to generate and XDP program.
Parse packets to get the 4-tuple and then lookup up in a table to track
number of hits for the flow. TCP packets are tracked. This variant
calls XDP2_XDP_MAKE_PARSER_PROGRAM t provide the XDP template program
code and structures

Building
--------

The build process needs the LLVM library. Set **LD_LIBRARY_PATH** accordingly
like below (replace */opt/riscv64/lib* with the appropriate path):
```
export LD_LIBRARY_PATH=/opt/riscv64/lib
```

Build in the *samples/dp/flow_tracker_tmpl* directory like below
(replace *~/xdp2/install* with the proper path name to the XDP2 install
directory):
```
make XDP2DIR=~/xdp2/install
```

Running
-------

Load the XDP program. In this example we attach to the loopback interface (lo):

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
	key 4B  value 752B  max_entries 1  memlock 8192B
	btf_id 211  frozen
```

The flow tracker map is */sys/fs/bpf/tc/globals/flowtracker* which has ID #8.
We can now dump the flow tracker map:

```
$ sudo bpftool map dump id 8
key: 7f 00 00 01 7f 00 00 01  40 3b a0 e0 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a1 3c 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a1 30 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a0 d2 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a1 3e 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a1 3e 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a1 06 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a0 e2 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a0 c6 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a1 28 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a1 56 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a1 1c 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a0 d2 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a1 2c 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a0 e2 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a1 4a 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a1 0c 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a0 f6 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a0 c6 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a1 64 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a1 06 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a1 2c 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a1 56 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a1 3c 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a1 4a 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a1 64 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a1 30 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a0 e0 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a1 1c 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a0 f6 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  a1 28 40 3b 06 00 00 00  value: 01 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  40 3b a1 0c 06 00 00 00  value: 01 00 00 00 00 00 00 00
Found 32 elements
```
This shows that we consumed all the thirty-two entries so that the table is
full.

The first four bytes of the key are the source address, and the next for are
the destination address. For all entries both of these are set to 0x7f000001
which is the loopback address. The next two bytes are the source port followed
by the destination port. These are little endian so that for example the
source port of the last entry is 0x3b40 (15168) and the destination port is
0x06a1 (1697). The eight bytes of the value count the number of hits for
the flow (the value shown is little endian).

