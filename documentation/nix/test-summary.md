# XDP2 Cross-Architecture Test Summary

## Overview

All parser and XDP sample tests are validated across three architectures:
x86_64 (native), RISC-V (cross-compiled, binfmt), and AArch64 (cross-compiled, binfmt).

## Test Results

| Test Suite           | x86_64   | RISC-V   | AArch64  |
|----------------------|----------|----------|----------|
| simple_parser        | 14/14 PASS | 14/14 PASS | 14/14 PASS |
| offset_parser        | 8/8 PASS   | 8/8 PASS   | 8/8 PASS   |
| ports_parser         | 8/8 PASS   | 8/8 PASS   | 8/8 PASS   |
| flow_tracker_combo   | 8/8 PASS   | 8/8 PASS   | 8/8 PASS   |
| xdp_build            | SKIPPED    | SKIPPED    | SKIPPED    |

**Total: 38/38 tests passing on all architectures.**

## Test Modes

- **x86_64**: Native compilation and execution via `nix run .#run-sample-tests`
- **RISC-V**: Cross-compiled with riscv64-unknown-linux-gnu GCC, executed via QEMU binfmt
- **AArch64**: Cross-compiled with aarch64-unknown-linux-gnu GCC, executed via QEMU binfmt

## Cross-Compilation Architecture

```
HOST (x86_64)                         TARGET (riscv64/aarch64)
┌─────────────────────┐               ┌──────────────────────┐
│ xdp2-compiler       │──generates──▶ │ .p.c source files    │
│ (runs natively)     │               │                      │
│                     │               │ cross-GCC compiles   │
│ nix build           │──produces───▶ │ target binaries      │
│ .#prebuilt-samples  │               │                      │
└─────────────────────┘               └──────────────────────┘
                                              │
                                     QEMU binfmt executes
                                     on x86_64 host
```

## xdp_build Status: SKIPPED

XDP BPF build tests are blocked pending architectural fixes:

1. **BPF stack limitations** — `XDP2_METADATA_TEMP_*` macros generate code exceeding BPF stack limit ("stack arguments are not supported")
2. **Template API mismatch** — `xdp_def.template.c` uses old `ctrl.hdr.*` API ("no member named hdr in struct xdp2_ctrl_data")

Affected samples: flow_tracker_simple, flow_tracker_tlvs, flow_tracker_tmpl.

## Running Tests

```bash
# Native x86_64
make test                  # All tests
make test-simple           # Individual suite

# Cross-compiled (requires binfmt)
make test-riscv64          # RISC-V via binfmt
make test-aarch64          # AArch64 via binfmt

# MicroVM (full system testing)
make test-riscv64-vm       # RISC-V in QEMU VM
make test-aarch64-vm       # AArch64 in QEMU VM
```
