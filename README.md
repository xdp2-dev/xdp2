<img src="documentation/images/xdp2-big.png" alt="XDP2 big logo"/>

XDP2 (eXpress DataPath 2)
=========================

**XDP2** is a software programming model, framework, set of libraries, and an
API used to program the high performance datapath. In networking, XDP2 is
applied to optimize packet and protocol processing.

Jump [here](#Building-src) if you'd like to skip to building the code.

Contact information
===================

For more information, inquiries, or comments about the XDP2 project please
send to the XDP2 mailing list xdp2@lists.linux.dev.

Description
===========

This repository contains the code base for the XDP2 project. The XDP2 code
is composed of a number of C libraries, include files for the API, scripts,
test code, and sample code.

Relationship to XDP
===================

XDP2 can be thought of as a generalization of XDP. Where XDP is a facility
for programming the low level datapath from device drivers, XDP2 extends
the model to program programmable hardware as well as software environments
that are not eBPF like DPDK. XDP2 retains the spirit of XDP in an easy-to-use
programming model, as well as the general look and feel of XDP. The XDP2
API is a bit more generic than XDP and abstracts out target specific items.
XDP is a first class target of XDP (see XDP2 samples). Converting an XDP
program to XDP2 should mostly be a matter of adapting the code to the XDP2
API and splitting out required XDP code like glue code.

Directory structure
===================

The top level directories are:

* **src**: contains source code for libraries, the XDP2 API, and test code
* **samples**: contains standalone example applications that use the XDP2 API
* **documentation**: contains documentation for XDP2
* **platforms**: contains platform definitions. Currently a default platform
is supported
* **thirdparty**: contains third party code include pcap library
* **data**: contains data files including sample pcap files

Source directories
------------------

The *src* directory contains the in-tree source code for XDP-2.
The subdirectories of **src** are:

* **lib**: contains the code for the XDP2 libraries. The lib directory has
subdirectories:
	* **xdp2**: the main library that implements the XDP2 programming model
		 and the XDP2 Parser
	* **siphash**: a port of the siphash functions to userspace
	* **flowdis**: contains a port of kernel flow dissector to userspace
	* **parselite**: a simple handwritten parser for evaluation
	* **cli**: CLI library used to provide an XDP2 CLI
	* **crc**: CRC function library
	* **lzf**: LZF compression library
	* **murmur3hash**: Murmur3 Hash library

* **include**: contains the include files of the XDP2 API. The include
directory has subdirectories
	* **xdp2**: General utility functions, header files, and API for the
	  XDP2 library
	* **siphash**: Header files for the siphash library
	* **flowdis**: Header files for the flowdis library
	* **parselite**: Header files for the parselite library
	* **cli**: Generic CLI
	* **crc**: CRC functions
	* **lzf**: LZF compression
	* **murmur3hash**: Murmur3 hash

	For **XDP2**, see the include files in the xdp2 include directory as
	well as the documentation. For the others see the include files in the
	corresponding directory of the library.

* **test**: contains related tests for XDP2. Subdirectory is:
	* **pvbuf**: Test of PVbufs
	* **bitmaps**: Test of bitmaps
	* **parse_dump**: Parse dump test
	* **parser**: Parser test
	* **router**: Super simple router test
	* **switch**: Test switch statement
	* **tables**: Test advanced tables
	* **timer**: Test of timers
	* **vstructs**: Variable structures test
	* **accelerator**: Accelerator test

Samples
-------

The *samples* directory contains out-of-tree sample code.
The subdirectories of **samples** are:

* **parser**: Standalone example programs for the XDP2 parser.
* **xdp**: Example XDP2 in XDP programs

For more information about the XDP2 code samples see the README files in
the samples directory

Building-src
============

The XDP source is built by doing a make in the top level source directory.

Prerequisites
-------------

To install the basic development prerequisites on Ubuntu 20.10 or up we need to install the following packages:

```
# apt-get install -y build-essential gcc-multilib pkg-config bison flex libboost-all-dev libpcap-dev
```

If you want to generate graph visualizations from the xdp2 compiler you must
install the graphviz package:

```
# apt-get install -y graphviz
```

If you intend use the optimized compiler or build XDP samples then you must
install the dependencies to compile and load BPF programs as described below:

```
# apt-get install -y libelf-dev clang llvm libbpf-dev linux-tools-$(uname -r)
```

Because the linux tools is dependent on the kernel version, you should use the
**uname -r** command to get the current kernel version as shown above.

For XDP, we recommend a minimum Linux kernel version of 5.8, which is available on Ubuntu 20.10 and up.

Configure
---------

The *configure* script in the source directory is ran to configure the
build process. The usage is:

```
$ ./configure --help

$ ./configure --help

Usage: ./configure [--config-defines <defines>] [--ccarch <arch>]
 [--arch <arch>] [--compiler <compiler>]
 [--installdir <dir>] [--build-opt-parser]
 [--pkg-config-path <path>] [--python-ver <version>]
 [--llvm-config <llvm-config>]
```

Parameters:

* **--config-defines <defines>** set compiler defines with format
  "**-D**\<*name*\>=\<*val*\> ..."
* **--ccarch <arch>** set cross compiler architecture (cross compilation will
be supported in the future)
* **--arch <arch>** set architecture. Currently *x84_64* is supported
* **--compiler <compiler>** set the compiler. Default is *gcc*, *clang* is
an alternative
* **--installdir** install directory
* **--build-opt-parser** build the optimized parser (see **xdp-compiler**
below)
* **--pkg-config-path <path>** set Python package config path
* **--python-ver <version>** sets the Python version
* **--llvm-config <llvm-config>** set the LLVM config command. The default
is */usr/bin/llvm-config*. This is only used if **--build-opt-parser** is set
 
Examples:

Run configure with no arguments
```

$ ./configure

Setting default compiler as gcc for native builds

Platform is default
Architecture is x86_64
Architecture includes for x86_64 not found, using generic
Target Architecture is 
COMPILER is gcc
XDP2_CLANG_VERSION=14.0.0
XDP2_C_INCLUDE_PATH=/usr/lib/llvm-14/lib/clang/14/include
XDP2_CLANG_RESOURCE_PATH=/usr/lib/llvm-14/lib/clang/14
```

Run configure with build optimized parser, an install directory,
an LLVM config program, and use clang as the compiler:
```
./configure --installdir ~/xdp2/install --build-opt-parser --llvm-config /usr/bin/llvm-config-20 --compiler clang


Platform is default
Architecture is x86_64
Architecture includes for x86_64 not found, using generic
Target Architecture is 
COMPILER is clang
^[[OXDP2_CLANG_VERSION=20.1.8
XDP2_C_INCLUDE_PATH=/usr/lib/llvm-20/lib/clang/20/include
XDP2_CLANG_RESOURCE_PATH=/usr/lib/llvm-20/lib/clang/20
```

## Make

Building of the main libraries and code is performed by doing make in the
**src** directory:

```
make
```

The compiled libraries, header files, and binaries may be installed in
the installation directory specified by *configure**:
```
make install
```

To get verbose output from make add **V=1** to the command line. Additional
CFLAGS may be set by using CCOPTS in the command line:

```
$ make CCOPTS=-O3

$ make CCOPTS=-g
```

Building samples
================

Before building samples the main source must be built and installed. Note that
*samples/xdp* requires that the optimized parser was built. Sample are built
by:
```
make XDP2DIR=<install-dir>
```

where *\<install-dir\>* is the installation directory for XDP2.

For more information consult the README files in the *samples* directory.

Test
====

The XDP2 tests are built as part of XDP build. They are installed in
*\<install-dir\>/bin/test_\** where \* is replaced by the test name. For
instance:

```
$ ls ~/xdp2/install/bin
ls ~/xdp2/install/bin
parse_dump  test_accel   test_parser  test_router  test_tables  test_vstructs
pmacro_gen  test_bitmap  test_pvbuf   test_switch  test_timer
```

Most of the tests are documented in the *documentation* directory (under the
service descriptions)

# Basic validation testing

To perform basic validation of the parser do

**cd src/test/parser**

**run-tests.sh**

The output should show the the parsers being run with no reported diffs or
other errors.

For more information on the parser test please see
[testing](documentation/test-parser.md).
