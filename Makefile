# Makefile for XDP2
#
# This Makefile provides convenient targets for building and testing XDP2
# using Nix. All builds are performed via Nix flakes.
#
# Usage:
#   make help           - Show all available targets
#   make build          - Build xdp2 (production)
#   make test           - Run all x86_64 tests
#   make test-riscv64   - Run RISC-V tests via binfmt
#
# Output directories:
#   result/             - Default xdp2 build
#   result-debug/       - Debug build with assertions
#   result-samples/     - Pre-built samples (native)
#   result-riscv64/     - RISC-V cross-compiled xdp2
#   result-riscv64-samples/ - RISC-V pre-built samples
#   result-aarch64/     - AArch64 cross-compiled xdp2
#   result-aarch64-samples/ - AArch64 pre-built samples
#

.PHONY: help build build-debug build-all clean
.PHONY: test test-all test-simple test-offset test-ports test-flow
.PHONY: samples samples-riscv64 samples-aarch64
.PHONY: riscv64 riscv64-debug riscv64-samples riscv64-tests test-riscv64 test-riscv64-vm
.PHONY: aarch64 aarch64-debug aarch64-samples aarch64-tests test-aarch64 test-aarch64-vm
.PHONY: vm-x86 vm-aarch64 vm-riscv64 vm-test-all
.PHONY: deb deb-x86
.PHONY: analysis analysis-quick analysis-standard analysis-deep
.PHONY: dev shell check eval

# Default target
.DEFAULT_GOAL := help

# =============================================================================
# Help
# =============================================================================

help:
	@echo "XDP2 Build System (Nix-based)"
	@echo ""
	@echo "=== Quick Start ==="
	@echo "  make build          Build xdp2 (production)"
	@echo "  make test           Run all x86_64 tests"
	@echo "  make dev            Enter development shell"
	@echo ""
	@echo "=== Native Builds (x86_64) ==="
	@echo "  make build          Build xdp2 production -> result/"
	@echo "  make build-debug    Build xdp2 with assertions -> result-debug/"
	@echo "  make samples        Build pre-built samples -> result-samples/"
	@echo ""
	@echo "=== Native Tests ==="
	@echo "  make test           Run all sample tests"
	@echo "  make test-simple    Run simple_parser tests only"
	@echo "  make test-offset    Run offset_parser tests only"
	@echo "  make test-ports     Run ports_parser tests only"
	@echo "  make test-flow      Run flow_tracker_combo tests only"
	@echo ""
	@echo "=== RISC-V Cross-Compilation ==="
	@echo "  make riscv64        Build xdp2 for RISC-V -> result-riscv64/"
	@echo "  make riscv64-debug  Build debug xdp2 for RISC-V -> result-riscv64-debug/"
	@echo "  make riscv64-samples Build pre-built samples -> result-riscv64-samples/"
	@echo "  make test-riscv64   Run RISC-V tests via binfmt (requires binfmt enabled)"
	@echo "  make test-riscv64-vm Run RISC-V tests in MicroVM"
	@echo ""
	@echo "=== AArch64 Cross-Compilation ==="
	@echo "  make aarch64        Build xdp2 for AArch64 -> result-aarch64/"
	@echo "  make aarch64-debug  Build debug xdp2 for AArch64 -> result-aarch64-debug/"
	@echo "  make aarch64-samples Build pre-built samples -> result-aarch64-samples/"
	@echo "  make test-aarch64   Run AArch64 tests via binfmt (requires binfmt enabled)"
	@echo "  make test-aarch64-vm Run AArch64 tests in MicroVM"
	@echo ""
	@echo "=== MicroVM Testing ==="
	@echo "  make vm-x86         Build x86_64 MicroVM -> result-vm-x86/"
	@echo "  make vm-aarch64     Build AArch64 MicroVM -> result-vm-aarch64/"
	@echo "  make vm-riscv64     Build RISC-V MicroVM -> result-vm-riscv64/"
	@echo "  make vm-test-all    Run full VM lifecycle tests (all architectures)"
	@echo ""
	@echo "=== Static Analysis ==="
	@echo "  make analysis       Run quick static analysis (alias for analysis-quick)"
	@echo "  make analysis-quick Run clang-tidy + cppcheck"
	@echo "  make analysis-standard  Run + flawfinder, clang-analyzer, gcc-warnings"
	@echo "  make analysis-deep  Run all 8 tools including gcc-analyzer, semgrep, sanitizers"
	@echo ""
	@echo "=== Packaging ==="
	@echo "  make deb            Build Debian package -> result-deb/"
	@echo ""
	@echo "=== Development ==="
	@echo "  make dev            Enter nix development shell"
	@echo "  make shell          Alias for 'make dev'"
	@echo "  make check          Verify nix flake"
	@echo "  make eval           Evaluate all flake outputs (syntax check)"
	@echo ""
	@echo "=== Cleanup ==="
	@echo "  make clean          Remove all result-* symlinks"
	@echo "  make gc             Run nix garbage collection"
	@echo ""
	@echo "=== Prerequisites ==="
	@echo "  - Nix with flakes enabled"
	@echo "  - For RISC-V/AArch64 binfmt: boot.binfmt.emulatedSystems in NixOS config"
	@echo ""

