# nix/packaging/metadata.nix
#
# Package metadata for XDP2 distribution packages.
# Single source of truth for package information.
#
# Phase 1: x86_64 .deb only
# See: documentation/nix/microvm-implementation-phase1.md
#
{
  # Package identity
  name = "xdp2";
  version = "0.1.0";

  # Maintainer info
  maintainer = "XDP2 Team <team@xdp2.dev>";

  # Package description
  description = "High-performance packet processing framework using eBPF/XDP";
  # Long description for Debian: continuation lines must start with space,
  # blank lines must be " ." (space-dot)
  longDescription = ''
    XDP2 is a packet processing framework that generates eBPF/XDP programs
    for high-speed packet handling in the Linux kernel.
    .
    Features:
    - xdp2-compiler: Code generator for packet parsers
    - Libraries for packet parsing and flow tracking
    - Templates for common packet processing patterns'';

  # Project info
  homepage = "https://github.com/xdp2/xdp2";
  license = "MIT";

  # Debian package dependencies (runtime)
  # These are the packages that must be installed for xdp2 to run
  debDepends = [
    "libc6"
    "libstdc++6"
    "libboost-filesystem1.83.0 | libboost-filesystem1.74.0"
    "libboost-program-options1.83.0 | libboost-program-options1.74.0"
    "libelf1"
  ];

  # Architecture mappings
  # Maps Nix system to Debian architecture name
  debArchMap = {
    "x86_64-linux" = "amd64";
    "aarch64-linux" = "arm64";
    "riscv64-linux" = "riscv64";
    "riscv32-linux" = "riscv32";
    "armv7l-linux" = "armhf";
  };
}
