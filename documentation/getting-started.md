# Getting started with XDP2

## Introduction

This document is a guide to getting started with XDP2

It is intended for developers who are new to XDP2 and want to learn how to use it to program network devices

This document covers both:
- "native" ( no nix )
- with Nix

## Prerequisites

We are assuming access to a Ubuntu 22.04.3 LTS machine

We also assume you have updated the packages ( because you are security concious )
```
sudo apt update
sudo apt --yes upgrade
```

Walkthrough machine has details:
```
das@ubuntu2404-no-nix:~/xdp2/src$ cat /etc/lsb-release
DISTRIB_ID=Ubuntu
DISTRIB_RELEASE=24.04
DISTRIB_CODENAME=noble
DISTRIB_DESCRIPTION="Ubuntu 24.04.3 LTS"
das@ubuntu2404-no-nix:~/xdp2/src$ uname -a
Linux ubuntu2404-no-nix 6.8.0-87-generic #88-Ubuntu SMP PREEMPT_DYNAMIC Sat Oct 11 09:28:41 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux
```

## Native usage

This section covers getting started with xdp2 using native Ubuntu ( without Nix )

### Overview of the steps

1. Package installs
2. git clone
3. ./configure.sh
3.5 make clean
4. make cppfront
5. make
6. make install
7. ports_parser example


### Walkthrough

#### 1. Package installs

**Ubuntu Packages** Ubuntu has x2 forms of packages: un-numbered / numbered

The most simple method is using no version specified.  These seems to default to a version 18 of clang (as of 2025 November).  ./src/configure was designed for this style of package names.
```
sudo apt --yes install build-essential gcc gcc-multilib pkg-config bison flex \
    libboost-all-dev libpcap-dev python3-scapy graphviz libelf-dev libbpf-dev

sudo apt-get --yes install llvm-dev clang libclang-dev clang-tools lld
sudo apt-get --yes install linux-tools-$(uname -r)
```

Ubuntu also has other specific versions available.  We have been testing with version 20, but 17 and 19 is available also (untested currently).

(Unfortunately?) the names of important binaries, like /usr/bin/llvm-config change to /usr/bin/llvm-config-20.

Originally, the ./src/configure script didn't work with the versioned packages.  There is an experimental ./src/configure.sh which adds the extra complexity of supporting the unverioned and versioned forms.

```
sudo apt install --yes build-essential gcc gcc-multilib pkg-config bison flex \
    libboost-all-dev libpcap-dev python3-scapy graphviz libelf-dev libbpf-dev

sudo apt-get --yes install llvm-20-dev clang-20 libclang-20-dev clang-tools-20 lld-20
sudo apt-get --yes install linux-tools-$(uname -r)
```

This walk through is designed to support both styles, so you are welcome to choose which you would prefer.


#### 2. git clone

We will assume starting in your home directory "~".

```
git clone https://github.com/xdp2/xdp2.git
```

#### 3. configure.sh

For this walk through, use the ".sh" version of configure

```
cd ~xdp2/src
./configure.sh
```

**Debugging configure** To allow debugging configure.sh, it supports an arugment --debug-level, which takes integer 0-7 (liek syslog levels).
```
cd ~xdp2/src
./configure.sh --debug-level 7
```

Example outputs

Ubuntu with no versions:
```
das@ubuntu2404-no-nix-no-version:~/xdp2/src$ ./configure.sh


Platform is default
Architecture is x86_64
Architecture includes for x86_64 not found, using generic
Target Architecture is
COMPILER is gcc
LLVM_VER:18.1.3
HOST_LLVM_CONFIG:/usr/bin/llvm-config
XDP2_CLANG_VERSION=18.1.3
XDP2_C_INCLUDE_PATH=/usr/lib/llvm-18/lib/clang/18/include
XDP2_CLANG_RESOURCE_PATH=/usr/lib/llvm-18/lib/clang/18

das@ubuntu2404-no-nix-no-version:~/xdp2/src$

```

