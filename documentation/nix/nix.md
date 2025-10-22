# XDP2 — Nix Flake Development Environment

This document explains how to use **Nix** to set up a reproducible development environment for the **XDP2** project.

## Goals

The goals of using Nix in this repository are to:
- **Simplify onboarding** — make it easy to get started with XDP2 on any Linux system
- **Improve reproducibility** — ensure consistent build environments across developers (no more "it worked on my machine")
- **Reduce setup friction** — eliminate dependency conflicts and version mismatches

> ⚠️ **Linux only:** This flake currently supports **Linux only** because [`libbpf`](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/pkgs/os-specific/linux/libbpf/default.nix) is Linux-specific.

Feedback and merge requests are welcome. If we're missing a tool, please open an issue or PR. See the `corePackages` section in `flake.nix`.

---

## Table of Contents

- [XDP2 — Nix Flake Development Environment](#xdp2--nix-flake-development-environment)
  - [Goals](#goals)
  - [Table of Contents](#table-of-contents)
  - [Background](#background)
  - [Quick Start](#quick-start)
    - [1. Install Nix](#1-install-nix)
    - [2. Enter Development Environment](#2-enter-development-environment)
    - [3. First Run Considerations](#3-first-run-considerations)
    - [4. Smart Configure](#4-smart-configure)
    - [5. Build and Test](#5-build-and-test)
  - [Debugging](#debugging)
    - [Debugging nix develop](#debugging-nix-develop)
    - [Shellcheck](#shellcheck)
  - [Understanding the Environment](#understanding-the-environment)
    - [Nix Derivation vs Development Shell](#nix-derivation-vs-development-shell)
    - [How the Nix Development Environment Works](#how-the-nix-development-environment-works)
    - [Makefiles and Environment Variables](#makefiles-and-environment-variables)
    - [Environment Variables Reference](#environment-variables-reference)
  - [Dependencies and Packages](#dependencies-and-packages)
    - [Boost Libraries](#boost-libraries)
      - [Boost Libraries Actually Used](#boost-libraries-actually-used)
      - [Usage Context](#usage-context)
  - [Compiler Architecture](#compiler-architecture)
    - [Analysis of Multiple Compilers](#analysis-of-multiple-compilers-in-the-xdp2-project)
      - [Compiler Separation Architecture](#compiler-separation-architecture)
      - [Specific Compiler Usage Patterns](#specific-compiler-usage-patterns)
      - [Compiler Usage by File/Directory](#compiler-usage-by-filedirectory)
      - [Why They Don't Conflict](#why-they-dont-conflict)
      - [Example: Parser Optimization Workflow](#example-parser-optimization-workflow)
      - [Why This Architecture?](#why-this-architecture)
      - [In Practice](#in-practice)
  - [Nix Development Environment Implications](#nix-development-environment-implications)
    - [Required Environment Variables](#required-environment-variables)
    - [Key Considerations](#key-considerations)
  - [Configure Script Challenges](#configure-script-challenges)
    - [Required Environment Variables](#required-environment-variables-1)
    - [Nix Flake Configuration](#nix-flake-configuration)

---

## Background

[Nix](https://nixos.org) is a package manager for Linux and other Unix-like systems that provides **reproducible, isolated** environments. By tracking all dependencies and hashing their content, it ensures every developer uses the same versions of every package.

### What This Repository Provides

This repository includes `flake.nix` and `flake.lock`:
- **`flake.nix`** defines the development environment (compilers, libraries, tools, helper functions)
- **`flake.lock`** pins exact versions so all developers use **identical** inputs

Running `nix develop` spawns a shell with the correct toolchains, libraries, and environment variables configured for you.

---

## Quick Start

Install Nix if you don't already have it, then enter the development environment.

### 1. Install Nix

Choose **multi-user** (daemon) or **single-user**:

- **Multi-user install** (recommended on most distros)
  [Install Nix (multi-user)](https://nix.dev/manual/nix/2.24/installation/#multi-user)
  ```bash
  bash <(curl -L https://nixos.org/nix/install) --daemon
  ```

- **Single-user install** (recommended on most distros)
  [Install Nix (single-user)](https://nix.dev/manual/nix/2.24/installation/#single-user)
  ```bash
  bash <(curl -L https://nixos.org/nix/install) --no-daemon
  ```

#### Video Tutorials

| Platform | Video |
|----------|-------|
| Ubuntu | [Installing Nix on Ubuntu](https://youtu.be/cb7BBZLhuUY) |
| Fedora | [Installing Nix on Fedora](https://youtu.be/RvaTxMa4IiY) |

### 2. Enter Development Environment

Run the `nix develop` command to enter the development environment shell.

This command will:
- Generate a new shell environment with all required environment variables set
- Make available all tools required for XDP2 development
- Provide helper functions for building and testing

#### Enable Flakes (if needed)

If you don't have the "flakes" feature enabled, run this command:
```bash
nix --extra-experimental-features 'nix-command flakes' develop .
```

To permanently enable the Nix "flakes" feature, update `/etc/nix/nix.conf`:
```bash
test -d /etc/nix || sudo mkdir /etc/nix
echo 'experimental-features = nix-command flakes' | sudo tee -a /etc/nix/nix.conf
nix develop
```

With flakes enabled, simply run:
```bash
nix develop
```

See also: [Nix Flakes Wiki](https://nixos.wiki/wiki/flakes)

### 3. First Run Considerations

On first execution, Nix will download and build all dependencies, which might take several minutes. On subsequent executions, Nix will reuse the cache in `/nix/store/` and will be essentially instantaneous.

> **Note:** Nix will not interact with any "system" packages you may already have installed. The Nix versions are isolated and will effectively "disappear" when you exit the development shell.

### 4. Smart Configure

When starting the `nix develop` shell, the Nix code will run a `smart-configure` function that:
- Checks if `./src/config.mk` exists
- Creates it by running `configure` if it doesn't exist
- Warns you if `./src/config.mk` is older than 14 days (configurable via `configAgeWarningDays`)

### 5. Build and Test

Once inside the Nix development shell, you have access to helper functions:

```bash
xdp2-help

build-all

clean-all
```

The `build-all` function applies various changes to ensure the build works inside the Nix shell (for example, it changes the path to bash to use the Nix path).

After running `build-all` once, the necessary changes will have been applied to the files, and you could then do `cd ./src; make`

## Debugging

### Debugging nix develop

The `flake.nix` and embedded bash code make use of the environment variable `XDP2_NIX_DEBUG`. This variable uses syslog levels between 0 (default) and 7.

For maximum debugging:
```bash
XDP2_NIX_DEBUG=7 nix develop --verbose --print-build-logs
```

### Shellcheck

The `flake.nix` checks all the bash code within the flake to ensure there are no issues.

## Understanding the Environment

### Nix Derivation vs Development Shell

Please note that the purpose of the development shell is different from a "Derivation", so the differences are highlighted here.

**Derivation Build:**
- Source code is copied to `/nix/store/` (read-only)
- Build happens in isolated environment
- Output is final and immutable
- No access to your working directory

**Development Shell:**
- Works with your actual source code in your directory
- Can edit files and rebuild incrementally
- Has access to your git repository and local changes
- Allows interactive development

The `flake.nix` provides the "development shell": [Development environment with nix-shell](https://nixos.wiki/wiki/Development_environment_with_nix-shell)


### How the Nix Development Environment Works

The Nix development environment uses a Nix flake to define the development environment. The `flake.nix` file is located in the root of the project.

The Nix flake uses Nix package versions of all dependencies, rather than the Debian packages described in the [README.md](../README.md).

- All Nix packages are available at [NixOS/nixpkgs](https://github.com/NixOS/nixpkgs/) and are searchable at [search.nixos.org](https://search.nixos.org/packages?channel=unstable)

In this case, we are essentially mapping the Debian packages to their Nix equivalents.

| Debian Package | Nix Package | Purpose |
|----------------|-------------|---------|
| `build-essential` | `stdenv.cc` (part of stdenv) | C/C++ compiler and build tools |
| `gcc-multilib` | `gccMultiStdenv` | Multi-architecture GCC support |
| `pkg-config` | `pkg-config` | Package configuration tool |
| `bison` | `bison` | Parser generator |
| `flex` | `flex` | Lexical analyzer generator |
| `libboost-all-dev` | `boost` | C++ Boost libraries |
| `libpcap-dev` | `libpcap` | Packet capture library |
| `graphviz` | `graphviz` | Graph visualization software |
| `libelf-dev` | `libelf` | ELF library development files |
| `clang` | `clang` | LLVM C/C++ compiler |
| `llvm` | `llvm` | LLVM compiler infrastructure |
| `libbpf-dev` | `libbpf` | BPF library development files |
| `linux-tools-$(uname -r)` | `linuxPackages.bpftool` | BPF tools (kernel version specific) |

The `flake.nix` essentially sets up an environment with all these packages available. Please note that rather than being installed in `/usr` like on traditional systems, all packages are installed in the `/nix/store`, and then environment variables are configured to point to the correct packages.



### Makefiles and Environment Variables

The XDP2 package contains many Makefiles, which are listed here:

```bash
[das@l:~/Downloads/xdp2]$ find ./ -name "Makefile"
./platforms/default/src/include/arch/arch_generic/Makefile
./thirdparty/pcap-src/pcap/libpcap/Makefile
./thirdparty/pcap-src/pcap/Makefile
./thirdparty/json/Makefile
./thirdparty/json/test/Makefile
./thirdparty/json/doc/Makefile
./thirdparty/json/doc/mkdocs/Makefile
./thirdparty/json/doc/docset/Makefile
./thirdparty/cppfront/Makefile
./samples/Makefile
./samples/xdp/flow_tracker_simple/Makefile
./samples/xdp/Makefile
./samples/xdp/flow_tracker_tmpl/Makefile
./samples/xdp/flow_tracker_tlvs/Makefile
./samples/xdp/flow_tracker_combo/Makefile
./samples/parser/Makefile
./samples/parser/simple_parser/Makefile
./samples/parser/ports_parser/Makefile
./samples/parser/offset_parser/Makefile
./src/Makefile
./src/test/pvbuf/Makefile
./src/test/Makefile
./src/test/tables/Makefile
./src/test/accelerator/Makefile
./src/test/router/Makefile
./src/test/parser/Makefile
./src/test/parse_dump/Makefile
./src/test/timer/Makefile
./src/test/switch/Makefile
./src/test/bitmaps/Makefile
./src/test/vstructs/Makefile
./src/lib/murmur3hash/Makefile
./src/lib/siphash/Makefile
./src/lib/Makefile
./src/lib/parselite/Makefile
./src/lib/cli/Makefile
./src/lib/crc/Makefile
./src/lib/lzf/Makefile
./src/lib/xdp2/Makefile
./src/lib/flowdis/Makefile
./src/tools/Makefile
./src/tools/packets/Makefile
./src/tools/packets/uet/Makefile
./src/tools/packets/falcon/Makefile
./src/tools/packets/sue/Makefile
./src/tools/compiler/Makefile
./src/tools/pmacro/Makefile
./src/include/murmur3hash/Makefile
./src/include/siphash/Makefile
./src/include/Makefile
./src/include/parselite/Makefile
./src/include/cli/Makefile
./src/include/uet/Makefile
./src/include/crc/Makefile
./src/include/lzf/Makefile
./src/include/falcon/Makefile
./src/include/xdp2/Makefile
./src/include/xdp2/proto_defs/Makefile
./src/include/xdp2/parsers/Makefile
./src/include/sue/Makefile

[das@l:~/Downloads/xdp2]$
```

Having reviewed these Makefiles, the list of other tools used and their Nix package equivalents is listed in the following table. The Nix flake also ensures all these tools are available (these tools should probably be listed as requirements in the README.md).

| Tool | Nix Package | Purpose |
|------|-------------|---------|
| `gcc` | `gcc` | C compiler |
| `clang` | `clang` | LLVM C/C++ compiler |
| `clang++` | `clang` | LLVM C++ compiler |
| `make` | `gnumake` | Build system |
| `install` | `coreutils` | File installation utility |
| `sed` | `gnused` | Stream editor |
| `awk` | `gawk` | Text processing tool |
| `cat` | `coreutils` | File concatenation |
| `bison` | `bison` | Parser generator |
| `flex` | `flex` | Lexical analyzer generator |
| `llvm-config` | `llvm` | LLVM configuration tool |
| `python3` | `python3` | Python interpreter for packet generation |
| `tar` | `gnutar` | Archive utility |
| `xz` | `xz` | Compression utility |
| `clang-format` | `clang` | Code formatter |
| `git` | `git` | Version control system |
| `bash` | `bash` | Shell interpreter |
| `sh` | `bash` | POSIX shell |
| `cppfront-compiler` | Built from source | C++ front compiler (third-party) |

## Dependencies and Packages

### Boost Libraries

The build guide suggests installing `libboost-all-dev`, which includes many packages. However, XDP2 only uses a specific subset of Boost libraries, primarily in the XDP2 compiler tool.

### Boost Libraries Actually Used

Based on the codebase analysis, XDP2 uses these specific Boost libraries:

| Boost Library | Nix Package | Purpose |
|---------------|-------------|---------|
| `boost_wave` | `boost` | C++ preprocessor for template processing |
| `boost_thread` | `boost` | Threading support |
| `boost_filesystem` | `boost` | File system operations |
| `boost_system` | `boost` | System error handling |
| `boost_program_options` | `boost` | Command-line argument parsing |
| `boost_graph` | `boost` | Graph algorithms and data structures |
| `boost_range` | `boost` | Range-based algorithms |
| `boost_io` | `boost` | I/O stream state management |

### Usage Context

- **XDP2 Compiler**: Uses Boost libraries for graph processing, command-line parsing, and C++ preprocessing
- **Graph Processing**: Boost.Graph for parser graph analysis and optimization
- **Template Processing**: Boost.Wave for C++ template preprocessing
- **Build System**: Configure script checks for these specific Boost libraries

The Nix `boost` package provides all these libraries as a single package, making it more efficient than installing individual Debian boost packages.

### Environment Variables Reference

The Makefiles make use of the following environment variables, and in particular the `XDP2DIR` variable must be set by the Nix development shell to point to a directory where XDP2 has been built and installed. This enables users to run sample code after `git clone` and `nix develop` without additional setup.

| Environment Variable | Purpose |
|---------------------|---------|
| `XDP2DIR` | **Primary variable** - Points to XDP2 installation directory where libraries, headers, and binaries are located (set by Nix development environment) |
| `INSTALLDIR` | Installation directory for make install targets |
| `BUILD_OPT_PARSER` | Enable building of optimized parser (requires LLVM/Clang) |
| `CC` | C compiler (default: gcc) |
| `CXX` | C++ compiler (default: g++) |
| `HOST_CC` | Host C compiler for cross-compilation |
| `HOST_CXX` | Host C++ compiler for cross-compilation |
| `CCOPTS` | C compiler options (e.g., -O3, -g) |
| `CFLAGS` | C compiler flags |
| `CXXFLAGS` | C++ compiler flags |
| `XDP2_ARCH` | Target architecture (e.g., x86_64) |
| `ARCH` | Architecture identifier |
| `PYTHON_VER` | Python version (default: 3) |
| `CFLAGS_PYTHON` | Python C API compiler flags |
| `LDFLAGS_PYTHON` | Python C API linker flags |
| `XDP2_CLANG_VERSION` | Clang version for compiler integration |
| `XDP2_CLANG_RESOURCE_PATH` | Clang resource directory path |
| `HOST_LLVM_CONFIG` | LLVM configuration tool path |
| `LIBDIR` | Library installation directory (default: /lib) |
| `BINDIR` | Binary installation directory (default: /bin) |
| `HDRDIR` | Header installation directory (default: /include) |
| `SBINDIR` | System binary directory (default: /sbin) |
| `ETCDIR` | Configuration directory (default: /etc) |
| `CONFDIR` | Configuration directory (default: /etc) |
| `DATADIR` | Data directory (default: /share) |
| `MANDIR` | Manual page directory (default: /share/man) |
| `KERNEL_INCLUDE` | Kernel header directory (default: /usr/include) |
| `KDIR` | Kernel source directory |
| `VERBOSE` | Enable verbose make output |
| `V` | Verbose flag (1 for verbose, 0 for quiet) |
| `MAKEFLAGS` | Make flags for recursive calls |
| `MFLAGS` | Make flags |
| `LDFLAGS` | Linker flags |
| `LDLIBS` | Libraries to link against |
| `CPPFRONT` | Path to cppfront compiler |
| `TEMPLATES_PATH` | Path to XDP2 templates directory |

## Compiler Architecture

### Analysis of Multiple Compilers

XDP2 uses a **two-stage compilation approach** with different compilers for different purposes. This design allows the project to leverage the strengths of different compiler toolchains.

### Compiler Separation Architecture

The build system distinguishes between:
- **`HOST_CC`/`HOST_CXX`**: Used for building **tools** that run on the build machine
- **`CC`/`CXX`**: Used for building the **final XDP2 libraries and applications**

### Specific Compiler Usage Patterns

#### **HOST_CC/HOST_CXX is used for:**
- **Building the XDP2 compiler itself** (`xdp2-compiler`)
- **Building utility tools** like `pmacro_gen`, packet generators
- **Building code generation tools** that run during the build process
- **Building the cppfront compiler** (third-party tool)

#### **CC/CXX is used for:**
- **Building the main XDP2 libraries** (`libxdp2.so`, `libcli.so`, etc.)
- **Building test programs** and sample applications
- **Building the final optimized parsers** (after xdp2-compiler processes them)

### Compiler Usage by File/Directory

| **Usage Type** | **Compiler** | **Compiler Used** | **Files/Directories** | **Line Number** | **Line Content** | **Purpose** |
|---|---|---|---|---|---|---|
| **Variable** | **HOST_CC** | `HOST_CC` | `src/tools/pmacro/Makefile` | Line 8 | `$(QUIET_HOST_CC)$(HOST_CC) $(SRC) -o $@` | Build utility tools that run during build |
| **Variable** | **HOST_CC** | `HOST_CC` | `src/tools/packets/uet/Makefile` | Line 13 | `$(QUIET_CC)$(HOST_CC) $(CCOPTS) -I$(SRCDIR)/include $(CONFIG_DEFINES) $^ -o $@` | Build packet generation utilities |
| **Variable** | **HOST_CC** | `HOST_CC` | `src/tools/packets/falcon/Makefile` | Line 13 | `$(QUIET_CC)$(HOST_CC) $(CCOPTS) -I$(SRCDIR)/include $(CONFIG_DEFINES) $^ -o $@` | Build packet generation utilities |
| **Variable** | **HOST_CC** | `HOST_CC` | `src/tools/packets/sue/Makefile` | Line 13 | `$(QUIET_CC)$(HOST_CC) $(CCOPTS) -I$(SRCDIR)/include $(CONFIG_DEFINES) $^ -o $@` | Build packet generation utilities |
| **Variable** | **HOST_CC** | `HOST_CC` | `src/test/bitmaps/Makefile` | Line 20 | `$(QUIET_LINK)$(HOST_CC) -o $@ $^` | Build bitmap test utilities |
| **Variable** | **HOST_CXX** | `HOST_CXX` | `src/tools/compiler/Makefile` | Line 41 | `$(HOST_CXX) $^ -o $@ $(LLVM_LIBS) $(BOOST_LIBS) $(CLANG_LIBS) $(LDFLAGS_PYTHON) $(LIBS)` | Build XDP2 compiler with LLVM integration |
| **Variable** | **HOST_CXX** | `HOST_CXX` | `src/tools/compiler/Makefile` | Line 44 | `$(HOST_CXX) $(TESTOBJS) -o $@ $(BOOST_LIBS) $(LDFLAGS_PYTHON)` | Build XDP2 compiler test |
| **Variable** | **HOST_CXX** | `HOST_CXX` | `thirdparty/cppfront/Makefile` | Line 12 | `$(HOST_CXX) $(STD_FLAGS) $(SRC) -o $(TARGET)` | Build cppfront compiler (third-party) |
| **Variable** | **CC** | `CC` | `src/lib/xdp2/Makefile` | Line 45 | `$(CC) -shared $^ -o $@ -lpcap` | Build main XDP2 library |
| **Variable** | **CC** | `CC` | `src/lib/cli/Makefile` | Line 18 | `$(CC) $(LDLIBS) -shared $^ -o $@` | Build CLI library |
| **Variable** | **CC** | `CC` | `src/lib/parselite/Makefile` | Line 16 | `$(CC) -shared $^ -o $@` | Build parser library |
| **Variable** | **CC** | `CC` | `src/lib/siphash/Makefile` | Line 18 | `$(QUIET_CC)$(CC) $(LDLIBS) -shared $^ -o $@` | Build hash library |
| **Variable** | **CC** | `CC` | `src/lib/crc/Makefile` | Line 18 | `$(QUIET_CC)$(CC) $(LDLIBS) -shared $^ -o $@` | Build CRC library |
| **Variable** | **CC** | `CC` | `src/lib/flowdis/Makefile` | Line 18 | `$(CC) $(LDLIBS) -shared $^ -o $@` | Build flow dissector library |
| **Variable** | **CC** | `CC` | `src/lib/lzf/Makefile` | Line 19 | `$(QUIET_CC)$(CC) $(LDLIBS) -shared $^ -o $@` | Build compression library |
| **Variable** | **CC** | `CC` | `src/lib/lzf/Makefile` | Line 25 | `$(QUIET_CC)$(CC) $(LDLIBS) -shared $^ -o $@` | Build compression library |
| **Variable** | **CC** | `CC` | `src/lib/murmur3hash/Makefile` | Line 17 | `$(QUIET_CC)$(CC) $(LDLIBS) -shared $^ -o $@` | Build hash library |
| **Variable** | **CC** | `CC` | `src/test/parser/Makefile` | Line 42 | `$(CC) $(LDFLAGS) -o test_parser $(OBJ) $(LIBS)` | Build parser test applications |
| **Variable** | **CC** | `CC` | `src/test/tables/Makefile` | Line 21 | `$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@` | Build table test applications |
| **Variable** | **CC** | `CC` | `src/test/pvbuf/Makefile` | Line 15 | `$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@` | Build pvbuf test applications |
| **Variable** | **CC** | `CC` | `src/test/accelerator/Makefile` | Line 16 | `$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@` | Build accelerator test applications |
| **Variable** | **CC** | `CC` | `src/test/router/Makefile` | Line 19 | `$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@` | Build router test applications |
| **Variable** | **CC** | `CC` | `src/test/parse_dump/Makefile` | Line 22 | `$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@` | Build parse dump test applications |
| **Variable** | **CC** | `CC` | `src/test/parse_dump/Makefile` | Line 26 | `$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@` | Build parse dump test applications |
| **Variable** | **CC** | `CC` | `src/test/parse_dump/Makefile` | Line 29 | `$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@` | Build parse dump test applications |
| **Variable** | **CC** | `CC` | `src/test/timer/Makefile` | Line 14 | `$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@` | Build timer test applications |
| **Variable** | **CC** | `CC` | `src/test/switch/Makefile` | Line 13 | `$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@` | Build switch test applications |
| **Variable** | **CC** | `CC` | `src/test/bitmaps/Makefile` | Line 17 | `$(QUIET_LINK)$(CC) test_bitmap.o $(LDFLAGS) $(LDLIBS) -o $@` | Build bitmap test applications |
| **Variable** | **CC** | `CC` | `src/test/vstructs/Makefile` | Line 17 | `$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@ $(LDLIBS_LOCAL)` | Build vstructs test applications |
| **Direct Binary** | **gcc** | `CC= gcc` | `samples/parser/simple_parser/Makefile` | Line 12 | `CC= gcc` | Explicitly set CC to gcc binary |
| **Variable** | **CC** | `CC` | `samples/parser/simple_parser/Makefile` | Line 33 | `$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lpcap -lxdp2 -lcli -lsiphash` | Use CC variable (set to gcc above) |
| **Variable** | **CC** | `CC` | `samples/parser/simple_parser/Makefile` | Line 39 | `$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lpcap -lxdp2 -lcli -lsiphash` | Use CC variable (set to gcc above) |
| **Direct Binary** | **gcc** | `CC= gcc` | `samples/parser/ports_parser/Makefile` | Line 12 | `CC= gcc` | Explicitly set CC to gcc binary |
| **Variable** | **CC** | `CC` | `samples/parser/ports_parser/Makefile` | Line 31 | `$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lpcap -lxdp2 -lcli -lsiphash` | Use CC variable (set to gcc above) |
| **Direct Binary** | **gcc** | `CC= gcc` | `samples/parser/offset_parser/Makefile` | Line 12 | `CC= gcc` | Explicitly set CC to gcc binary |
| **Variable** | **CC** | `CC` | `samples/parser/offset_parser/Makefile` | Line 31 | `$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lpcap -lxdp2 -lcli -lsiphash` | Use CC variable (set to gcc above) |
| **Direct Binary** | **clang** | `CC= clang` | `samples/xdp/flow_tracker_simple/Makefile` | Line 12 | `CC= clang # Use clang just to test it` | Explicitly set CC to clang binary |
| **Variable** | **CC** | `CC` | `samples/xdp/flow_tracker_simple/Makefile` | Line 31 | `$(CC) -x c -target bpf $(CFLAGS) $(LDFLAGS) -c -o $@ $<` | Use CC variable (set to clang above) |
| **Direct Binary** | **clang** | `CC= clang` | `samples/xdp/flow_tracker_tmpl/Makefile` | Line 12 | `CC= clang # Use clang just to test it` | Explicitly set CC to clang binary |
| **Variable** | **CC** | `CC` | `samples/xdp/flow_tracker_tmpl/Makefile` | Line 31 | `$(CC) -x c -target bpf $(CFLAGS) $(LDFLAGS) -c -o $@ $<` | Use CC variable (set to clang above) |
| **Direct Binary** | **clang** | `XCC= clang` | `samples/xdp/flow_tracker_tlvs/Makefile` | Line 12 | `XCC= clang` | Explicitly set XCC to clang binary |
| **Variable** | **CC** | `CC` | `samples/xdp/flow_tracker_tlvs/Makefile` | Line 24 | `$(CC) -I$(INCDIR) -c -o parser.o $<` | Use CC variable for parser compilation |
| **Variable** | **XCC** | `XCC` | `samples/xdp/flow_tracker_tlvs/Makefile` | Line 28 | `$(XCC) -save-temps -fverbose-asm -x c -target bpf $(XCFLAGS) $(XLDFLAGS) -c -o $@ $<` | Use XCC variable for BPF compilation |
| **Direct Binary** | **clang** | `XCC= clang` | `samples/xdp/flow_tracker_combo/Makefile` | Line 12 | `XCC= clang` | Explicitly set XCC to clang binary |
| **Direct Binary** | **gcc** | `CC= gcc` | `samples/xdp/flow_tracker_combo/Makefile` | Line 17 | `CC= gcc` | Explicitly set CC to gcc binary |
| **Variable** | **XCC** | `XCC` | `samples/xdp/flow_tracker_combo/Makefile` | Line 32 | `$(XCC) -x c -target bpf -I$(INCDIR) $(XCFLAGS) $(XLDFLAGS) -c -o $@ $<` | Use XCC variable for BPF compilation |
| **Variable** | **CC** | `CC` | `samples/xdp/flow_tracker_combo/Makefile` | Line 38 | `$(CC) $(CFLAGS) $(LDFLAGS) -o $@ flow_parser.c parser.p.c -lpcap -lxdp2 -lcli -lsiphash` | Use CC variable for final linking |
| **Direct Binary** | **gcc** | `CC := gcc` | `src/Makefile` | Line 49 | `CC := gcc` | Default CC setting for main build |
| **Direct Binary** | **gcc** | `CC := gcc` | `thirdparty/pcap-src/pcap/Makefile` | Line 43 | `CC := gcc` | Third-party pcap library build |
| **Variable** | **CC** | `CC` | `thirdparty/pcap-src/pcap/libpcap/Makefile` | Line 30 | `$(QUIET_CC)$(CC) $(CFLAGS) -I $(CURRDIR)/include-linux/uapi\` | Third-party pcap library compilation |
| **Variable** | **CC** | `CC` | `thirdparty/pcap-src/pcap/libpcap/Makefile` | Line 45 | `HOSTCC ?= $(CC)` | Third-party pcap library host compiler |
| **Variable** | **CC** | `CC` | `src/Makefile` | Line 51 | `HOSTCC ?= $(CC)` | Main build system host compiler |
| **Variable** | **CXX** | `CXX` | `thirdparty/json/test/Makefile` | Line 17 | `$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(FUZZER_ENGINE) src/fuzzer-parse_json.cpp -o $@` | Third-party JSON fuzzer compilation |
| **Variable** | **CXX** | `CXX` | `thirdparty/json/test/Makefile` | Line 20 | `$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(FUZZER_ENGINE) src/fuzzer-parse_bson.cpp -o $@` | Third-party JSON fuzzer compilation |
| **Variable** | **CXX** | `CXX` | `thirdparty/json/test/Makefile` | Line 23 | `$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(FUZZER_ENGINE) src/fuzzer-parse_cbor.cpp -o $@` | Third-party JSON fuzzer compilation |
| **Variable** | **CXX** | `CXX` | `thirdparty/json/test/Makefile` | Line 26 | `$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(FUZZER_ENGINE) src/fuzzer-parse_msgpack.cpp -o $@` | Third-party JSON fuzzer compilation |
| **Variable** | **CXX** | `CXX` | `thirdparty/json/test/Makefile` | Line 29 | `$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(FUZZER_ENGINE) src/fuzzer-parse_ubjson.cpp -o $@` | Third-party JSON fuzzer compilation |
| **Config Variable** | **HOST_LLVM_CONFIG** | `HOST_LLVM_CONFIG` | `src/tools/compiler/Makefile` | Line 21 | `LLVM_INCLUDE ?= -I\`$(HOST_LLVM_CONFIG) --includedir\`` | LLVM include path configuration |
| **Config Variable** | **HOST_LLVM_CONFIG** | `HOST_LLVM_CONFIG` | `src/tools/compiler/Makefile` | Line 22 | `LLVM_LIBS ?= -L\`$(HOST_LLVM_CONFIG) --libdir\`` | LLVM library path configuration |
| **Config Variable** | **XDP2_CLANG_VERSION** | `XDP2_CLANG_VERSION` | `src/tools/compiler/Makefile` | Line 19 | `CLANG_INFO ?= -DXDP2_CLANG_VERSION="$(XDP2_CLANG_VERSION)" -DXDP2_CLANG_RESOURCE_PATH="$(XDP2_CLANG_RESOURCE_PATH)"` | Clang version configuration |
| **Config Variable** | **XDP2_CLANG_RESOURCE_PATH** | `XDP2_CLANG_RESOURCE_PATH` | `src/tools/compiler/Makefile` | Line 19 | `CLANG_INFO ?= -DXDP2_CLANG_VERSION="$(XDP2_CLANG_VERSION)" -DXDP2_CLANG_RESOURCE_PATH="$(XDP2_CLANG_RESOURCE_PATH)"` | Clang resource path configuration |

### Why They Don't Conflict

The compilers don't conflict because they're used for **different stages** of the build process:

1. **Stage 1 - Tool Building**: `HOST_CC`/`HOST_CXX` builds tools that run on the build machine
2. **Stage 2 - Code Generation**: The built tools (like `xdp2-compiler`) process source files
3. **Stage 3 - Final Compilation**: `CC`/`CXX` compiles the generated code into final binaries

### Example: Parser Optimization Workflow

```makefile
# Stage 1: Build the xdp2-compiler tool (using HOST_CXX)
xdp2-compiler: $(OBJS)
	$(HOST_CXX) $^ -o $@ $(LLVM_LIBS) $(BOOST_LIBS) $(CLANG_LIBS) ...

# Stage 2: Use xdp2-compiler to generate optimized parser code
$(OPT_PARSER_SRC): parser.c parser.o
	$(XDP2_COMPILER) -I$(SRCDIR)/include -o $@ -i $<

# Stage 3: Compile the generated code into final binary (using CC)
$(TARGET): $(OBJ) $(OPT_PARSER_OBJ)
	$(QUIET_LINK)$(CC) $^ $(LDLIBS) -o $@
```

### Why This Architecture?

- **Cross-compilation support**: You can build XDP2 tools on one architecture and compile XDP2 programs for another
- **Tool isolation**: Build tools don't need to be optimized for the target platform
- **LLVM/Clang integration**: The XDP2 compiler uses LLVM/Clang libraries for AST parsing, but the final code can be compiled with GCC for better optimization
- **Flexibility**: Different compilers can be used for different optimization goals

### In Practice

- **GCC** is typically used as the default `CC` for building the final libraries and applications
- **Clang** (via `HOST_CXX`) is used for building the XDP2 compiler because it needs LLVM integration
- The **xdp2-compiler** tool uses Clang's AST parsing capabilities to analyze C code and generate optimized parser code
- The **generated code** is then compiled with GCC for maximum performance

This design allows XDP2 to leverage the best of both worlds: Clang's excellent C++ tooling and AST manipulation for the compiler, and GCC's mature optimization for the final runtime code.

## Nix Development Environment Implications

Given the multiple compiler architecture, the Nix development environment must provide both compiler toolchains and ensure they work together properly.

### Required Environment Variables

The Nix flake must set up the following environment variables to support the dual-compiler architecture:

| Environment Variable | Nix Package Source | Purpose |
|---------------------|-------------------|---------|
| `CC` | `gcc` | Primary C compiler for final libraries/applications |
| `CXX` | `gcc` (g++) | Primary C++ compiler for final libraries/applications |
| `HOST_CC` | `gcc` | Host C compiler for building tools |
| `HOST_CXX` | `clang` | Host C++ compiler for building XDP2 compiler (needs LLVM) |
| `HOST_LLVM_CONFIG` | `llvm` | LLVM configuration tool for Clang integration |
| `XDP2_CLANG_VERSION` | From `clang` package | Clang version for compiler integration |
| `XDP2_CLANG_RESOURCE_PATH` | From `clang` package | Clang resource directory path |

### Key Considerations

1. **LLVM/Clang Integration**: The `HOST_CXX` must be Clang because the XDP2 compiler requires LLVM libraries for AST parsing
2. **GCC Optimization**: The primary `CC`/`CXX` should be GCC for optimal performance of the final runtime code
3. **Cross-compilation Ready**: The separation allows for future cross-compilation scenarios
4. **Tool Chain Compatibility**: Both compilers must be able to produce compatible object files and libraries

## Configure Script Challenges

The configure script presents several challenges in the Nix environment. For detailed analysis and solutions, see [nix_configure.md](nix_configure.md).

### Required Environment Variables

The Nix development environment must provide all necessary environment variables for the configure script to function properly.

### Nix Flake Configuration

The `flake.nix` should ensure:
- Both `gcc` and `clang` are available in the development environment
- `HOST_LLVM_CONFIG` points to the correct LLVM configuration tool
- Clang version and resource path are properly set for the XDP2 compiler
- All necessary libraries (Boost, LLVM, libbpf, etc.) are available to both compiler toolchains

This setup ensures that developers can build both the XDP2 compiler tools and the final XDP2 applications within the same Nix development environment.

## Troubleshooting

### Common Issues

**Issue: `nix develop` fails with "experimental features" error**
```bash
# Solution: Enable flakes permanently
echo 'experimental-features = nix-command flakes' | sudo tee -a /etc/nix/nix.conf
```

**Issue: Build fails with "command not found" errors**
```bash
# Solution: Ensure you're inside the nix develop shell
nix develop
# Then run build-all
build-all
```

**Issue: Permission denied errors**
```bash
# Solution: Check if you have proper permissions for the directory
ls -la
# Make sure you own the files or have write permissions
```

**Issue: Outdated config.mk warnings**
```bash
# Solution: Regenerate the configuration
rm src/config.mk
# Exit and re-enter nix develop to trigger smart-configure
exit
nix develop
```
