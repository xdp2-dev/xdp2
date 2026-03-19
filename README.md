<img src="documentation/images/xdp2-big.png" alt="XDP2 logo"/>

# XDP2 (eXpress DataPath 2)

**XDP2** is a programming model, framework, and set of C libraries for
high-performance datapath programming. It provides an API, an optimizing
compiler, test suites, and sample programs for packet and protocol processing.

## Table of Contents

- [Introduction](#introduction)
- [Quick Start (Ubuntu)](#quick-start-ubuntu)
- [Quick Start (Nix)](#quick-start-nix)
- [Project Layout](#project-layout)
- [Documentation](#documentation)
- [Nix Build System](#nix-build-system)
- [Status](#status)
- [Contact](#contact)

## Introduction

XDP2 provides a programmatic way to build high-performance packet parsing and
processing code. You define a **parse graph** — a directed graph of protocol
nodes — and XDP2 generates efficient parser code from it. The C libraries
handle packet traversal, metadata extraction, and flow tracking, while a C++
optimizing compiler can compile the same parse graph to run as an XDP/eBPF
program in the kernel or as a userspace application. For example, the
`flow_tracker_combo` sample uses a single parse graph to extract TCP/UDP flow
tuples over IPv4 and IPv6, running the same parser both in XDP and as a
standalone program.

This repository contains the XDP2 code base: C libraries, include files for
the API, an optimizing compiler (C++ with cppfront), scripts, test code, and
sample programs.

XDP2 is a generalization of [XDP (eXpress Data Path)](https://www.kernel.org/doc/html/latest/networking/af_xdp.html). Where XDP programs the
low-level datapath from device drivers using eBPF, XDP2 extends the model to
programmable hardware and software environments like DPDK. XDP2 retains the
spirit and general feel of XDP, with a more generic API that abstracts
target-specific details. XDP is a first-class compilation target of XDP2.

## Quick Start (Ubuntu)

Target: Ubuntu 22.04+ on x86_64. For the full walkthrough with versioned
packages, debugging, and sample builds, see the
[Getting Started Guide](documentation/getting-started.md).

**1. Install packages**

```
sudo apt-get install -y build-essential gcc gcc-multilib pkg-config bison flex \
    libboost-all-dev libpcap-dev python3-scapy graphviz \
    libelf-dev libbpf-dev llvm-dev clang libclang-dev clang-tools lld \
    linux-tools-$(uname -r)
```

**2. Clone the repository**

```
git clone https://github.com/xdp2/xdp2.git
cd xdp2
```

**3. Build the cppfront dependency**

```
cd thirdparty/cppfront && make
```

**4. Configure and build**

```
cd ../../src
./configure.sh
make && make install
```

**5. Run the parser tests**

```
cd test/parser && ./run-tests.sh
```

## Quick Start (Nix)

Requires [Nix with flakes enabled](https://nixos.org/download/).

```
make build       # Build xdp2            (nix build .#xdp2 -o result)
make test        # Run all x86_64 tests  (nix run .#run-sample-tests)
make dev         # Enter dev shell       (nix develop)
```

For cross-compilation, static analysis, MicroVM testing, and more, see the
[Nix Development Environment Guide](documentation/nix/nix.md).

## Project Layout

```
xdp2/
├── src/                    Source code
│   ├── include/            API headers (xdp2, cli, parselite, flowdis, ...)
│   ├── lib/                Libraries (xdp2, cli, parselite, flowdis, crc, ...)
│   ├── test/               Tests (parser, bitmaps, pvbuf, tables, ...)
│   ├── tools/              Compiler and utilities
│   └── templates/          Code generation templates
├── samples/                Standalone examples
│   ├── parser/             Parser sample programs
│   └── xdp/               XDP2-in-XDP programs
├── documentation/          Project documentation
│   └── nix/                Nix build system docs
├── data/                   Data files and sample pcaps
├── platforms/              Platform definitions
├── thirdparty/             Third-party dependencies
│   ├── cppfront/           Cpp2/cppfront compiler (build dependency)
│   ├── json/               JSON library
│   └── pcap-src/           Packet capture library
├── nix/                    Nix package and module definitions
├── Makefile                Nix build targets (make help)
└── flake.nix               Nix flake definition
```

## Documentation

**Getting Started**
- [Getting Started Guide](documentation/getting-started.md) — full build walkthrough for Ubuntu and Nix

**Core Components**
- [Parser](documentation/parser.md) — XDP2 parser architecture and usage
- [Parser IR](documentation/parser-ir.md) — parser intermediate representation
- [Parse Dump](documentation/parse-dump.md) — parse dump utility
- [Parser Testing](documentation/test-parser.md) — parser test framework
- [Bitmaps](documentation/bitmap.md) — bitmap data structures
- [PVbufs](documentation/pvbufs.md) — packet vector buffers
- [Tables](documentation/tables.md) — advanced table support

**Compiler and Targets**
- [XDP2 Compiler](documentation/xdp2-compiler.md) — optimizing compiler
- [XDP Target](documentation/xdp.md) — XDP compilation target

**Nix Build System**
- [Nix Development Environment](documentation/nix/nix.md) — reproducible builds and dev shell
- [Nix Configure](documentation/nix/nix_configure.md) — Nix-based configure details
- [Dev Shell Isolation](documentation/nix/nix_dev_shell_isolation.md) — isolation model
- [Cross-Architecture Test Summary](documentation/nix/test-summary.md) — test results across architectures

**Development**
- [C++ Style Guide](documentation/cpp-style-guide.md) — coding conventions

## Nix Build System

The Nix build system provides reproducible builds, cross-compilation to RISC-V
and AArch64, integrated static analysis with 8 tools at 3 depth levels, and
MicroVM-based full-system testing.

| Category | Targets |
|---|---|
| **Build** | `make build`, `make build-debug`, `make samples` |
| **Test** | `make test`, `make test-simple`, `make test-offset`, `make test-ports`, `make test-flow` |
| **Cross-compile** | `make riscv64`, `make aarch64` |
| **Cross-test** | `make test-riscv64`, `make test-aarch64` |
| **Static analysis** | `make analysis-quick`, `make analysis-standard`, `make analysis-deep` |
| **MicroVM** | `make vm-x86`, `make vm-aarch64`, `make vm-riscv64` |
| **Packaging** | `make deb` |
| **Development** | `make dev`, `make check`, `make eval` |

Run `make help` for the complete list of targets.

See the [Nix Development Environment Guide](documentation/nix/nix.md) for full
details.

## Status

### Parser and sample tests

| Architecture | Method | Result |
|---|---|---|
| x86_64 | Native | 38/38 PASS |
| RISC-V | Cross-compiled, binfmt | 38/38 PASS |
| AArch64 | Cross-compiled, binfmt | 38/38 PASS |

Note: `xdp_build` tests are skipped due to BPF stack limitations.

### MicroVM full-system testing

| Architecture | Status |
|---|---|
| RISC-V | Built (`make vm-riscv64`) |
| AArch64 | TODO — infrastructure exists, not yet validated |
| x86_64 | TODO — infrastructure exists, not yet validated |

### Static analysis

8 tools available across 3 depth levels (`quick`, `standard`, `deep`).

See the [Cross-Architecture Test Summary](documentation/nix/test-summary.md)
for detailed results.

## Contact

For information, inquiries, or feedback about the XDP2 project, email the
mailing list: **xdp2@lists.linux.dev**
