<img src="../../../documentation/images/xdp2.png" alt="XDP2 logo"/>

Extract 4-tuple from IPv4 parser example
========================================

Simple parser that parses UDP and TCP in IPv4 and extracts the 4-tuple
(IP addresses and ports). This builds an optimized and
non-optimized parser.

Run by:
```
     ./parser [-O] <pcap-file>
```

Building
--------

The build process needs the LLVM library. Set **LD_LIBRARY_PATH** accordingly
like below (replace */opt/riscv64/lib* with the appropriate path):
```
export LD_LIBRARY_PATH=/opt/riscv64/lib
```

Build in the *samples/parser/ports_parser* directory like below
(replace *~/xdp2/install* with the proper path name to the XDP2 install
directory):
```
make XDP2DIR=~/xdp2/install
```

Running
-------

Set **LD_LIBRARY_PATH** to include the lib directory in the XDP2 install path
like below (replace *~/xdp2/install/lib* with the appropriate path)
```
export LD_LIBRARY_PATH=~/xdp2/install/lib
```

Note that all the non-optimized and optimized parser variants should produce
identical output for the same pcap file.

Run the non-optimized parser variant like below (replace the pcap file as
desired):
```
./parser ~/xdp2/data/pcaps/tcp_ipv4.pcap
Packet 0: 10.0.2.15:44188 -> 10.0.2.15:43
Packet 1: 192.0.47.59:43 -> 192.0.47.59:44188
Packet 2: 10.0.2.15:44188 -> 10.0.2.15:43
Packet 3: 10.0.2.15:44188 -> 10.0.2.15:43
Packet 4: 192.0.47.59:43 -> 192.0.47.59:44188
Packet 5: 192.0.47.59:43 -> 192.0.47.59:44188
Packet 6: 10.0.2.15:44188 -> 10.0.2.15:43
Packet 7: 192.0.47.59:43 -> 192.0.47.59:44188
Packet 8: 10.0.2.15:44188 -> 10.0.2.15:43
Packet 9: 10.0.2.15:44188 -> 10.0.2.15:43
Packet 10: 192.0.47.59:43 -> 192.0.47.59:44188
```

Run the optimized parser variant like below (replace the pcap file as
desired):
```
./parser -O ~/xdp2/data/pcaps/tcp_ipv4.pcap
Packet 0: 10.0.2.15:44188 -> 10.0.2.15:43
Packet 1: 192.0.47.59:43 -> 192.0.47.59:44188
Packet 2: 10.0.2.15:44188 -> 10.0.2.15:43
Packet 3: 10.0.2.15:44188 -> 10.0.2.15:43
Packet 4: 192.0.47.59:43 -> 192.0.47.59:44188
Packet 5: 192.0.47.59:43 -> 192.0.47.59:44188
Packet 6: 10.0.2.15:44188 -> 10.0.2.15:43
Packet 7: 192.0.47.59:43 -> 192.0.47.59:44188
Packet 8: 10.0.2.15:44188 -> 10.0.2.15:43
Packet 9: 10.0.2.15:44188 -> 10.0.2.15:43
Packet 10: 192.0.47.59:43 -> 192.0.47.59:44188
```
