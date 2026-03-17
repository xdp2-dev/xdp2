# nix/tests/default.nix
#
# Test definitions for XDP2 samples
#
# This module exports test derivations that verify XDP2 samples work correctly.
# Tests are implemented as writeShellApplication scripts that can be run
# after building with `nix build`.
#
# Two modes of operation:
# 1. Native mode (default): Tests build samples at runtime using xdp2-compiler
# 2. Pre-built mode: Tests use pre-compiled sample binaries (for cross-compilation)
#
# Usage:
#   # Native mode (x86_64 host running x86_64 tests)
#   nix build .#tests.simple-parser && ./result/bin/xdp2-test-simple-parser
#   nix build .#tests.all && ./result/bin/xdp2-test-all
#
#   # Pre-built mode (for RISC-V cross-compilation)
#   prebuiltSamples = import ./nix/samples { ... };
#   tests = import ./nix/tests {
#     inherit pkgs xdp2;
#     prebuiltSamples = prebuiltSamples;
#   };
#
# Future: VM-based tests for XDP samples that require kernel access
#

{ pkgs
, xdp2
  # Pre-built samples for cross-compilation (optional)
  # When provided, tests will use pre-compiled binaries instead of building at runtime
, prebuiltSamples ? null
}:

let
  # Determine if we're using pre-built samples
  usePrebuilt = prebuiltSamples != null;

  # Import test modules with appropriate mode
  simpleParser = import ./simple-parser.nix {
    inherit pkgs xdp2;
    prebuiltSample = if usePrebuilt then prebuiltSamples.simple-parser else null;
  };

  offsetParser = import ./offset-parser.nix {
    inherit pkgs xdp2;
    prebuiltSample = if usePrebuilt then prebuiltSamples.offset-parser else null;
  };

  portsParser = import ./ports-parser.nix {
    inherit pkgs xdp2;
    prebuiltSample = if usePrebuilt then prebuiltSamples.ports-parser else null;
  };

  flowTrackerCombo = import ./flow-tracker-combo.nix {
    inherit pkgs xdp2;
    prebuiltSample = if usePrebuilt then prebuiltSamples.flow-tracker-combo else null;
  };

in {
  # Parser sample tests (userspace, no root required)
  simple-parser = simpleParser;
  offset-parser = offsetParser;
  ports-parser = portsParser;

  # Debug test for diagnosing optimized parser issues
  simple-parser-debug = import ./simple-parser-debug.nix { inherit pkgs xdp2; };

  # XDP sample tests
  flow-tracker-combo = flowTrackerCombo;

  # XDP build verification (compile-only, no runtime test)
  xdp-build = import ./xdp-build.nix { inherit pkgs xdp2; };

  # Combined test runner
  all = pkgs.writeShellApplication {
    name = "xdp2-test-all";
    runtimeInputs = [];
    text = ''
      echo "=== Running all XDP2 tests ==="
      echo ""
      ${if usePrebuilt then ''echo "Mode: Pre-built samples (cross-compilation)"'' else ''echo "Mode: Runtime compilation (native)"''}
      echo ""

      # Phase 1: Parser sample tests
      echo "=== Phase 1: Parser Samples ==="
      echo ""

      # Run simple-parser test
      ${simpleParser}/bin/xdp2-test-simple-parser

      echo ""

      # Run offset-parser test
      ${offsetParser}/bin/xdp2-test-offset-parser

      echo ""

      # Run ports-parser test
      ${portsParser}/bin/xdp2-test-ports-parser

      echo ""

      # Phase 2: XDP sample tests
      echo "=== Phase 2: XDP Samples ==="
      echo ""

      # Run flow-tracker-combo test (userspace + XDP build)
      ${flowTrackerCombo}/bin/xdp2-test-flow-tracker-combo

      echo ""

      # Run XDP build verification tests
      ${import ./xdp-build.nix { inherit pkgs xdp2; }}/bin/xdp2-test-xdp-build

      echo ""
      echo "=== All tests completed ==="
    '';
  };
}
