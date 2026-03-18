# nix/tests/simple-parser-debug.nix
#
# Debug test for diagnosing optimized parser issues
#
# This test generates detailed output to help diagnose why the optimized
# parser may not be working correctly (e.g., missing proto_table extraction).
#
# Usage:
#   nix build .#tests.simple-parser-debug
#   ./result/bin/xdp2-test-simple-parser-debug
#
# Output is saved to ./debug-output/ directory (created in current working dir)
#
# Key diagnostics:
#   - Generated .p.c files (line counts should be ~676 for full functionality)
#   - Switch statement counts (should be 4 for protocol routing)
#   - Proto table extraction output from xdp2-compiler
#   - Actual parser output comparison (basic vs optimized)

{ pkgs, xdp2 }:

let
  # Source directory for test data (pcap files)
  testData = ../..;

  # LLVM config for getting correct clang paths
  llvmConfig = import ../llvm.nix { inherit pkgs; lib = pkgs.lib; };
in
pkgs.writeShellApplication {
  name = "xdp2-test-simple-parser-debug";

  runtimeInputs = [
    pkgs.gnumake
    pkgs.gcc
    pkgs.coreutils
    pkgs.gnugrep
    pkgs.gawk
    pkgs.libpcap
    pkgs.libpcap.lib
    pkgs.linuxHeaders
  ];

  text = ''
    set -euo pipefail

    echo "=== XDP2 simple_parser Debug Test ==="
    echo ""

    # Create debug output directory in current working directory (resolve to absolute path)
    DEBUG_DIR="$(pwd)/debug-output"
    rm -rf "$DEBUG_DIR"
    mkdir -p "$DEBUG_DIR"

    echo "Debug output directory: $DEBUG_DIR"
    echo ""

    # Create temp build directory
    WORKDIR=$(mktemp -d)
    trap 'rm -rf "$WORKDIR"' EXIT

    # Copy sample sources to writable directory
    cp -r ${testData}/samples/parser/simple_parser/* "$WORKDIR/"
    cd "$WORKDIR"

    # Set up environment
    export XDP2DIR="${xdp2}"
    export LD_LIBRARY_PATH="${xdp2}/lib:${pkgs.libpcap.lib}/lib''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export PATH="${xdp2}/bin:$PATH"

    # Include paths for xdp2-compiler's libclang usage
    export XDP2_C_INCLUDE_PATH="${llvmConfig.paths.clangResourceDir}/include"
    export XDP2_GLIBC_INCLUDE_PATH="${pkgs.stdenv.cc.libc.dev}/include"
    export XDP2_LINUX_HEADERS_PATH="${pkgs.linuxHeaders}/include"

    # Log environment
    echo "=== Environment ===" | tee "$DEBUG_DIR/environment.txt"
    echo "XDP2DIR: $XDP2DIR" | tee -a "$DEBUG_DIR/environment.txt"
    echo "XDP2_C_INCLUDE_PATH: $XDP2_C_INCLUDE_PATH" | tee -a "$DEBUG_DIR/environment.txt"
    echo "XDP2_GLIBC_INCLUDE_PATH: $XDP2_GLIBC_INCLUDE_PATH" | tee -a "$DEBUG_DIR/environment.txt"
    echo "XDP2_LINUX_HEADERS_PATH: $XDP2_LINUX_HEADERS_PATH" | tee -a "$DEBUG_DIR/environment.txt"
    echo "" | tee -a "$DEBUG_DIR/environment.txt"

    # Build parser_notmpl with verbose compiler output
    echo "=== Running xdp2-compiler (verbose) ===" | tee "$DEBUG_DIR/compiler-verbose.txt"

    xdp2-compiler --verbose \
      -I"$XDP2DIR/include" \
      -i parser_notmpl.c \
      -o parser_notmpl.p.c \
      2>&1 | tee -a "$DEBUG_DIR/compiler-verbose.txt" || true

    echo "" | tee -a "$DEBUG_DIR/compiler-verbose.txt"

    # Check if output file was created
    echo "=== Generated File Analysis ===" | tee "$DEBUG_DIR/analysis.txt"

    if [[ -f parser_notmpl.p.c ]]; then
      echo "parser_notmpl.p.c created successfully" | tee -a "$DEBUG_DIR/analysis.txt"

      # Copy generated file
      cp parser_notmpl.p.c "$DEBUG_DIR/"

      # Line count (should be ~676 for full functionality, ~601 if proto_tables missing)
      LINES=$(wc -l < parser_notmpl.p.c)
      echo "Line count: $LINES" | tee -a "$DEBUG_DIR/analysis.txt"

      if [[ $LINES -lt 650 ]]; then
        echo "WARNING: Line count is low - proto_table extraction may be broken" | tee -a "$DEBUG_DIR/analysis.txt"
      fi

      # Count switch statements (should be 4 for protocol routing)
      SWITCHES=$(grep -c "switch (type)" parser_notmpl.p.c || echo "0")
      echo "Switch statements for protocol routing: $SWITCHES" | tee -a "$DEBUG_DIR/analysis.txt"

      if [[ $SWITCHES -lt 3 ]]; then
        echo "WARNING: Missing switch statements - protocol routing will not work" | tee -a "$DEBUG_DIR/analysis.txt"
        echo "This causes 'Unknown addr type 0' output in optimized mode" | tee -a "$DEBUG_DIR/analysis.txt"
      fi

      # Extract switch statement context
      echo "" | tee -a "$DEBUG_DIR/analysis.txt"
      echo "=== Switch Statement Locations ===" | tee -a "$DEBUG_DIR/analysis.txt"
      grep -n "switch (type)" parser_notmpl.p.c | tee -a "$DEBUG_DIR/analysis.txt" || echo "No switch statements found" | tee -a "$DEBUG_DIR/analysis.txt"

    else
      echo "ERROR: parser_notmpl.p.c was NOT created" | tee -a "$DEBUG_DIR/analysis.txt"
      echo "This indicates xdp2-compiler failed to find parser roots" | tee -a "$DEBUG_DIR/analysis.txt"
    fi

    echo "" | tee -a "$DEBUG_DIR/analysis.txt"

    # Extract key verbose output sections
    echo "=== Proto Table Extraction ===" | tee "$DEBUG_DIR/proto-tables.txt"
    grep -E "COLECTED DATA FROM PROTO TABLE|Analyzing table|entries:|proto-tables" "$DEBUG_DIR/compiler-verbose.txt" 2>/dev/null | tee -a "$DEBUG_DIR/proto-tables.txt" || echo "No proto table output found" | tee -a "$DEBUG_DIR/proto-tables.txt"

    echo "" | tee -a "$DEBUG_DIR/proto-tables.txt"
    echo "=== Graph Building ===" | tee "$DEBUG_DIR/graph.txt"
    grep -E "GRAPH SIZE|FINAL|insert_node_by_name|Skipping node|No roots" "$DEBUG_DIR/compiler-verbose.txt" 2>/dev/null | tee -a "$DEBUG_DIR/graph.txt" || echo "No graph output found" | tee -a "$DEBUG_DIR/graph.txt"

    # Build and test the parsers if .p.c was created
    if [[ -f parser_notmpl.p.c ]]; then
      echo ""
      echo "=== Building Parser Binary ===" | tee "$DEBUG_DIR/build.txt"

      gcc -I"$XDP2DIR/include" -I"${pkgs.libpcap}/include" -g \
        -L"$XDP2DIR/lib" -L"${pkgs.libpcap.lib}/lib" \
        -Wl,-rpath,"${pkgs.libpcap.lib}/lib" \
        -o parser_notmpl parser_notmpl.p.c \
        -lpcap -lxdp2 -lcli -lsiphash 2>&1 | tee -a "$DEBUG_DIR/build.txt"

      if [[ -x ./parser_notmpl ]]; then
        echo "Build successful" | tee -a "$DEBUG_DIR/build.txt"

        PCAP="${testData}/data/pcaps/tcp_ipv6.pcap"

        echo ""
        echo "=== Parser Output: Basic Mode ===" | tee "$DEBUG_DIR/parser-basic.txt"
        ./parser_notmpl "$PCAP" 2>&1 | tee -a "$DEBUG_DIR/parser-basic.txt" || true

        echo ""
        echo "=== Parser Output: Optimized Mode (-O) ===" | tee "$DEBUG_DIR/parser-optimized.txt"
        ./parser_notmpl -O "$PCAP" 2>&1 | tee -a "$DEBUG_DIR/parser-optimized.txt" || true

        echo ""
        echo "=== Comparison ===" | tee "$DEBUG_DIR/comparison.txt"

        BASIC_IPV6=$(grep -c "IPv6:" "$DEBUG_DIR/parser-basic.txt" || echo "0")
        OPT_IPV6=$(grep -c "IPv6:" "$DEBUG_DIR/parser-optimized.txt" || echo "0")
        OPT_UNKNOWN=$(grep -c "Unknown addr type" "$DEBUG_DIR/parser-optimized.txt" || echo "0")

        echo "Basic mode IPv6 lines: $BASIC_IPV6" | tee -a "$DEBUG_DIR/comparison.txt"
        echo "Optimized mode IPv6 lines: $OPT_IPV6" | tee -a "$DEBUG_DIR/comparison.txt"
        echo "Optimized mode 'Unknown addr type' lines: $OPT_UNKNOWN" | tee -a "$DEBUG_DIR/comparison.txt"

        echo "" | tee -a "$DEBUG_DIR/comparison.txt"

        if [[ $OPT_IPV6 -gt 0 && $OPT_UNKNOWN -eq 0 ]]; then
          echo "RESULT: Optimized parser is working correctly!" | tee -a "$DEBUG_DIR/comparison.txt"
        else
          echo "RESULT: Optimized parser is NOT working correctly" | tee -a "$DEBUG_DIR/comparison.txt"
          echo "  - Proto table extraction appears to be broken" | tee -a "$DEBUG_DIR/comparison.txt"
          echo "  - Check proto-tables.txt for extraction output" | tee -a "$DEBUG_DIR/comparison.txt"
          echo "  - Check analysis.txt for switch statement count" | tee -a "$DEBUG_DIR/comparison.txt"
        fi
      else
        echo "Build failed" | tee -a "$DEBUG_DIR/build.txt"
      fi
    fi

    echo ""
    echo "==================================="
    echo "Debug output saved to: $DEBUG_DIR"
    echo ""
    echo "Key files:"
    echo "  - analysis.txt        : Line counts, switch statement analysis"
    echo "  - parser_notmpl.p.c   : Generated parser code"
    echo "  - compiler-verbose.txt: Full xdp2-compiler output"
    echo "  - proto-tables.txt    : Proto table extraction output"
    echo "  - graph.txt           : Graph building output"
    echo "  - comparison.txt      : Basic vs optimized parser comparison"
    echo "==================================="
  '';
}
