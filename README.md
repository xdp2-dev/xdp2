XDP2 (eXpress DataPath 2)
=========================


**XDP2** is a software programming model, framework, set of libraries, and an
API used to program the high performance datapath. In networking, XDP2 is
applied to optimize packet and protocol processing.

# Contact information

For more information, inquiries, or comments about the XDP2 project please
direct email to tom@xdpnet.ai.

# Description

This repository contains the code base for the XDP2 project. The XDP2 code
is composed of a number of C libraries, include files for the API, test code,
and sample code.

There are four libraries:

* **xdp2**: the main library that implements the XDP2 programming model
	 and the XDP2 Parser
* **siphash**: a port of the siphash functions to userspace
* **flowdis**: contains a port of kernel flow dissector to userspace
* **parselite**: a simple handwritten parser for evaluation

# Directory structure

The top level directories are:

* **src**: contains source code for libraries, the XDP2 API, and test code
* **samples**: contains standalone example applications that use the XDP2 API
* **documentation**: contains documentation for XDP2

The subdirectories of **src** are:

* **lib**: contains the code for the XDP2 libraries. The lib directory has
subdirectories:
	* **xdp2**: The main XDP2 library
	* **flowdis**: Flow dissector library
	* **parselite**: A very lightweight parser
	* **siphash**: Port of siphash library to userspace

* **include**: contains the include files of the XDP2 API. The include
directory has subdirectories
	* **xdp2**: General utility functions, header files, and API for the
	  XDP2 library
	* **flowdis**: Header files for the flowdis library
	* **parselite**: Header files for the parselite library
	* **siphash**: Header files for the siphash library
	* **crc**: CRC functions
	* **lzf**: LZF compression
	* **murmur3hash**: Murmur3 hash
	* **cli**: Gerneric CLI
	* **kernel**: Kernel headers for XDP2

	For usage of the **flowdis**, **parselite**, and **siphash** libraries,
	see the include files in the corresponding directory of the library.
	For **XDP2**, see the include files in the xdp2 include directory as
	well as the documentation.

* **test**: contains related tests for XDP2. Subdirectory is:
	* **parser**: Contains code and scripts for testing the XDP2
	  parser, flowdis parser, and parselite parsers
	* **pvbuf**: Test of PVbufs
	* **bitmaps**: Test of bitmaps
	* **parse_dump**: Parse dump test
	* **parser**: Parser test
	* **router**: Super simple router test
	* **switch**: Test switch statement
	* **tables**: Test advanced tables
	* **timer**: Test of timers
	* **vstructs**: Super simple router test
	* **accelerator**: Accelerator test

The subdirectories of **samples** are:

* **parser**: Standalone example programs for the XDP2 parser.
* **xdp**: Example XDP2 in XDP programs
* **dpdk**: DPDK example
* **kmod**: Kernel module example
* **xdp**: Example

# Building

## Prerequisites

To install the basic development prerequisites on Ubuntu 20.10 or up we need to install the following packages:

**# apt-get install -y build-essential gcc-multilib pkg-config bison flex libboost-all-dev libpcap-dev**

If you want to generate graph visualizations from the xdp2 compiler you must install the graphviz package:

**# apt-get install -y graphviz**

And if you intent to use the eBPF compilation or the **flow_tracker** sample, you must install the dependencies to compile and load BPF programs as described below:

**# apt-get install -y libelf-dev clang llvm libbpf-dev linux-tools-$(uname -r)**

Because the linux tools is dependent on the kernel version, you should use the **uname -r** command to get the current kernel version as shown above.

For XDP, we recommend a minimum Linux kernel version of 5.8, which is available on Ubuntu 20.10 and up.

## Configure

Building of the main libraries and code is performed by doing make in the
**src** directory:

**cd src**

**./configure**

## Make

In the src directory do:

**make**

## Install

The compiled libraries, header files, and binaries may be installed in a
specified directory:

**make INSTALLDIR=$(MYINSTALLDIR) install**

To get verbose output from make add **V=1** to the command line. To include the
uapi files use **UAPI=1** (see note below). For example,

**make INSTALLDIR=$(MYINSTALLDIR) V=1 install**

builds the with verbose output from make, includes the uapi files, and
install the target files in the given install directory (set in
MYINSTALLDIR)

To install the xdp2 tc classifier:

**make -C kernel modules_install**

If you are using secure boot, you will need to sign the kernel module.
Read more about this here: https://ubuntu.com/blog/how-to-sign-things-for-secure-boot

# Basic validation testing

To perform basic validation of the parser do

**cd src/test/parser**

**run-tests.sh**

The output should show the the parsers being run with no reported diffs or
other errors.

For more information please see [testing](documentation/test-parser.md).

# Sample applications

# simple_parser

**samples/parser/simple_parser** contains two examples of code for a very
simple parser that extracts IP addresses and port numbers from UDP and TCP
packets and prints the information as well as a tuple hash.
See [simple_parser](samples/parser/simple_parser/README.md) for details.

# xdp

**samples/xdp** contains sample XDP2 in xdp programs. See
[xdp](documentation/xdp.md) for background information on XDP2 in XDP.

**samples/xdp/flow_tracker_simple** contains a sample XDP2 Parser in XDP
application. See [flow_tracker_simple](samples/xdp/flow_tracker_simple/README.md)
for details.

**samples/xdp/flow_tracker_tmpl** contains a sample XDP2 Parser in XDP
application that uses the XDP program template in include/xdp2/xdp_tmpl.h.
See [flow_tracker_tmpl](samples/xdp/flow_tracker_tmpl/README.md) for details.

**samples/xdp/flow_tracker_combo** contains and example XDP2 Parser in XDP and also demonstrates use of the same parser code in a userspace application. See
[flow_tracker_combo](samples/xdp/flow_tracker_combo/README.md) for details.

# kmod

**samples/kmod/cls** contains a sample kernel module that runs the XDP2 Parser using the pxdp2classifier. For more information see
[kmod](documentation/kmod.md).

# dpdk

**samples/dpdk/dpdk_snoop_app** contains a sample app which uses DPDK stack to
snoop packets from available DPDK driver ready interfaces (DPDK RX ports) and
uses the XDP2 Parser to extract information from the packets.
For more information see [dpdk_snoop_app](samples/dpdk/dpdk-snoop-app/README.md).
