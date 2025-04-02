<img src="../../../documentation/images/xdp2.png" alt="XDP2 logo"/>

Flow tracker TLVs example
=========================

Flow tracker XDP sample program with TLVs parsing. This defines an XDP
program to parse packets to get the 4-tuple and then lookup up in a
table to track number of hits for the flow. TCP packets are tracked.
The parser parses various TCP options and saves fields in the metadata

Building
--------

The build process needs the LLVM library. Set **LD_LIBRARY_PATH** accordingly
like below (replace */opt/riscv64/lib* with the appropriate path):
```
export LD_LIBRARY_PATH=/opt/riscv64/lib
```

Build in the *samples/xdp/flow_tracker_tlvs* directory like below
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
 sudo bpftool map -f
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
	key 4B  value 1840B  max_entries 1  memlock 8192B
	btf_id 211  frozen
```

The flow tracker map is */sys/fs/bpf/tc/globals/flowtracker* which has ID #8.
We can now dump the flow tracker map:

```
$ sudo bpftool map dump id 8
key: 7f 00 00 01 7f 00 00 01  00 00 00 00 06 00 00 00  value: 58 01 00 00 00 00 00 00
```

Generate some more traffic (telnet to 127.0.0.1:22). Look at the map again

```
$ sudo bpftool map dump id 8
key: 7f 00 00 01 7f 00 00 01  14 e9 14 e9 06 00 00 00  value: 6e 00 00 00 00 00 00 00
key: 7f 00 00 01 7f 00 00 01  00 00 00 00 06 00 00 00  value: 3e 08 00 00 00 00 00 00
key: ff ff ff ff ff ff ff ff  ff ff ff ff ff 00 00 00  value: 04 00 00 00 00 00 00 00
```

The first four bytes of the key are the source address, and the next for are
the destination address. For all entries both of these are set to 0x7f000001
which is the loopback address. The next two bytes are the source port followed
by the destination port (these are little endian). The last entry is an
"error" entry. This was created when the parser encountered a protocol it
can't parse and it's configured to exit parsing with an error.
