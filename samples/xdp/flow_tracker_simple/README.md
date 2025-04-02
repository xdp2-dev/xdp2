<img src="../../../documentation/images/xdp2.png" alt="XDP2 logo"/>

XDP Flow Tracker Simple
=======================

Example simple XDP flow tracker. Parse packets to get the 4-tuple
and then lookup up in a table to track number of hits for the flow
in the map. Packets with source port equal to 22 are tracked


Building
--------

The build process needs the LLVM library. Set **LD_LIBRARY_PATH** accordingly
like below (replace */opt/riscv64/lib* with the appropriate path):
```
export LD_LIBRARY_PATH=/opt/riscv64/lib
```

Build in the *samples/parser/offset_parser* directory like below
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
14: array  name flow_tra.data  flags 0x400
	key 4B  value 8B  max_entries 1  memlock 8192B
	btf_id 217
15: array  name flow_tra.rodata  flags 0x480
	key 4B  value 752B  max_entries 1  memlock 8192B
	btf_id 217  frozen
```

The flow tracker map is */sys/fs/bpf/tc/globals/flowtracker* which has ID #8.
We can now dump the flow tracker map:

```
$ sudo bpftool map dump id 8
Found 0 elements
```

Now telnet to 127.0.0.1:22 to generate some packets with source port 22.
```
Trying 127.0.0.1...
Connected to 127.0.0.1.
Escape character is '^]'.
SSH-2.0-OpenSSH_8.9p1 Ubuntu-3ubuntu0.13

<hit return>
Invalid SSH identification string.
Connection closed by foreign host.
```

Dump the map again.

```
$ sudo bpftool map dump id 8
key: 7f 00 00 01 7f 00 00 01  00 16 9f c2 06 00 00 00  value: 08 00 00 00 00 00 00 00
Found 1 element
```

Now we see an entry in the map.

The first four bytes of the key are the source address, and the next for are
the destination address. For all entries both of these are set to 0x7f000001
which is the loopback address. The next two bytes are the source port followed
by the destination port. These are little endian so that for example the
source port of the last entry is 0x4eba (22) and the destination port is
0xc29f (49823). The eight bytes of the value count show eight packets were
received  for the flow (the value shown is little endian for 8).

Perform another telnet to 127.0.0.1:22.

```
$ sudo bpftool map dump id 8
key: 7f 00 00 01 7f 00 00 01  00 16 9f c2 06 00 00 00  value: 08 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  00 16 ce 86 06 00 00 00  value: 07 00 00 00 00 00 00 00
```

Now we see two flows in the tracker. Both have source port 22 and the new
flow shows seven packets were tracked.