# =============================================================================
# Native Builds (x86_64)
# =============================================================================

# Build production xdp2
build:
	@echo "Building xdp2 (production)..."
	nix build .#xdp2 -o result

# Build debug xdp2 with assertions enabled
build-debug:
	@echo "Building xdp2 (debug with assertions)..."
	nix build .#xdp2-debug -o result-debug

# Build both production and debug
build-all: build build-debug

# Build pre-built samples (native x86_64)
samples:
	@echo "Building native x86_64 samples..."
	@echo "Note: Native samples are built at test runtime, this target is for reference"
	nix build .#xdp-samples -o result-samples

# =============================================================================
# Native Tests (x86_64)
# =============================================================================

# Run all tests
test:
	@echo "Running all x86_64 sample tests..."
	nix run .#run-sample-tests

# Alias for test
test-all: test

# Run individual test suites
test-simple:
	@echo "Running simple_parser tests..."
	nix run .#tests.simple-parser

test-offset:
	@echo "Running offset_parser tests..."
	nix run .#tests.offset-parser

test-ports:
	@echo "Running ports_parser tests..."
	nix run .#tests.ports-parser

test-flow:
	@echo "Running flow_tracker_combo tests..."
	nix run .#tests.flow-tracker-combo

# =============================================================================
# RISC-V Cross-Compilation
# =============================================================================

# Build xdp2 for RISC-V (production)
riscv64:
	@echo "Building xdp2 for RISC-V (production)..."
	nix build .#xdp2-debug-riscv64 -o result-riscv64

# Build xdp2 for RISC-V (debug) - same as above, debug is default for cross
riscv64-debug: riscv64

# Build pre-built samples for RISC-V
# These are compiled on x86_64 host using xdp2-compiler, then cross-compiled to RISC-V
riscv64-samples:
	@echo "Building pre-built samples for RISC-V..."
	@echo "  - xdp2-compiler runs on x86_64 host"
	@echo "  - Sample binaries are cross-compiled to RISC-V"
	nix build .#prebuilt-samples-riscv64 -o result-riscv64-samples

# Build RISC-V test derivations
riscv64-tests:
	@echo "Building RISC-V test derivations..."
	nix build .#riscv64-tests.all -o result-riscv64-tests