Ubuntu with versions, example with 20:
```
das@ubuntu2404-no-nix:~/xdp2/src$ ./configure.sh


Platform is default
Architecture is x86_64
Architecture includes for x86_64 not found, using generic
Target Architecture is
COMPILER is gcc
LLVM_VER:20.1.2
HOST_LLVM_CONFIG:/usr/bin/llvm-config-20
XDP2_CLANG_VERSION=20.1.2
XDP2_C_INCLUDE_PATH=/usr/lib/llvm-20/lib/clang/20/include
XDP2_CLANG_RESOURCE_PATH=/usr/lib/llvm-20/lib/clang/20

das@ubuntu2404-no-nix:~/xdp2/src$
```

configure.sh with debug-level 4
```
das@ubuntu2404-no-nix:~/xdp2/src$ ./configure.sh --debug-level 4


Platform is default
Architecture is x86_64
Architecture includes for x86_64 not found, using generic
Target Architecture is
COMPILER is gcc
[DEBUG-1] Tool Detection: Starting llvm-config detection
[DEBUG-2] Tool Detection: Auto-detecting llvm-config...
[DEBUG-3] Tool Detection: Checking for llvm-config
[DEBUG-3] Tool Detection: Checking for llvm-config-20
[DEBUG-3] Tool Detection: Found llvm-config-20 at /usr/bin/llvm-config-20
[DEBUG-1] Tool Detection: Selected llvm-config-20 (version 20.1.2)
LLVM_VER:20.1.2
[DEBUG-1] Tool Detection: Using HOST_LLVM_CONFIG=/usr/bin/llvm-config-20
HOST_LLVM_CONFIG:/usr/bin/llvm-config-20
[DEBUG-1] Configuration: Platform=default, Architecture=x86_64, Compiler=gcc
[DEBUG-1] Clang.Lib: Starting check
[DEBUG-4] Clang.Lib: HOST_CXX=g++
[DEBUG-4] Clang.Lib: HOST_LLVM_CONFIG=/usr/bin/llvm-config-20
[DEBUG-4] Clang.Lib: llvm-config --ldflags: -L/usr/lib/llvm-20/lib
[DEBUG-4] Clang.Lib: llvm-config --cxxflags: -I/usr/lib/llvm-20/include -std=c++17   -fno-exceptions -funwind-tables -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS
[DEBUG-4] Clang.Lib: llvm-config --libdir: /usr/lib/llvm-20/lib
[DEBUG-4] Clang.Lib: llvm-config --libs: -lLLVM-20
[DEBUG-4] Clang.Lib: Found clang-cpp: libclang-cpp.so.20.1 -> using full path
[DEBUG-4] Clang.Lib: Found clangTooling: libclangTooling.a -> -lclangTooling
[DEBUG-3] Clang.Lib: Discovered clang libraries: /usr/lib/llvm-20/lib/libclang-cpp.so.20.1 -lclangTooling
[DEBUG-3] Clang.Lib: Attempting link with discovered libs: /usr/lib/llvm-20/lib/libclang-cpp.so.20.1 -lclangTooling
[DEBUG-1] Clang.Lib: Check PASSED with libraries: /usr/lib/llvm-20/lib/libclang-cpp.so.20.1 -lclangTooling
XDP2_CLANG_VERSION=20.1.2
XDP2_C_INCLUDE_PATH=/usr/lib/llvm-20/lib/clang/20/include
XDP2_CLANG_RESOURCE_PATH=/usr/lib/llvm-20/lib/clang/20

das@ubuntu2404-no-nix:~/xdp2/src$
```

If you succeed at using xdp2 on something other than ubuntu, please let us know! ( Please note xdp2 with nix on Fedora _is_ tested )

#### 3.3 make clean

It will never hurt to run `make clean` before all of this ;)
```
cd ~/xdp2/src/
make clean
```

#### 4. make cppfront

xdp2 uses an old version of cppfront, which should be built before anything else, as this is a dependancy.

```
cd ~/xdp2/thirdparty/cppfront
make
```

