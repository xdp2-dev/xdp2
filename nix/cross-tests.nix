# nix/cross-tests.nix
#
# Cross-compiled test infrastructure for XDP2.
#
# This module provides cross-compiled test derivations for non-native architectures.
# The tests are compiled on the build host (x86_64) but run on the target arch
# (riscv64, aarch64) via QEMU microvm.
#
# Usage:
#   # Build cross-compiled tests for riscv64
#   nix build .#riscv64-tests.all
#
#   # Start the RISC-V VM and run tests inside
#   nix run .#riscv64-test-runner
#
{ pkgs, lib, nixpkgs, targetArch }:

let
  # Cross-compilation configuration
  crossConfig = {
    riscv64 = {
      system = "riscv64-linux";
      crossSystem = "riscv64-linux";
    };
    aarch64 = {
      system = "aarch64-linux";
      crossSystem = "aarch64-linux";
    };
  };

  cfg = crossConfig.${targetArch} or (throw "Unsupported target architecture: ${targetArch}");

  # Import cross-compilation pkgs
  pkgsCross = import nixpkgs {
    localSystem = "x86_64-linux";
    crossSystem = cfg.crossSystem;
    config = { allowUnfree = true; };
    overlays = [
      # Disable failing tests under cross-compilation
      (final: prev: {
        boehmgc = prev.boehmgc.overrideAttrs (old: { doCheck = false; });
        libuv = prev.libuv.overrideAttrs (old: { doCheck = false; });
        meson = prev.meson.overrideAttrs (old: { doCheck = false; doInstallCheck = false; });
      })
    ];
  };

  # Import LLVM configuration for cross target
  llvmConfig = import ./llvm.nix { pkgs = pkgsCross; lib = pkgsCross.lib; };
  llvmPackages = llvmConfig.llvmPackages;

  # Import packages module for cross target
  packagesModule = import ./packages.nix { pkgs = pkgsCross; inherit llvmPackages; };

  # Cross-compiled xdp2-debug
  xdp2-debug = import ./derivation.nix {
    pkgs = pkgsCross;
    lib = pkgsCross.lib;
    inherit llvmConfig;
    inherit (packagesModule) nativeBuildInputs buildInputs;
    enableAsserts = true;
  };

  # Cross-compiled tests
  tests = import ./tests {
    pkgs = pkgsCross;
    xdp2 = xdp2-debug;
  };

in {
  # Cross-compiled xdp2
  inherit xdp2-debug;

  # Cross-compiled tests
  inherit tests;

  # Test package path (for use in VM)
  testAllPath = tests.all;

  # Info for documentation
  info = {
    inherit targetArch;
    system = cfg.system;
    xdp2Path = xdp2-debug;
    testsPath = tests.all;
  };
}