# Run RISC-V tests via binfmt emulation (requires binfmt enabled)
# This runs RISC-V binaries directly on x86_64 via QEMU user-mode
test-riscv64: riscv64-samples
	@echo "Running RISC-V tests via binfmt emulation..."
	@echo "Prerequisites: boot.binfmt.emulatedSystems = [ \"riscv64-linux\" ];"
	@echo ""
	@if [ ! -f /proc/sys/fs/binfmt_misc/riscv64-linux ]; then \
		echo "ERROR: RISC-V binfmt not registered!"; \
		echo "Run: sudo systemctl restart systemd-binfmt.service"; \
		exit 1; \
	fi
	@echo "Testing simple_parser (parser_notmpl)..."
	./result-riscv64-samples/bin/parser_notmpl data/pcaps/tcp_ipv6.pcap
	@echo ""
	@echo "Testing simple_parser optimized (-O)..."
	./result-riscv64-samples/bin/parser_notmpl -O data/pcaps/tcp_ipv6.pcap
	@echo ""
	@echo "Testing offset_parser..."
	./result-riscv64-samples/bin/parser data/pcaps/tcp_ipv6.pcap
	@echo ""
	@echo "RISC-V binfmt tests completed!"

# Run RISC-V tests inside MicroVM
test-riscv64-vm:
	@echo "Running RISC-V tests in MicroVM..."
	nix run .#run-riscv64-tests

# =============================================================================
# AArch64 Cross-Compilation
# =============================================================================

# Build xdp2 for AArch64
aarch64:
	@echo "Building xdp2 for AArch64..."
	nix build .#xdp2-debug-aarch64 -o result-aarch64

# Build xdp2 for AArch64 (debug) - same as above, debug is default for cross
aarch64-debug: aarch64

# Build pre-built samples for AArch64
aarch64-samples:
	@echo "Building pre-built samples for AArch64..."
	@echo "  - xdp2-compiler runs on x86_64 host"
	@echo "  - Sample binaries are cross-compiled to AArch64"
	nix build .#prebuilt-samples-aarch64 -o result-aarch64-samples

# Build AArch64 test derivations
aarch64-tests:
	@echo "Building AArch64 test derivations..."
	nix build .#aarch64-tests.all -o result-aarch64-tests

# Run AArch64 tests via binfmt emulation (requires binfmt enabled)
test-aarch64: aarch64-samples
	@echo "Running AArch64 tests via binfmt emulation..."
	@echo "Prerequisites: boot.binfmt.emulatedSystems = [ \"aarch64-linux\" ];"
	@echo ""
	@if [ ! -f /proc/sys/fs/binfmt_misc/aarch64-linux ]; then \
		echo "ERROR: AArch64 binfmt not registered!"; \
		echo "Run: sudo systemctl restart systemd-binfmt.service"; \
		exit 1; \
	fi
	@echo "Testing simple_parser (parser_notmpl)..."
	./result-aarch64-samples/bin/parser_notmpl data/pcaps/tcp_ipv6.pcap
	@echo ""
	@echo "Testing simple_parser optimized (-O)..."
	./result-aarch64-samples/bin/parser_notmpl -O data/pcaps/tcp_ipv6.pcap
	@echo ""
	@echo "Testing offset_parser..."
	./result-aarch64-samples/bin/parser data/pcaps/tcp_ipv6.pcap
	@echo ""
	@echo "AArch64 binfmt tests completed!"

# Run AArch64 tests inside MicroVM
test-aarch64-vm:
	@echo "Running AArch64 tests in MicroVM..."
	nix run .#run-aarch64-tests

# =============================================================================
# MicroVM Testing
# =============================================================================

# Build MicroVMs for each architecture
vm-x86:
	@echo "Building x86_64 MicroVM..."
	nix build .#microvm-x86_64 -o result-vm-x86

vm-aarch64:
	@echo "Building AArch64 MicroVM..."
	nix build .#microvm-aarch64 -o result-vm-aarch64

vm-riscv64:
	@echo "Building RISC-V MicroVM..."
	nix build .#microvm-riscv64 -o result-vm-riscv64

# Run full VM lifecycle tests for all architectures
vm-test-all:
	@echo "Running full VM lifecycle tests (all architectures)..."
	nix run .#microvms.test-all

# =============================================================================
# Packaging
# =============================================================================

# Build Debian package for x86_64
deb:
	@echo "Building Debian package..."
	nix build .#deb-x86_64 -o result-deb

deb-x86: deb

# =============================================================================
# Static Analysis
# =============================================================================

