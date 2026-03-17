# nix/packaging/default.nix
#
# Entry point for XDP2 package generation.
#
# Phase 1: x86_64 .deb only
# See: documentation/nix/microvm-implementation-phase1.md
#
# Usage in flake.nix:
#   packaging = import ./nix/packaging { inherit pkgs lib xdp2; };
#   packages.deb-x86_64 = packaging.deb.x86_64;
#
{ pkgs, lib, xdp2 }:

let
  # Import .deb packaging module
  debPackaging = import ./deb.nix { inherit pkgs lib xdp2; };

in {
  # Phase 1: x86_64 .deb only
  # The architecture is determined by pkgs.stdenv.hostPlatform.system
  deb = {
    x86_64 = debPackaging.deb;
  };

  # Staging directories (for debugging/inspection)
  staging = {
    x86_64 = debPackaging.staging;
  };

  # Metadata (for use by other modules)
  metadata = debPackaging.metadata;

  # Architecture info (for debugging)
  archInfo = {
    detected = debPackaging.arch;
    system = pkgs.stdenv.hostPlatform.system;
  };
}