Example from ubuntu no versions
```
das@ubuntu2404-no-nix-no-version:~/xdp2/src$ cd ~/xdp2/thirdparty/cppfront
das@ubuntu2404-no-nix-no-version:~/xdp2/thirdparty/cppfront$ make
g++ -std=c++20 source/cppfront.cpp -o cppfront-compiler
das@ubuntu2404-no-nix-no-version:~/xdp2/thirdparty/cppfront$ ls -la
total 5208
drwxr-xr-x 4 das das    4096 Nov  6 17:56 .
drwxr-xr-x 5 das das    4096 Oct  2 00:22 ..
-rw-r--r-- 1 das das    5756 Oct  2 00:22 CODE_OF_CONDUCT.md
-rw-r--r-- 1 das das    1027 Oct  2 00:22 CONTRIBUTING.md
-rwxrwxr-x 1 das das 5270624 Nov  6 17:56 cppfront-compiler                  <------------
-rw-r--r-- 1 das das     253 Oct  2 00:22 .gitignore
drwxr-xr-x 2 das das    4096 Nov  5 05:14 include
-rw-r--r-- 1 das das     530 Oct  2 00:22 LICENSE
-rw-r--r-- 1 das das     255 Oct  2 00:22 Makefile
-rw-r--r-- 1 das das   19485 Oct  2 00:22 README.md
drwxr-xr-x 2 das das    4096 Oct  2 00:22 source
das@ubuntu2404-no-nix-no-version:~/xdp2/thirdparty/cppfront$ sha256sum cppfront-compiler
d941dd0c74f37377770f9e3a4aefaa43df7403a9b24215d3256a4e62863ba482  cppfront-compiler
das@ubuntu2404-no-nix-no-version:~/xdp2/thirdparty/cppfront$
```

Ubuntu llvm20
```
das@ubuntu2404-no-nix:~/xdp2/thirdparty/cppfront$ sha256sum cppfront-compiler
26d37f784f43a7766e5f892c49a7337e7e0b7858d4fd3b65dec59e8c23846569  cppfront-compiler
```

#### 5. make
Once cppfront is compiled, it's time to build xdp2

```
cd ~/xdp2/src
make clean
make
```

Example output
```
das@ubuntu2404-no-nix-no-version:~/xdp2/src$ make

tools
include/xdp2gen/llvm/patterns.h2... ok (mixed Cpp1/Cpp2, Cpp2 code passes safety checks)

include/xdp2gen/ast-consumer/patterns.h2... ok (mixed Cpp1/Cpp2, Cpp2 code passes safety checks)

...
    LINK     test_bitmap
    CC       main.o
    CC       cli.o
    CC       test_packets_rx.o
    CC       test_packets_tx.o
    CC       test_packets.o
    LINK     test_uet
    CC       main.o
    CC       cli.o
    CC       test_packets_rx.o
    CC       test_packets_tx.o
    CC       test_packets.o
    LINK     test_falcon
das@ubuntu2404-no-nix-no-version:~/xdp2/src$
das@ubuntu2404-no-nix-no-version:~/xdp2/src$ ls -la ./tools/compiler/xdp2-compiler
-rwxrwxr-x 1 das das 38783152 Nov  6 19:46 ./tools/compiler/xdp2-compiler
das@ubuntu2404-no-nix-no-version:~/xdp2/src$ file ./tools/compiler/xdp2-compiler
./tools/compiler/xdp2-compiler: ELF 64-bit LSB pie executable, x86-64, version 1 (GNU/Linux), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=5003d9629f8319c0c7fcdbfb3abae7c215c540d2, for GNU/Linux 3.2.0, with debug_info, not stripped
das@ubuntu2404-no-nix-no-version:~/xdp2/src$ sha256sum ./tools/compiler/xdp2-compiler
5ee68877f374f9493b4e156cdc554b571dcf25d4096710388eba82dd0a70f5e0  ./tools/compiler/xdp2-compiler
```