# Quick analysis: clang-tidy + cppcheck
analysis: analysis-quick

analysis-quick:
	@echo "Running quick static analysis (clang-tidy + cppcheck)..."
	nix build .#analysis-quick -o result-analysis-quick
	@echo ""
	@cat result-analysis-quick/summary.txt

# Standard analysis: + flawfinder, clang-analyzer, gcc-warnings
analysis-standard:
	@echo "Running standard static analysis..."
	nix build .#analysis-standard -o result-analysis-standard
	@echo ""
	@cat result-analysis-standard/summary.txt

# Deep analysis: all 8 tools
analysis-deep:
	@echo "Running deep static analysis (all tools)..."
	nix build .#analysis-deep -o result-analysis-deep
	@echo ""
	@cat result-analysis-deep/summary.txt

# =============================================================================
# Development
# =============================================================================

# Enter development shell
dev:
	@echo "Entering development shell..."
	nix develop

shell: dev

# Verify flake
check:
	@echo "Checking nix flake..."
	nix flake check

# Evaluate all outputs (quick syntax/reference check)
eval:
	@echo "Evaluating flake outputs..."
	@echo "Native packages:"
	nix eval .#xdp2 --apply 'x: x.name' 2>/dev/null && echo "  xdp2: OK" || echo "  xdp2: FAIL"
	nix eval .#xdp2-debug --apply 'x: x.name' 2>/dev/null && echo "  xdp2-debug: OK" || echo "  xdp2-debug: FAIL"
	@echo "Tests:"
	nix eval .#tests.simple-parser --apply 'x: x.name' 2>/dev/null && echo "  tests.simple-parser: OK" || echo "  tests.simple-parser: FAIL"
	nix eval .#tests.all --apply 'x: x.name' 2>/dev/null && echo "  tests.all: OK" || echo "  tests.all: FAIL"
	@echo "RISC-V:"
	nix eval .#xdp2-debug-riscv64 --apply 'x: x.name' 2>/dev/null && echo "  xdp2-debug-riscv64: OK" || echo "  xdp2-debug-riscv64: FAIL"
	nix eval .#prebuilt-samples-riscv64 --apply 'x: x.name' 2>/dev/null && echo "  prebuilt-samples-riscv64: OK" || echo "  prebuilt-samples-riscv64: FAIL"
	nix eval .#riscv64-tests.all --apply 'x: x.name' 2>/dev/null && echo "  riscv64-tests.all: OK" || echo "  riscv64-tests.all: FAIL"
	@echo "AArch64:"
	nix eval .#xdp2-debug-aarch64 --apply 'x: x.name' 2>/dev/null && echo "  xdp2-debug-aarch64: OK" || echo "  xdp2-debug-aarch64: FAIL"
	nix eval .#prebuilt-samples-aarch64 --apply 'x: x.name' 2>/dev/null && echo "  prebuilt-samples-aarch64: OK" || echo "  prebuilt-samples-aarch64: FAIL"
	nix eval .#aarch64-tests.all --apply 'x: x.name' 2>/dev/null && echo "  aarch64-tests.all: OK" || echo "  aarch64-tests.all: FAIL"
	@echo "Analysis:"
	nix eval .#analysis-quick --apply 'x: x.name' 2>/dev/null && echo "  analysis-quick: OK" || echo "  analysis-quick: FAIL"
	nix eval .#analysis-standard --apply 'x: x.name' 2>/dev/null && echo "  analysis-standard: OK" || echo "  analysis-standard: FAIL"
	nix eval .#analysis-deep --apply 'x: x.name' 2>/dev/null && echo "  analysis-deep: OK" || echo "  analysis-deep: FAIL"
	@echo ""
	@echo "All evaluations completed."

# =============================================================================
# Cleanup
# =============================================================================

# Remove all result symlinks
clean:
	@echo "Removing result symlinks..."
	rm -f result result-*
	@echo "Done. Run 'make gc' to garbage collect nix store."

# Nix garbage collection
gc:
	@echo "Running nix garbage collection..."
	nix-collect-garbage -d
