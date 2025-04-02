<img src="../../../documentation/images/xdp2.png" alt="XDP2 logo"/>

Offset parser example
=====================

Simple parser that parses UDP and TCP in IPv4 and IPv6 and extracts the
network layer and transport layer offsets. This builds an optimized and
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

Build in the *samples/parser/offset_parser* directory like below
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
$ ./parser ~/xdp2/data/pcaps/tcp_ipv6.pcap
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
```

Run the optimized parser variant like below (replace the pcap file as
desired):
```
$ ./parser -O ~/xdp2/data/pcaps/tcp_ipv6.pcap
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
Network offset: 14
Transport offset: 54
```