```
das@ubuntu2404-no-nix:~/xdp2/src$ make

tools
include/xdp2gen/llvm/patterns.h2... ok (mixed Cpp1/Cpp2, Cpp2 code passes safety checks)

include/xdp2gen/ast-consumer/patterns.h2... ok (mixed Cpp1/Cpp2, Cpp2 code passes safety checks)

...
    LINK     test_bitmap
    CC       main.o
    CC       cli.o
    CC       test_packets_rx.o
    CC       test_packets_tx.o
    CC       test_packets.o
    LINK     test_uet
    CC       main.o
    CC       cli.o
    CC       test_packets_rx.o
    CC       test_packets_tx.o
    CC       test_packets.o
    LINK     test_falcon
das@ubuntu2404-no-nix:~/xdp2/src$
das@ubuntu2404-no-nix:~/xdp2/src$ ls -la ./tools/compiler/xdp2-compiler
-rwxrwxr-x 1 das das 39306864 Nov  6 19:46 ./tools/compiler/xdp2-compiler
das@ubuntu2404-no-nix:~/xdp2/src$ file ./tools/compiler/xdp2-compiler
./tools/compiler/xdp2-compiler: ELF 64-bit LSB pie executable, x86-64, version 1 (GNU/Linux), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=c7409d48c698a5a871307d8369fdf15d881d5d7d, for GNU/Linux 3.2.0, with debug_info, not stripped
das@ubuntu2404-no-nix:~/xdp2/src$ sha256sum ./tools/compiler/xdp2-compiler
b21e4b9f3074b25d8c62c2ed8f12aedecf2bf3d96881cc18c0d75459452cc7e6  ./tools/compiler/xdp2-compiler
das@ubuntu2404-no-nix:~/xdp2/src$
```

#### 6. make install

The default INSTALLDIR is `../install/x86_64` from `src/`, which resolves to `~/xdp2/install/x86_64/` (keeps everything in the xdp2 repository directory).

```
cd ~/xdp2/src
make install
```

```
das@ubuntu2404-no-nix-no-version:~$ cd ~/xdp2/src
das@ubuntu2404-no-nix-no-version:~/xdp2/src$ make install

tools
include/xdp2gen/llvm/patterns.h2... ok (mixed Cpp1/Cpp2, Cpp2 code passes safety checks)

include/xdp2gen/ast-consumer/patterns.h2... ok (mixed Cpp1/Cpp2, Cpp2 code passes safety checks)

...

test
    INSTALL  test_vstructs
    INSTALL  test_switch
    INSTALL
    INSTALL  test_timer
    INSTALL  test_pvbuf
    INSTALL  test_parser
    INSTALL
    INSTALL  test_accel
    INSTALL
    INSTALL  test_bitmap
    INSTALL
    INSTALL

xdp2 installed into directory: /home/das/xdp2/src/../install/x86_64
das@ubuntu2404-no-nix-no-version:~/xdp2/src$
```

#### 7. xdp2 samples

With xdp2-compiler built and installed, we can now try using some of the samples, which are in ~/xdp2/samples/

**Important:** The sample Makefile-s needs to know where xdp2 is installed. Set `XDP2DIR` to point to your installation directory (without the architecture suffix).

The samples rely on the environment variable XDP2DIR, which should be set to the same as INSTALLDIR.
```
das@ubuntu2404-no-nix-no-version:~/xdp2/samples$ cat ~/xdp2/src/config.mk | grep INSTALLDIR
INSTALLDIR ?= /home/das/xdp2/src/../install/x86_64
```


