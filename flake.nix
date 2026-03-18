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
# To run the sample test:
# nix build .#tests.simple-parser
# ./result/bin/xdp2-test-simple-parser
#
{
  description = "XDP2 packet processing framework";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    # MicroVM for eBPF testing (Phase 1)
    # See: documentation/nix/microvm-implementation-phase1.md
    microvm = {
      url = "github:astro/microvm.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, flake-utils, microvm }:
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

        # XDP sample programs (BPF bytecode)
        # Uses xdp2-debug for xdp2-compiler and headers
        xdp-samples = import ./nix/xdp-samples.nix {
          inherit pkgs;
          xdp2 = xdp2-debug;
        };

        # Import development shell module
        devshell = import ./nix/devshell.nix {
          inherit pkgs lib llvmConfig compilerConfig envVars;
          packages = packagesModule;
        };

        # Import tests module (uses debug build for assertion support)
        tests = import ./nix/tests {
          inherit pkgs;
          xdp2 = xdp2-debug;  # Tests use debug build with assertions
        };

        # =====================================================================
        # Phase 1: Packaging (x86_64 .deb only)
        # See: documentation/nix/microvm-implementation-phase1.md
        # =====================================================================
        packaging = import ./nix/packaging {
          inherit pkgs lib;
          xdp2 = xdp2;  # Use production build for distribution
        };

        # =====================================================================
        # Phase 2: MicroVM infrastructure (x86_64, aarch64, riscv64)
        # See: documentation/nix/microvm-phase2-arm-riscv-plan.md
        #
        # Cross-compilation: We pass buildSystem so that when building for
        # non-native architectures (e.g., riscv64 on x86_64), we use true
        # cross-compilation with native cross-compilers instead of slow
        # binfmt emulation.
        # =====================================================================
        microvms = import ./nix/microvms {
          inherit pkgs lib microvm nixpkgs;
          buildSystem = system;  # Pass host system for cross-compilation
        };

        # Convenience target to run all sample tests
        run-sample-tests = pkgs.writeShellApplication {
          name = "run-sample-tests";
          runtimeInputs = [];
          text = ''
            echo "========================================"
            echo "  XDP2 Sample Tests Runner"
            echo "========================================"
            echo ""

            # Run all tests via the combined test runner
            ${tests.all}/bin/xdp2-test-all
          '';
        };

      in
      {
        # Package outputs
        packages = {
          default = xdp2;
          xdp2 = xdp2;
          xdp2-debug = xdp2-debug;  # Debug build with assertions
          xdp-samples = xdp-samples;  # XDP sample programs (BPF bytecode)

          # Tests (build with: nix build .#tests.simple-parser)
          tests = tests;

          # Convenience aliases for individual tests
          simple-parser-test = tests.simple-parser;
          offset-parser-test = tests.offset-parser;
          ports-parser-test = tests.ports-parser;
          flow-tracker-combo-test = tests.flow-tracker-combo;
          xdp-build-test = tests.xdp-build;

          # Run all sample tests in one go
          # Usage: nix run .#run-sample-tests
          inherit run-sample-tests;

          # ===================================================================
          # Phase 1: Packaging outputs (x86_64 .deb only)
          # See: documentation/nix/microvm-implementation-phase1.md
          # ===================================================================

          # Staging directory (for inspection/debugging)
          # Usage: nix build .#deb-staging
          deb-staging = packaging.staging.x86_64;

          # Debian package
          # Usage: nix build .#deb-x86_64
          deb-x86_64 = packaging.deb.x86_64;

          # ===================================================================
          # Phase 2: MicroVM outputs (x86_64, aarch64, riscv64)
          # See: documentation/nix/microvm-phase2-arm-riscv-plan.md
          # ===================================================================
          #
          # Primary interface (nested):
          #   nix build .#microvms.x86_64
          #   nix run .#microvms.test-x86_64
          #   nix run .#microvms.test-all
          #
          # Legacy interface (flat, backwards compatible):
          #   nix build .#microvm-x86_64
          #   nix run .#xdp2-lifecycle-full-test
          #

          # ─────────────────────────────────────────────────────────────────
          # Nested MicroVM structure (primary interface)
          # ─────────────────────────────────────────────────────────────────
          microvms = {
            # VM derivations
            x86_64 = microvms.vms.x86_64;
            aarch64 = microvms.vms.aarch64;
            riscv64 = microvms.vms.riscv64;

            # Individual architecture tests
            test-x86_64 = microvms.tests.x86_64;
            test-aarch64 = microvms.tests.aarch64;
            test-riscv64 = microvms.tests.riscv64;

            # Combined test (all architectures)
            test-all = microvms.tests.all;

            # Lifecycle scripts (nested by arch)
            lifecycle = microvms.lifecycleByArch;

            # Helper scripts (nested by arch)
            helpers = microvms.helpers;

            # Expect scripts (nested by arch)
            expect = microvms.expect;
          };

          # ─────────────────────────────────────────────────────────────────
          # Legacy flat exports (backwards compatibility)
          # ─────────────────────────────────────────────────────────────────

          # VM derivations (legacy names)
          microvm-x86_64 = microvms.vms.x86_64;
          microvm-aarch64 = microvms.vms.aarch64;
          microvm-riscv64 = microvms.vms.riscv64;

          # Test runner (legacy name)
          xdp2-test-phase1 = microvms.testRunner;

          # Helper scripts (legacy names, x86_64 default)
          xdp2-vm-console = microvms.connectConsole;
          xdp2-vm-serial = microvms.connectSerial;
          xdp2-vm-status = microvms.vmStatus;

          # Login helpers
          xdp2-vm-login-serial = microvms.loginSerial;
          xdp2-vm-login-virtio = microvms.loginVirtio;

          # Command execution helpers
          xdp2-vm-run-serial = microvms.runCommandSerial;
          xdp2-vm-run-virtio = microvms.runCommandVirtio;

          # Expect-based helpers
          xdp2-vm-expect-run = microvms.expectRunCommand;
          xdp2-vm-debug-expect = microvms.debugVmExpect;
          xdp2-vm-expect-verify-service = microvms.expectVerifyService;

          # Lifecycle scripts (legacy names, x86_64 default)
          xdp2-lifecycle-0-build = microvms.lifecycle.checkBuild;
          xdp2-lifecycle-1-check-process = microvms.lifecycle.checkProcess;
          xdp2-lifecycle-2-check-serial = microvms.lifecycle.checkSerial;
          xdp2-lifecycle-2b-check-virtio = microvms.lifecycle.checkVirtio;
          xdp2-lifecycle-3-verify-ebpf-loaded = microvms.lifecycle.verifyEbpfLoaded;
          xdp2-lifecycle-4-verify-ebpf-running = microvms.lifecycle.verifyEbpfRunning;
          xdp2-lifecycle-5-shutdown = microvms.lifecycle.shutdown;
          xdp2-lifecycle-6-wait-exit = microvms.lifecycle.waitExit;
          xdp2-lifecycle-force-kill = microvms.lifecycle.forceKill;
          xdp2-lifecycle-full-test = microvms.lifecycle.fullTest;
        } // (
          # ===================================================================
          # Cross-compiled packages for RISC-V (built on x86_64, runs on riscv64)
          # ===================================================================
          if system == "x86_64-linux" then
            let
              pkgsCrossRiscv = import nixpkgs {
                localSystem = "x86_64-linux";
                crossSystem = "riscv64-linux";
                config = { allowUnfree = true; };
                overlays = [
                  (final: prev: {
                    boehmgc = prev.boehmgc.overrideAttrs (old: { doCheck = false; });
                    libuv = prev.libuv.overrideAttrs (old: { doCheck = false; });
                    meson = prev.meson.overrideAttrs (old: { doCheck = false; doInstallCheck = false; });
                    libseccomp = prev.libseccomp.overrideAttrs (old: { doCheck = false; });
                  })
                ];
              };

              # For cross-compilation, use HOST LLVM for xdp2-compiler (runs on build machine)
              # Use target packages for the actual xdp2 libraries
              packagesModuleRiscv = import ./nix/packages.nix { pkgs = pkgsCrossRiscv; llvmPackages = llvmConfig.llvmPackages; };

              xdp2-debug-riscv64 = import ./nix/derivation.nix {
                pkgs = pkgsCrossRiscv;
                lib = pkgsCrossRiscv.lib;
                # Use HOST llvmConfig, not target, because xdp2-compiler runs on HOST
                llvmConfig = llvmConfig;
                inherit (packagesModuleRiscv) nativeBuildInputs buildInputs;
                enableAsserts = true;
              };

              # Pre-built samples for RISC-V cross-compilation
              # Key: xdp2-compiler runs on HOST (x86_64), generates .p.c files
              # which are then compiled with TARGET (RISC-V) toolchain
              prebuiltSamplesRiscv64 = import ./nix/samples {
                inherit pkgs;                    # Host pkgs (for xdp2-compiler)
                xdp2 = xdp2-debug;               # Host xdp2 with compiler (x86_64)
                xdp2Target = xdp2-debug-riscv64; # Target xdp2 libraries (RISC-V)
                targetPkgs = pkgsCrossRiscv;     # Target pkgs for binaries
              };

              testsRiscv64 = import ./nix/tests {
                pkgs = pkgsCrossRiscv;
                xdp2 = xdp2-debug-riscv64;
                prebuiltSamples = prebuiltSamplesRiscv64;
              };

              # ─── AArch64 cross-compilation (same pattern as RISC-V) ───
              pkgsCrossAarch64 = import nixpkgs {
                localSystem = "x86_64-linux";
                crossSystem = "aarch64-linux";
                config = { allowUnfree = true; };
                overlays = [
                  (final: prev: {
                    boehmgc = prev.boehmgc.overrideAttrs (old: { doCheck = false; });
                    libuv = prev.libuv.overrideAttrs (old: { doCheck = false; });
                    meson = prev.meson.overrideAttrs (old: { doCheck = false; doInstallCheck = false; });
                    libseccomp = prev.libseccomp.overrideAttrs (old: { doCheck = false; });
                  })
                ];
              };

              packagesModuleAarch64 = import ./nix/packages.nix { pkgs = pkgsCrossAarch64; llvmPackages = llvmConfig.llvmPackages; };

              xdp2-debug-aarch64 = import ./nix/derivation.nix {
                pkgs = pkgsCrossAarch64;
                lib = pkgsCrossAarch64.lib;
                llvmConfig = llvmConfig;
                inherit (packagesModuleAarch64) nativeBuildInputs buildInputs;
                enableAsserts = true;
              };

              prebuiltSamplesAarch64 = import ./nix/samples {
                inherit pkgs;
                xdp2 = xdp2-debug;
                xdp2Target = xdp2-debug-aarch64;
                targetPkgs = pkgsCrossAarch64;
              };

              testsAarch64 = import ./nix/tests {
                pkgs = pkgsCrossAarch64;
                xdp2 = xdp2-debug-aarch64;
                prebuiltSamples = prebuiltSamplesAarch64;
              };
            in {
              # Cross-compiled xdp2 for RISC-V
              xdp2-debug-riscv64 = xdp2-debug-riscv64;

              # Pre-built samples for RISC-V (built on x86_64, runs on riscv64)
              prebuilt-samples-riscv64 = prebuiltSamplesRiscv64.all;

              # Cross-compiled tests for RISC-V (using pre-built samples)
              riscv64-tests = testsRiscv64;

              # Runner script for RISC-V tests in VM
              run-riscv64-tests = pkgs.writeShellApplication {
                name = "run-riscv64-tests";
                runtimeInputs = [ pkgs.expect pkgs.netcat-gnu ];
                text = ''
                  echo "========================================"
                  echo "  XDP2 RISC-V Sample Tests"
                  echo "========================================"
                  echo ""
                  echo "Test binary: ${testsRiscv64.all}/bin/xdp2-test-all"
                  echo ""
                  echo "Running tests inside RISC-V VM..."

                  # Use expect to run the tests
                  ${microvms.expect.riscv64.runCommand}/bin/xdp2-vm-expect-run-riscv64 \
                    "${testsRiscv64.all}/bin/xdp2-test-all"
                '';
              };

              # ─── AArch64 exports ───

              # Cross-compiled xdp2 for AArch64
              xdp2-debug-aarch64 = xdp2-debug-aarch64;

              # Pre-built samples for AArch64 (built on x86_64, runs on aarch64)
              prebuilt-samples-aarch64 = prebuiltSamplesAarch64.all;

              # Cross-compiled tests for AArch64 (using pre-built samples)
              aarch64-tests = testsAarch64;

              # Runner script for AArch64 tests in VM
              run-aarch64-tests = pkgs.writeShellApplication {
                name = "run-aarch64-tests";
                runtimeInputs = [ pkgs.expect pkgs.netcat-gnu ];
                text = ''
                  echo "========================================"
                  echo "  XDP2 AArch64 Sample Tests"
                  echo "========================================"
                  echo ""
                  echo "Test binary: ${testsAarch64.all}/bin/xdp2-test-all"
                  echo ""
                  echo "Running tests inside AArch64 VM..."

                  # Use expect to run the tests
                  ${microvms.expect.aarch64.runCommand}/bin/xdp2-vm-expect-run-aarch64 \
                    "${testsAarch64.all}/bin/xdp2-test-all"
                '';
              };
            }
          else {}
        );

        # Development shell
        devShells.default = devshell;
      });
}
