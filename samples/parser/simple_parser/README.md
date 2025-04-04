<img src="../../../documentation/images/xdp2.png" alt="XDP2 logo"/>

Simple parser example
=====================

Example parser for TCP/IP with parsing timestamp option. There are
two variants: one that uses
[metadata templates](../../../src/include/xdp2/parser_metadata.h) and one
that doesn't. Both an optimized and non-optimized parser s built (run
optimized with -O)

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

Build in the *samples/parser/simple_parser* directory like below
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

Note that all the variants should produce identical output for the same
pcap file.

Run the no metadata templates and non-optimized parser variant like below
(replace the pcap file as desired):
```
$ ./parser_notmpl ~/xdp2/data/pcaps/tcp_ipv6.pcap
IPv6: ::1:51648->::1:631
	TCP timestamps value: 1887522685, echo 0
	Hash d3f87531
IPv6: ::1:631->::1:51648
	TCP timestamps value: 1887522685, echo 1887522685
	Hash ca63a2de
IPv6: ::1:51648->::1:631
	TCP timestamps value: 1887522685, echo 1887522685
	Hash d3f87531
IPv6: ::1:51648->::1:631
	TCP timestamps value: 1887522685, echo 1887522685
	Hash d3f87531
IPv6: ::1:631->::1:51648
	TCP timestamps value: 1887522685, echo 1887522685
	Hash ca63a2de
IPv6: ::1:631->::1:51648
	TCP timestamps value: 1887522688, echo 1887522685
	Hash ca63a2de
IPv6: ::1:51650->::1:631
	TCP timestamps value: 1887523538, echo 0
	Hash 512a939c
IPv6: ::1:631->::1:51650
	TCP timestamps value: 1887523538, echo 1887523538
	Hash b857973f
IPv6: ::1:51650->::1:631
	TCP timestamps value: 1887523538, echo 1887523538
	Hash 512a939c
IPv6: ::1:51650->::1:631
	TCP timestamps value: 1887523538, echo 1887523538
	Hash 512a939c
IPv6: ::1:631->::1:51650
	TCP timestamps value: 1887523538, echo 1887523538
	Hash b857973f
IPv6: ::1:631->::1:51650
	TCP timestamps value: 1887523541, echo 1887523538
	Hash b857973f
```

Run the no metadata templates and optimized parser variant like below
(replace the pcap file as desired):
```
$ ./parser_notmpl -O ~/xdp2/data/pcaps/tcp_ipv6.pcap 
IPv6: ::1:51648->::1:631
	TCP timestamps value: 1887522685, echo 0
	Hash d3f87531
IPv6: ::1:631->::1:51648
	TCP timestamps value: 1887522685, echo 1887522685
	Hash ca63a2de
IPv6: ::1:51648->::1:631
	TCP timestamps value: 1887522685, echo 1887522685
	Hash d3f87531
IPv6: ::1:51648->::1:631
	TCP timestamps value: 1887522685, echo 1887522685
	Hash d3f87531
IPv6: ::1:631->::1:51648
	TCP timestamps value: 1887522685, echo 1887522685
	Hash ca63a2de
IPv6: ::1:631->::1:51648
	TCP timestamps value: 1887522688, echo 1887522685
	Hash ca63a2de
IPv6: ::1:51650->::1:631
	TCP timestamps value: 1887523538, echo 0
	Hash 512a939c
IPv6: ::1:631->::1:51650
	TCP timestamps value: 1887523538, echo 1887523538
	Hash b857973f
IPv6: ::1:51650->::1:631
	TCP timestamps value: 1887523538, echo 1887523538
	Hash 512a939c
IPv6: ::1:51650->::1:631
	TCP timestamps value: 1887523538, echo 1887523538
	Hash 512a939c
IPv6: ::1:631->::1:51650
	TCP timestamps value: 1887523538, echo 1887523538
	Hash b857973f
IPv6: ::1:631->::1:51650
	TCP timestamps value: 1887523541, echo 1887523538
	Hash b857973f
```

Run the with metadata templates and non-optimized parser variant like below
(replace the pcap file as desired):
```
$ ./parser_tmpl ~/xdp2/data/pcaps/tcp_ipv6.pcap 
IPv6: ::1:51648->::1:631
	TCP timestamps value: 1887522685, echo 0
	Hash d3f87531
IPv6: ::1:631->::1:51648
	TCP timestamps value: 1887522685, echo 1887522685
	Hash ca63a2de
IPv6: ::1:51648->::1:631
	TCP timestamps value: 1887522685, echo 1887522685
	Hash d3f87531
IPv6: ::1:51648->::1:631
	TCP timestamps value: 1887522685, echo 1887522685
	Hash d3f87531
IPv6: ::1:631->::1:51648
	TCP timestamps value: 1887522685, echo 1887522685
	Hash ca63a2de
IPv6: ::1:631->::1:51648
	TCP timestamps value: 1887522688, echo 1887522685
	Hash ca63a2de
IPv6: ::1:51650->::1:631
	TCP timestamps value: 1887523538, echo 0
	Hash 512a939c
IPv6: ::1:631->::1:51650
	TCP timestamps value: 1887523538, echo 1887523538
	Hash b857973f
IPv6: ::1:51650->::1:631
	TCP timestamps value: 1887523538, echo 1887523538
	Hash 512a939c
IPv6: ::1:51650->::1:631
	TCP timestamps value: 1887523538, echo 1887523538
	Hash 512a939c
IPv6: ::1:631->::1:51650
	TCP timestamps value: 1887523538, echo 1887523538
	Hash b857973f
IPv6: ::1:631->::1:51650
	TCP timestamps value: 1887523541, echo 1887523538
	Hash b857973f
```

Run the with metadata templates and optimized parser variant like below
(replace the pcap file as desired):
```
$ ./parser_tmpl -O ~/xdp2/data/pcaps/tcp_ipv6.pcap 
IPv6: ::1:51648->::1:631
	TCP timestamps value: 1887522685, echo 0
	Hash d3f87531
IPv6: ::1:631->::1:51648
	TCP timestamps value: 1887522685, echo 1887522685
	Hash ca63a2de
IPv6: ::1:51648->::1:631
	TCP timestamps value: 1887522685, echo 1887522685
	Hash d3f87531
IPv6: ::1:51648->::1:631
	TCP timestamps value: 1887522685, echo 1887522685
	Hash d3f87531
IPv6: ::1:631->::1:51648
	TCP timestamps value: 1887522685, echo 1887522685
	Hash ca63a2de
IPv6: ::1:631->::1:51648
	TCP timestamps value: 1887522688, echo 1887522685
	Hash ca63a2de
IPv6: ::1:51650->::1:631
	TCP timestamps value: 1887523538, echo 0
	Hash 512a939c
IPv6: ::1:631->::1:51650
	TCP timestamps value: 1887523538, echo 1887523538
	Hash b857973f
IPv6: ::1:51650->::1:631
	TCP timestamps value: 1887523538, echo 1887523538
	Hash 512a939c
IPv6: ::1:51650->::1:631
	TCP timestamps value: 1887523538, echo 1887523538
	Hash 512a939c
IPv6: ::1:631->::1:51650
	TCP timestamps value: 1887523538, echo 1887523538
	Hash b857973f
IPv6: ::1:631->::1:51650
	TCP timestamps value: 1887523541, echo 1887523538
	Hash b857973f
```