Example output:
```
das@ubuntu2404-no-nix-no-version:~/xdp2/samples$ make XDP2DIR=~/xdp2/install/x86_64
make[1]: Entering directory '/home/das/xdp2/samples/parser'
make[2]: Entering directory '/home/das/xdp2/samples/parser/offset_parser'
gcc -I/home/das/xdp2/install/x86_64/include -g   -c -o parser.o parser.c
parser.c: In function ‘extract_network’:
parser.c:62:40: error: ‘const struct xdp2_ctrl_data’ has no member named ‘hdr’
   62 |         metadata->network_offset = ctrl.hdr.hdr_offset;
      |                                        ^
parser.c: In function ‘extract_transport’:
parser.c:71:42: error: ‘const struct xdp2_ctrl_data’ has no member named ‘hdr’
   71 |         metadata->transport_offset = ctrl.hdr.hdr_offset;
      |                                          ^
In file included from parser.c:42:
parser.c: At top level:
parser.c:77:47: warning: initialization of ‘void (*)(const void *, size_t,  size_t,  void *, void *, const struct xdp2_ctrl_data *)’ {aka ‘void (*)(const void *, long unsigned int,  long unsigned int,  void *, void *, const struct xdp2_ctrl_data *)’} from incompatible pointer type ‘void (*)(const void *, void *, const struct xdp2_ctrl_data)’ [-Wincompatible-pointer-types]
   77 |                      (.ops.extract_metadata = extract_network));
      |                                               ^~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:209:49: note: in definition of macro ‘__XDP2_MAKE_PARSE_NODE_OPT_ONE’
  209 | #define __XDP2_MAKE_PARSE_NODE_OPT_ONE(OPT) .pn OPT,
      |                                                 ^~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:54:25: note: in expansion of macro ‘__XDP2_PMACRO_APPLY1’
   54 |                         __XDP2_PMACRO_APPLY##NUM(ACT, __VA_ARGS__)
      |                         ^~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:57:25: note: in expansion of macro ‘__XDP2_PMACRO_APPLY_ALL_3’
   57 |                         __XDP2_PMACRO_APPLY_ALL_3(ACT, NUM, __VA_ARGS__)
      |                         ^~~~~~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:60:9: note: in expansion of macro ‘__XDP2_PMACRO_APPLY_ALL_2’
   60 |         __XDP2_PMACRO_APPLY_ALL_2(ACT,                                  \
      |         ^~~~~~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:218:17: note: in expansion of macro ‘XDP2_PMACRO_APPLY_ALL’
  218 |                 XDP2_PMACRO_APPLY_ALL(                                  \
      |                 ^~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/utility.h:80:24: note: in expansion of macro ‘XDP2_DEPAIR2’
   80 | #define XDP2_DEPAIR(X) XDP2_DEPAIR2 X
      |                        ^~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:220:33: note: in expansion of macro ‘XDP2_DEPAIR’
  220 |                                 XDP2_DEPAIR(EXTRA))
      |                                 ^~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:228:17: note: in expansion of macro ‘__XDP2_MAKE_PARSE_NODE_COMMON’
  228 |                 __XDP2_MAKE_PARSE_NODE_COMMON(PARSE_NODE, PROTO_DEF,    \
      |                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
parser.c:76:1: note: in expansion of macro ‘XDP2_MAKE_PARSE_NODE’
   76 | XDP2_MAKE_PARSE_NODE(ipv4_node, xdp2_parse_ipv4, ip_table,
      | ^~~~~~~~~~~~~~~~~~~~
parser.c:77:47: note: (near initialization for ‘ipv4_node.pn.ops.extract_metadata’)
   77 |                      (.ops.extract_metadata = extract_network));
      |                                               ^~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:209:49: note: in definition of macro ‘__XDP2_MAKE_PARSE_NODE_OPT_ONE’
  209 | #define __XDP2_MAKE_PARSE_NODE_OPT_ONE(OPT) .pn OPT,
      |                                                 ^~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:54:25: note: in expansion of macro ‘__XDP2_PMACRO_APPLY1’
   54 |                         __XDP2_PMACRO_APPLY##NUM(ACT, __VA_ARGS__)
      |                         ^~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:57:25: note: in expansion of macro ‘__XDP2_PMACRO_APPLY_ALL_3’
   57 |                         __XDP2_PMACRO_APPLY_ALL_3(ACT, NUM, __VA_ARGS__)
      |                         ^~~~~~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:60:9: note: in expansion of macro ‘__XDP2_PMACRO_APPLY_ALL_2’
   60 |         __XDP2_PMACRO_APPLY_ALL_2(ACT,                                  \
      |         ^~~~~~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:218:17: note: in expansion of macro ‘XDP2_PMACRO_APPLY_ALL’
  218 |                 XDP2_PMACRO_APPLY_ALL(                                  \
      |                 ^~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/utility.h:80:24: note: in expansion of macro ‘XDP2_DEPAIR2’
   80 | #define XDP2_DEPAIR(X) XDP2_DEPAIR2 X
      |                        ^~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:220:33: note: in expansion of macro ‘XDP2_DEPAIR’
  220 |                                 XDP2_DEPAIR(EXTRA))
      |                                 ^~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:228:17: note: in expansion of macro ‘__XDP2_MAKE_PARSE_NODE_COMMON’
  228 |                 __XDP2_MAKE_PARSE_NODE_COMMON(PARSE_NODE, PROTO_DEF,    \
      |                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
parser.c:76:1: note: in expansion of macro ‘XDP2_MAKE_PARSE_NODE’
   76 | XDP2_MAKE_PARSE_NODE(ipv4_node, xdp2_parse_ipv4, ip_table,
      | ^~~~~~~~~~~~~~~~~~~~
parser.c:79:47: warning: initialization of ‘void (*)(const void *, size_t,  size_t,  void *, void *, const struct xdp2_ctrl_data *)’ {aka ‘void (*)(const void *, long unsigned int,  long unsigned int,  void *, void *, const struct xdp2_ctrl_data *)’} from incompatible pointer type ‘void (*)(const void *, void *, const struct xdp2_ctrl_data)’ [-Wincompatible-pointer-types]
   79 |                      (.ops.extract_metadata = extract_network));
      |                                               ^~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:209:49: note: in definition of macro ‘__XDP2_MAKE_PARSE_NODE_OPT_ONE’
  209 | #define __XDP2_MAKE_PARSE_NODE_OPT_ONE(OPT) .pn OPT,
      |                                                 ^~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:54:25: note: in expansion of macro ‘__XDP2_PMACRO_APPLY1’
   54 |                         __XDP2_PMACRO_APPLY##NUM(ACT, __VA_ARGS__)
      |                         ^~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:57:25: note: in expansion of macro ‘__XDP2_PMACRO_APPLY_ALL_3’
   57 |                         __XDP2_PMACRO_APPLY_ALL_3(ACT, NUM, __VA_ARGS__)
      |                         ^~~~~~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:60:9: note: in expansion of macro ‘__XDP2_PMACRO_APPLY_ALL_2’
   60 |         __XDP2_PMACRO_APPLY_ALL_2(ACT,                                  \
      |         ^~~~~~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:218:17: note: in expansion of macro ‘XDP2_PMACRO_APPLY_ALL’
  218 |                 XDP2_PMACRO_APPLY_ALL(                                  \
      |                 ^~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/utility.h:80:24: note: in expansion of macro ‘XDP2_DEPAIR2’
   80 | #define XDP2_DEPAIR(X) XDP2_DEPAIR2 X
      |                        ^~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:220:33: note: in expansion of macro ‘XDP2_DEPAIR’
  220 |                                 XDP2_DEPAIR(EXTRA))
      |                                 ^~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:228:17: note: in expansion of macro ‘__XDP2_MAKE_PARSE_NODE_COMMON’
  228 |                 __XDP2_MAKE_PARSE_NODE_COMMON(PARSE_NODE, PROTO_DEF,    \
      |                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
parser.c:78:1: note: in expansion of macro ‘XDP2_MAKE_PARSE_NODE’
   78 | XDP2_MAKE_PARSE_NODE(ipv6_node, xdp2_parse_ipv6, ip_table,
      | ^~~~~~~~~~~~~~~~~~~~
parser.c:79:47: note: (near initialization for ‘ipv6_node.pn.ops.extract_metadata’)
   79 |                      (.ops.extract_metadata = extract_network));
      |                                               ^~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:209:49: note: in definition of macro ‘__XDP2_MAKE_PARSE_NODE_OPT_ONE’
  209 | #define __XDP2_MAKE_PARSE_NODE_OPT_ONE(OPT) .pn OPT,
      |                                                 ^~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:54:25: note: in expansion of macro ‘__XDP2_PMACRO_APPLY1’
   54 |                         __XDP2_PMACRO_APPLY##NUM(ACT, __VA_ARGS__)
      |                         ^~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:57:25: note: in expansion of macro ‘__XDP2_PMACRO_APPLY_ALL_3’
   57 |                         __XDP2_PMACRO_APPLY_ALL_3(ACT, NUM, __VA_ARGS__)
      |                         ^~~~~~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:60:9: note: in expansion of macro ‘__XDP2_PMACRO_APPLY_ALL_2’
   60 |         __XDP2_PMACRO_APPLY_ALL_2(ACT,                                  \
      |         ^~~~~~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:218:17: note: in expansion of macro ‘XDP2_PMACRO_APPLY_ALL’
  218 |                 XDP2_PMACRO_APPLY_ALL(                                  \
      |                 ^~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/utility.h:80:24: note: in expansion of macro ‘XDP2_DEPAIR2’
   80 | #define XDP2_DEPAIR(X) XDP2_DEPAIR2 X
      |                        ^~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:220:33: note: in expansion of macro ‘XDP2_DEPAIR’
  220 |                                 XDP2_DEPAIR(EXTRA))
      |                                 ^~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:228:17: note: in expansion of macro ‘__XDP2_MAKE_PARSE_NODE_COMMON’
  228 |                 __XDP2_MAKE_PARSE_NODE_COMMON(PARSE_NODE, PROTO_DEF,    \
      |                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
parser.c:78:1: note: in expansion of macro ‘XDP2_MAKE_PARSE_NODE’
   78 | XDP2_MAKE_PARSE_NODE(ipv6_node, xdp2_parse_ipv6, ip_table,
      | ^~~~~~~~~~~~~~~~~~~~
parser.c:81:47: warning: initialization of ‘void (*)(const void *, size_t,  size_t,  void *, void *, const struct xdp2_ctrl_data *)’ {aka ‘void (*)(const void *, long unsigned int,  long unsigned int,  void *, void *, const struct xdp2_ctrl_data *)’} from incompatible pointer type ‘void (*)(const void *, void *, const struct xdp2_ctrl_data)’ [-Wincompatible-pointer-types]
   81 |                      (.ops.extract_metadata = extract_transport));
      |                                               ^~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:209:49: note: in definition of macro ‘__XDP2_MAKE_PARSE_NODE_OPT_ONE’
  209 | #define __XDP2_MAKE_PARSE_NODE_OPT_ONE(OPT) .pn OPT,
      |                                                 ^~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:54:25: note: in expansion of macro ‘__XDP2_PMACRO_APPLY1’
   54 |                         __XDP2_PMACRO_APPLY##NUM(ACT, __VA_ARGS__)
      |                         ^~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:57:25: note: in expansion of macro ‘__XDP2_PMACRO_APPLY_ALL_3’
   57 |                         __XDP2_PMACRO_APPLY_ALL_3(ACT, NUM, __VA_ARGS__)
      |                         ^~~~~~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:60:9: note: in expansion of macro ‘__XDP2_PMACRO_APPLY_ALL_2’
   60 |         __XDP2_PMACRO_APPLY_ALL_2(ACT,                                  \
      |         ^~~~~~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:218:17: note: in expansion of macro ‘XDP2_PMACRO_APPLY_ALL’
  218 |                 XDP2_PMACRO_APPLY_ALL(                                  \
      |                 ^~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/utility.h:80:24: note: in expansion of macro ‘XDP2_DEPAIR2’
   80 | #define XDP2_DEPAIR(X) XDP2_DEPAIR2 X
      |                        ^~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:220:33: note: in expansion of macro ‘XDP2_DEPAIR’
  220 |                                 XDP2_DEPAIR(EXTRA))
      |                                 ^~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:248:17: note: in expansion of macro ‘__XDP2_MAKE_PARSE_NODE_COMMON’
  248 |                 __XDP2_MAKE_PARSE_NODE_COMMON(PARSE_NODE, PROTO_DEF,    \
      |                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
parser.c:80:1: note: in expansion of macro ‘XDP2_MAKE_LEAF_PARSE_NODE’
   80 | XDP2_MAKE_LEAF_PARSE_NODE(ports_node, xdp2_parse_ports,
      | ^~~~~~~~~~~~~~~~~~~~~~~~~
parser.c:81:47: note: (near initialization for ‘ports_node.pn.ops.extract_metadata’)
   81 |                      (.ops.extract_metadata = extract_transport));
      |                                               ^~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:209:49: note: in definition of macro ‘__XDP2_MAKE_PARSE_NODE_OPT_ONE’
  209 | #define __XDP2_MAKE_PARSE_NODE_OPT_ONE(OPT) .pn OPT,
      |                                                 ^~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:54:25: note: in expansion of macro ‘__XDP2_PMACRO_APPLY1’
   54 |                         __XDP2_PMACRO_APPLY##NUM(ACT, __VA_ARGS__)
      |                         ^~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:57:25: note: in expansion of macro ‘__XDP2_PMACRO_APPLY_ALL_3’
   57 |                         __XDP2_PMACRO_APPLY_ALL_3(ACT, NUM, __VA_ARGS__)
      |                         ^~~~~~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/pmacro.h:60:9: note: in expansion of macro ‘__XDP2_PMACRO_APPLY_ALL_2’
   60 |         __XDP2_PMACRO_APPLY_ALL_2(ACT,                                  \
      |         ^~~~~~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:218:17: note: in expansion of macro ‘XDP2_PMACRO_APPLY_ALL’
  218 |                 XDP2_PMACRO_APPLY_ALL(                                  \
      |                 ^~~~~~~~~~~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/utility.h:80:24: note: in expansion of macro ‘XDP2_DEPAIR2’
   80 | #define XDP2_DEPAIR(X) XDP2_DEPAIR2 X
      |                        ^~~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:220:33: note: in expansion of macro ‘XDP2_DEPAIR’
  220 |                                 XDP2_DEPAIR(EXTRA))
      |                                 ^~~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:248:17: note: in expansion of macro ‘__XDP2_MAKE_PARSE_NODE_COMMON’
  248 |                 __XDP2_MAKE_PARSE_NODE_COMMON(PARSE_NODE, PROTO_DEF,    \
      |                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
parser.c:80:1: note: in expansion of macro ‘XDP2_MAKE_LEAF_PARSE_NODE’
   80 | XDP2_MAKE_LEAF_PARSE_NODE(ports_node, xdp2_parse_ports,
      | ^~~~~~~~~~~~~~~~~~~~~~~~~
parser.c: In function ‘run_parser’:
parser.c:111:33: error: storage size of ‘pdata’ isn’t known
  111 |         struct xdp2_packet_data pdata;
      |                                 ^~~~~
parser.c:122:17: warning: implicit declaration of function ‘XDP2_SET_BASIC_PDATA_LEN_SEQNO’ [-Wimplicit-function-declaration]
  122 |                 XDP2_SET_BASIC_PDATA_LEN_SEQNO(pdata, packet, plen,
      |                 ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
parser.c:125:44: warning: passing argument 3 of ‘xdp2_parse’ makes integer from pointer without a cast [-Wint-conversion]
  125 |                 xdp2_parse(parser, &pdata, &metadata, 0);
      |                                            ^~~~~~~~~
      |                                            |
      |                                            struct metadata *
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:297:48: note: expected ‘size_t’ {aka ‘long unsigned int’} but argument is of type ‘struct metadata *’
  297 |                              void *hdr, size_t len,
      |                                         ~~~~~~~^~~
parser.c:125:17: error: too few arguments to function ‘xdp2_parse’
  125 |                 xdp2_parse(parser, &pdata, &metadata, 0);
      |                 ^~~~~~~~~~
/home/das/xdp2/install/x86_64/include/xdp2/parser.h:296:19: note: declared here
  296 | static inline int xdp2_parse(const struct xdp2_parser *parser,
      |                   ^~~~~~~~~~
make[2]: *** [<builtin>: parser.o] Error 1
make[2]: Leaving directory '/home/das/xdp2/samples/parser/offset_parser'
make[1]: *** [Makefile:14: offset_parser] Error 2
make[1]: Leaving directory '/home/das/xdp2/samples/parser'
make: *** [Makefile:14: parser] Error 2
das@ubuntu2404-no-nix-no-version:~/xdp2/samples$
```


## Nix usage

This section covers how to get started with xdp2 using the Nix development environment.

<Note to auther: Complete "native method" before starting on Nix usage>