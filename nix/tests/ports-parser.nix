# nix/tests/ports-parser.nix
#
# Test for the ports_parser sample
#
# This test verifies that:
# 1. The ports_parser sample builds successfully using the installed xdp2
# 2. The parser binary runs and produces expected output (IP:PORT pairs)
# 3. The optimized parser (-O flag) also works correctly
#
# Note: ports_parser only handles IPv4 (no IPv6 support), so we use tcp_ipv4.pcap
#
# Supports two modes:
# - Native: Builds sample at runtime using xdp2-compiler
# - Pre-built: Uses pre-compiled binaries (for cross-compilation)
#
# Usage:
#   nix build .#tests.ports-parser
#   ./result/bin/xdp2-test-ports-parser
#

{ pkgs
, xdp2
  # Pre-built sample derivation (optional, for cross-compilation)
, prebuiltSample ? null
}:

let
  # Source directory for test data (pcap files)
  testData = ../..;

  # LLVM config for getting correct clang paths
  llvmConfig = import ../llvm.nix { inherit pkgs; lib = pkgs.lib; };

  # Determine if we're using pre-built samples
  usePrebuilt = prebuiltSample != null;
in
pkgs.writeShellApplication {
  name = "xdp2-test-ports-parser";

  runtimeInputs = if usePrebuilt then [
    pkgs.coreutils
    pkgs.gnugrep
  ] else [
    pkgs.gnumake
    pkgs.gcc
    pkgs.coreutils
    pkgs.gnugrep
    pkgs.libpcap       # For pcap.h
    pkgs.libpcap.lib   # For -lpcap (library in separate output)
    pkgs.linuxHeaders  # For <linux/types.h> etc.
  ];

  text = ''
    set -euo pipefail

    echo "=== XDP2 ports_parser Test ==="
    echo ""
    ${if usePrebuilt then ''echo "Mode: Pre-built samples"'' else ''echo "Mode: Runtime compilation"''}
    echo ""

    ${if usePrebuilt then ''
    # Pre-built mode: Use binary from prebuiltSample
    PARSER="${prebuiltSample}/bin/parser"

    echo "Using pre-built binary:"
    echo "  parser: $PARSER"
    echo ""

    # Verify binary exists
    if [[ ! -x "$PARSER" ]]; then
      echo "FAIL: parser binary not found at $PARSER"
      exit 1
    fi
    '' else ''
    # Runtime compilation mode: Build from source
    WORKDIR=$(mktemp -d)
    trap 'rm -rf "$WORKDIR"' EXIT

    echo "Work directory: $WORKDIR"
    echo ""

    # Copy sample sources to writable directory
    cp -r ${testData}/samples/parser/ports_parser/* "$WORKDIR/"
    cd "$WORKDIR"

    # Make all files writable (nix store files are read-only)
    chmod -R u+w .

    # Remove any pre-existing generated files to force rebuild
    rm -f ./*.p.c ./*.o 2>/dev/null || true

    # Set up environment
    export XDP2DIR="${xdp2}"
    export LD_LIBRARY_PATH="${xdp2}/lib:${pkgs.libpcap.lib}/lib''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export PATH="${xdp2}/bin:$PATH"

    # Include paths for xdp2-compiler's libclang usage
    # These are needed because ClangTool bypasses the Nix clang wrapper
    export XDP2_C_INCLUDE_PATH="${llvmConfig.paths.clangResourceDir}/include"
    export XDP2_GLIBC_INCLUDE_PATH="${pkgs.stdenv.cc.libc.dev}/include"
    export XDP2_LINUX_HEADERS_PATH="${pkgs.linuxHeaders}/include"

    # Add libpcap to compiler paths
    export CFLAGS="-I${pkgs.libpcap}/include"
    export LDFLAGS="-L${pkgs.libpcap.lib}/lib"

    echo "XDP2DIR: $XDP2DIR"
    echo "LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
    echo ""

    # Build the sample
    echo "--- Building ports_parser ---"
    make XDP2DIR="${xdp2}" CFLAGS="-I${xdp2}/include -I${pkgs.libpcap}/include -g" LDFLAGS="-L${xdp2}/lib -L${pkgs.libpcap.lib}/lib -Wl,-rpath,${xdp2}/lib -Wl,-rpath,${pkgs.libpcap.lib}/lib"
    echo ""

    PARSER="./parser"
    ''}

    # Track test results
    TESTS_PASSED=0
    TESTS_FAILED=0

    pass() {
      echo "PASS: $1"
      TESTS_PASSED=$((TESTS_PASSED + 1))
    }

    fail() {
      echo "FAIL: $1"
      TESTS_FAILED=$((TESTS_FAILED + 1))
    }

    # Verify binary was created
    if [[ ! -x "$PARSER" ]]; then
      fail "parser binary not found"
      exit 1
    fi
    pass "parser binary created"
    echo ""

    # Test pcap file (IPv4 traffic - ports_parser only supports IPv4)
    PCAP="${testData}/data/pcaps/tcp_ipv4.pcap"

    if [[ ! -f "$PCAP" ]]; then
      echo "FAIL: Test pcap file not found: $PCAP"
      exit 1
    fi

    # Test 1: parser basic run
    echo "--- Test 1: parser basic ---"
    OUTPUT=$("$PARSER" "$PCAP" 2>&1) || {
      fail "parser exited with error"
      echo "$OUTPUT"
      exit 1
    }

    if echo "$OUTPUT" | grep -q "Packet"; then
      pass "parser produced Packet output"
    else
      fail "parser did not produce expected Packet output"
      echo "Output was:"
      echo "$OUTPUT"
    fi

    # Check for IP address format (contains dots like 10.0.2.15)
    if echo "$OUTPUT" | grep -qE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+'; then
      pass "parser produced IP addresses"
    else
      fail "parser did not produce expected IP address format"
      echo "Output was:"
      echo "$OUTPUT"
    fi

    # Check for port numbers (IP:PORT format with colon)
    if echo "$OUTPUT" | grep -qE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+:[0-9]+'; then
      pass "parser produced IP:PORT format"
    else
      fail "parser did not produce expected IP:PORT format"
      echo "Output was:"
      echo "$OUTPUT"
    fi

    # Check for arrow format (-> between source and destination)
    if echo "$OUTPUT" | grep -q " -> "; then
      pass "parser produced source -> destination format"
    else
      fail "parser did not produce expected arrow format"
      echo "Output was:"
      echo "$OUTPUT"
    fi
    echo ""

    # Test 2: parser optimized (-O)
    echo "--- Test 2: parser optimized ---"
    OUTPUT_OPT=$("$PARSER" -O "$PCAP" 2>&1) || {
      fail "parser -O exited with error"
      echo "$OUTPUT_OPT"
      exit 1
    }

    if echo "$OUTPUT_OPT" | grep -q "Packet"; then
      pass "parser -O produced Packet output"
    else
      fail "parser -O did not produce expected Packet output"
      echo "Output was:"
      echo "$OUTPUT_OPT"
    fi

    if echo "$OUTPUT_OPT" | grep -qE '[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+:[0-9]+'; then
      pass "parser -O produced IP:PORT format"
    else
      fail "parser -O did not produce expected IP:PORT format"
      echo "Output was:"
      echo "$OUTPUT_OPT"
    fi

    # Compare basic and optimized output - they should be identical
    if [[ "$OUTPUT" == "$OUTPUT_OPT" ]]; then
      pass "parser basic and optimized modes produce identical output"
    else
      fail "parser basic and optimized modes produce different output"
      echo "Basic output:"
      echo "$OUTPUT"
      echo "Optimized output:"
      echo "$OUTPUT_OPT"
    fi
    echo ""

    # Summary
    echo "==================================="
    echo "        TEST SUMMARY"
    echo "==================================="
    echo ""
    echo "Tests passed: $TESTS_PASSED"
    echo "Tests failed: $TESTS_FAILED"
    echo ""

    if [[ $TESTS_FAILED -eq 0 ]]; then
      echo "All ports_parser tests passed!"
      echo "==================================="
      exit 0
    else
      echo "Some tests failed!"
      echo "==================================="
      exit 1
    fi
  '';
}
