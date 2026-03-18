# nix/tests/simple-parser.nix
#
# Test for the simple_parser sample
#
# This test verifies that:
# 1. The simple_parser sample builds successfully using the installed xdp2
# 2. The parser_notmpl binary runs and produces expected output
# 3. The optimized parser (-O flag) also works correctly
#
# Supports two modes:
# - Native: Builds sample at runtime using xdp2-compiler
# - Pre-built: Uses pre-compiled binaries (for cross-compilation)
#
# Usage:
#   nix build .#tests.simple-parser
#   ./result/bin/xdp2-test-simple-parser
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
  name = "xdp2-test-simple-parser";

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

    echo "=== XDP2 simple_parser Test ==="
    echo ""
    ${if usePrebuilt then ''echo "Mode: Pre-built samples"'' else ''echo "Mode: Runtime compilation"''}
    echo ""

    ${if usePrebuilt then ''
    # Pre-built mode: Use binaries from prebuiltSample
    PARSER_NOTMPL="${prebuiltSample}/bin/parser_notmpl"
    PARSER_TMPL="${prebuiltSample}/bin/parser_tmpl"

    echo "Using pre-built binaries:"
    echo "  parser_notmpl: $PARSER_NOTMPL"
    echo "  parser_tmpl: $PARSER_TMPL"
    echo ""

    # Verify binaries exist
    if [[ ! -x "$PARSER_NOTMPL" ]]; then
      echo "FAIL: parser_notmpl binary not found at $PARSER_NOTMPL"
      exit 1
    fi
    if [[ ! -x "$PARSER_TMPL" ]]; then
      echo "FAIL: parser_tmpl binary not found at $PARSER_TMPL"
      exit 1
    fi
    '' else ''
    # Runtime compilation mode: Build from source
    WORKDIR=$(mktemp -d)
    trap 'rm -rf "$WORKDIR"' EXIT

    echo "Work directory: $WORKDIR"
    echo ""

    # Copy sample sources to writable directory
    cp -r ${testData}/samples/parser/simple_parser/* "$WORKDIR/"
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
    echo "--- Building simple_parser ---"
    make XDP2DIR="${xdp2}" CFLAGS="-I${xdp2}/include -I${pkgs.libpcap}/include -g" LDFLAGS="-L${xdp2}/lib -L${pkgs.libpcap.lib}/lib -Wl,-rpath,${xdp2}/lib -Wl,-rpath,${pkgs.libpcap.lib}/lib"
    echo ""

    PARSER_NOTMPL="./parser_notmpl"
    PARSER_TMPL="./parser_tmpl"
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

    # Verify binaries were created
    if [[ ! -x "$PARSER_NOTMPL" ]]; then
      fail "parser_notmpl binary not found"
      exit 1
    fi
    pass "parser_notmpl binary created"

    if [[ ! -x "$PARSER_TMPL" ]]; then
      fail "parser_tmpl binary not found"
      exit 1
    fi
    pass "parser_tmpl binary created"
    echo ""

    # Test pcap file
    PCAP="${testData}/data/pcaps/tcp_ipv6.pcap"

    if [[ ! -f "$PCAP" ]]; then
      echo "FAIL: Test pcap file not found: $PCAP"
      exit 1
    fi

    # Test 1: parser_notmpl basic run
    echo "--- Test 1: parser_notmpl basic ---"
    OUTPUT=$("$PARSER_NOTMPL" "$PCAP" 2>&1) || {
      fail "parser_notmpl exited with error"
      echo "$OUTPUT"
      exit 1
    }

    if echo "$OUTPUT" | grep -q "IPv6:"; then
      pass "parser_notmpl produced IPv6 output"
    else
      fail "parser_notmpl did not produce expected IPv6 output"
      echo "Output was:"
      echo "$OUTPUT"
    fi

    if echo "$OUTPUT" | grep -q "TCP timestamps"; then
      pass "parser_notmpl parsed TCP timestamps"
    else
      fail "parser_notmpl did not parse TCP timestamps"
    fi

    if echo "$OUTPUT" | grep -q "Hash"; then
      pass "parser_notmpl computed hash values"
    else
      fail "parser_notmpl did not compute hash values"
    fi
    echo ""

    # Test 2: parser_notmpl optimized (-O)
    # The optimized parser should produce the same output as basic mode, including
    # proper IPv6 parsing and TCP timestamp extraction. This is critical for xdp2
    # performance - the optimized parser is the primary use case.
    echo "--- Test 2: parser_notmpl optimized ---"
    OUTPUT_OPT=$("$PARSER_NOTMPL" -O "$PCAP" 2>&1) || {
      fail "parser_notmpl -O exited with error"
      echo "$OUTPUT_OPT"
      exit 1
    }

    if echo "$OUTPUT_OPT" | grep -q "IPv6:"; then
      pass "parser_notmpl -O produced IPv6 output"
    else
      fail "parser_notmpl -O did not produce expected IPv6 output (proto_table extraction issue)"
      echo "Output was:"
      echo "$OUTPUT_OPT"
    fi

    if echo "$OUTPUT_OPT" | grep -q "TCP timestamps"; then
      pass "parser_notmpl -O parsed TCP timestamps"
    else
      fail "parser_notmpl -O did not parse TCP timestamps"
    fi

    if echo "$OUTPUT_OPT" | grep -q "Hash"; then
      pass "parser_notmpl -O computed hash values"
    else
      fail "parser_notmpl -O did not compute hash values"
    fi

    # Compare basic and optimized output - they should be identical
    if [[ "$OUTPUT" == "$OUTPUT_OPT" ]]; then
      pass "parser_notmpl basic and optimized modes produce identical output"
    else
      fail "parser_notmpl basic and optimized modes produce different output"
      echo "This may indicate proto_table extraction issues."
    fi
    echo ""

    # Test 3: parser_tmpl basic run
    echo "--- Test 3: parser_tmpl basic ---"
    OUTPUT_TMPL=$("$PARSER_TMPL" "$PCAP" 2>&1) || {
      fail "parser_tmpl exited with error"
      echo "$OUTPUT_TMPL"
      exit 1
    }

    if echo "$OUTPUT_TMPL" | grep -q "IPv6:"; then
      pass "parser_tmpl produced IPv6 output"
    else
      fail "parser_tmpl did not produce expected output"
      echo "Output was:"
      echo "$OUTPUT_TMPL"
    fi

    if echo "$OUTPUT_TMPL" | grep -q "Hash"; then
      pass "parser_tmpl computed hash values"
    else
      fail "parser_tmpl did not compute hash values"
    fi
    echo ""

    # Test 4: parser_tmpl optimized (-O)
    echo "--- Test 4: parser_tmpl optimized ---"
    OUTPUT_TMPL_OPT=$("$PARSER_TMPL" -O "$PCAP" 2>&1) || {
      fail "parser_tmpl -O exited with error"
      echo "$OUTPUT_TMPL_OPT"
      exit 1
    }

    if echo "$OUTPUT_TMPL_OPT" | grep -q "IPv6:"; then
      pass "parser_tmpl -O produced IPv6 output"
    else
      fail "parser_tmpl -O did not produce expected IPv6 output"
      echo "Output was:"
      echo "$OUTPUT_TMPL_OPT"
    fi

    if echo "$OUTPUT_TMPL_OPT" | grep -q "Hash"; then
      pass "parser_tmpl -O computed hash values"
    else
      fail "parser_tmpl -O did not compute hash values"
    fi

    # Compare basic and optimized output for parser_tmpl
    if [[ "$OUTPUT_TMPL" == "$OUTPUT_TMPL_OPT" ]]; then
      pass "parser_tmpl basic and optimized modes produce identical output"
    else
      fail "parser_tmpl basic and optimized modes produce different output"
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
      echo "All simple_parser tests passed!"
      echo "==================================="
      exit 0
    else
      echo "Some tests failed!"
      echo "==================================="
      exit 1
    fi
  '';
}
