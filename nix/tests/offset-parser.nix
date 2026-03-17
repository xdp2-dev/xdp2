# nix/tests/offset-parser.nix
#
# Test for the offset_parser sample
#
# This test verifies that:
# 1. The offset_parser sample builds successfully using the installed xdp2
# 2. The parser binary runs and produces expected output (network/transport offsets)
# 3. The optimized parser (-O flag) also works correctly
#
# Supports two modes:
# - Native: Builds sample at runtime using xdp2-compiler
# - Pre-built: Uses pre-compiled binaries (for cross-compilation)
#
# Usage:
#   nix build .#tests.offset-parser
#   ./result/bin/xdp2-test-offset-parser
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
  name = "xdp2-test-offset-parser";

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

    echo "=== XDP2 offset_parser Test ==="
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
    cp -r ${testData}/samples/parser/offset_parser/* "$WORKDIR/"
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
    echo "--- Building offset_parser ---"
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

    # Test pcap file (IPv6 traffic for offset testing)
    PCAP="${testData}/data/pcaps/tcp_ipv6.pcap"

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

    if echo "$OUTPUT" | grep -q "Network offset:"; then
      pass "parser produced Network offset output"
    else
      fail "parser did not produce expected Network offset output"
      echo "Output was:"
      echo "$OUTPUT"
    fi

    if echo "$OUTPUT" | grep -q "Transport offset:"; then
      pass "parser produced Transport offset output"
    else
      fail "parser did not produce expected Transport offset output"
      echo "Output was:"
      echo "$OUTPUT"
    fi

    # Verify expected offset values for IPv6 (network=14, transport=54)
    if echo "$OUTPUT" | grep -q "Network offset: 14"; then
      pass "parser produced correct network offset (14) for IPv6"
    else
      fail "parser did not produce expected network offset of 14"
      echo "Output was:"
      echo "$OUTPUT"
    fi

    if echo "$OUTPUT" | grep -q "Transport offset: 54"; then
      pass "parser produced correct transport offset (54) for IPv6"
    else
      fail "parser did not produce expected transport offset of 54"
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

    if echo "$OUTPUT_OPT" | grep -q "Network offset:"; then
      pass "parser -O produced Network offset output"
    else
      fail "parser -O did not produce expected Network offset output"
      echo "Output was:"
      echo "$OUTPUT_OPT"
    fi

    if echo "$OUTPUT_OPT" | grep -q "Transport offset:"; then
      pass "parser -O produced Transport offset output"
    else
      fail "parser -O did not produce expected Transport offset output"
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
      echo "All offset_parser tests passed!"
      echo "==================================="
      exit 0
    else
      echo "Some tests failed!"
      echo "==================================="
      exit 1
    fi
  '';
}
