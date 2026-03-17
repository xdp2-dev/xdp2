#
# flake.nix for XDP2
#
# This flake provides:
# - Development environment: nix develop
# - Package build: nix build
#
# To enter the development environment:
# nix develop
#
# To build the package:
# nix build .#xdp2
#
# If flakes are not enabled, use the following command:
# nix --extra-experimental-features 'nix-command flakes' develop .
# nix --extra-experimental-features 'nix-command flakes' build .
#
# To enable flakes, you may need to enable them in your system configuration:
# test -d /etc/nix || sudo mkdir /etc/nix
# echo 'experimental-features = nix-command flakes' | sudo tee -a /etc/nix/nix.conf
#
# Debugging:
# XDP2_NIX_DEBUG=7 nix develop --verbose --print-build-logs
#
# Alternative commands:
# nix --extra-experimental-features 'nix-command flakes' --option eval-cache false develop
# nix --extra-experimental-features 'nix-command flakes' develop --no-write-lock-file
# nix --extra-experimental-features 'nix-command flakes' print-dev-env --json
#
# Recommended term:
# export TERM=xterm-256color
#
{
  description = "XDP2 packet processing framework";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        lib = nixpkgs.lib;

        # Import LLVM configuration module
        # Use default LLVM version from nixpkgs (no pinning required)
        llvmConfig = import ./nix/llvm.nix { inherit pkgs lib; };
        llvmPackages = llvmConfig.llvmPackages;

        # Import packages module
        packagesModule = import ./nix/packages.nix { inherit pkgs llvmPackages; };

        # Compiler configuration
        compilerConfig = {
          cc = pkgs.gcc;
          cxx = pkgs.gcc;
          ccBin = "gcc";
          cxxBin = "g++";
        };

        # Import environment variables module
        envVars = import ./nix/env-vars.nix {
          inherit pkgs llvmConfig compilerConfig;
          packages = packagesModule;
          configAgeWarningDays = 14;
        };

        # Import package derivation (production build, assertions disabled)
        xdp2 = import ./nix/derivation.nix {
          inherit pkgs lib llvmConfig;
          inherit (packagesModule) nativeBuildInputs buildInputs;
          enableAsserts = false;
        };

        # Debug build with assertions enabled
        xdp2-debug = import ./nix/derivation.nix {
          inherit pkgs lib llvmConfig;
          inherit (packagesModule) nativeBuildInputs buildInputs;
          enableAsserts = true;
        };

        # Import development shell module
        devshell = import ./nix/devshell.nix {
          inherit pkgs lib llvmConfig compilerConfig envVars;
          packages = packagesModule;
        };

      in
      {
        # Package outputs
        packages = {
          default = xdp2;
          xdp2 = xdp2;
          xdp2-debug = xdp2-debug;  # Debug build with assertions
        };

        # Development shell
        devShells.default = devshell;
      });
}
