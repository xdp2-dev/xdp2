# nix/packages.nix
#
# Package definitions for XDP2
#
# This module defines all package dependencies, properly separated into:
# - nativeBuildInputs: Build-time tools (compilers, generators, etc.)
# - buildInputs: Libraries needed at build and runtime
# - devTools: Additional tools for development only
#
# Usage in flake.nix:
#   packages = import ./nix/packages.nix { inherit pkgs llvmPackages; };
#

{ pkgs, llvmPackages }:

let
  # Create a Python environment with scapy for packet generation
  pythonWithScapy = pkgs.python3.withPackages (ps: [ ps.scapy ]);
in
{
  # Build-time tools only - these run on the build machine
  nativeBuildInputs = [
    # Build system
    pkgs.gnumake
    pkgs.pkg-config
    pkgs.bison
    pkgs.flex

    # Core utilities needed during build
    pkgs.bash
    pkgs.coreutils
    pkgs.gnused
    pkgs.gawk
    pkgs.gnutar
    pkgs.xz
    pkgs.git

    # Compilers
    pkgs.gcc
    llvmPackages.clang
    llvmPackages.llvm.dev  # Provides llvm-config
    llvmPackages.lld
  ];

  # Libraries needed at build and runtime
  buildInputs = [
    # Core libraries
    pkgs.boost
    pkgs.libpcap
    pkgs.libelf
    pkgs.libbpf

    # Linux kernel headers (provides <linux/types.h> etc.)
    pkgs.linuxHeaders

    # Python environment
    pythonWithScapy

    # LLVM/Clang libraries (needed for xdp2-compiler)
    llvmPackages.llvm
    llvmPackages.libclang
    llvmPackages.clang-unwrapped
  ];

  # Development-only tools (not needed for building, only for dev workflow)
  devTools = [
    # Debugging
    pkgs.gdb
    pkgs.valgrind
    pkgs.strace
    pkgs.ltrace
    pkgs.glibc_multi.bin

    # BPF/XDP development tools
    pkgs.bpftools
    pkgs.bpftrace        # High-level tracing language for eBPF
    pkgs.bcc             # BPF Compiler Collection with Python bindings
    pkgs.perf  # Linux performance analysis tool
    pkgs.pahole          # DWARF debugging info analyzer (useful for BTF)

    # Visualization
    pkgs.graphviz

    # Code quality
    pkgs.shellcheck
    llvmPackages.clang-tools  # clang-tidy, clang-format, etc.

    # Utilities
    pkgs.jp2a          # ASCII art for logo
    pkgs.glibcLocales  # Locale support
  ];

  # Combined list for dev shell (all packages)
  # This replaces the old corePackages
  allPackages =
    let self = import ./packages.nix { inherit pkgs llvmPackages; };
    in self.nativeBuildInputs ++ self.buildInputs ++ self.devTools;

  # Export pythonWithScapy for use elsewhere
  inherit pythonWithScapy;
}
