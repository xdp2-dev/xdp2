# nix/microvms/default.nix
#
# Entry point for XDP2 MicroVM test infrastructure.
#
# This module generates VMs and helper scripts for all supported architectures.
# To add a new architecture, add it to `supportedArchs` and ensure the
# corresponding configuration exists in `constants.nix`.
#
# Usage in flake.nix:
#   microvms = import ./nix/microvms { inherit pkgs lib microvm nixpkgs; buildSystem = system; };
#   packages.microvm-x86_64 = microvms.vms.x86_64;
#
# Cross-compilation:
#   The buildSystem parameter specifies where we're building FROM (host).
#   This enables true cross-compilation for non-native architectures instead
#   of slow binfmt emulation.
#
{ pkgs, lib, microvm, nixpkgs, buildSystem ? "x86_64-linux" }:

let
  constants = import ./constants.nix;
  microvmLib = import ./lib.nix { inherit pkgs lib constants; };

  # ==========================================================================
  # Supported Architectures
  # ==========================================================================
  #
  # Add new architectures here as they become supported.
  # Each architecture must have a corresponding entry in constants.nix
  #
  # Phase 2: x86_64 (KVM), aarch64 (QEMU), riscv64 (QEMU)
  #
  supportedArchs = [ "x86_64" "aarch64" "riscv64" ];

  # Path to expect scripts (used by lifecycle and helper scripts)
  scriptsDir = ./scripts;

  # ==========================================================================
  # Generate VMs for all architectures
  # ==========================================================================

  vms = lib.genAttrs supportedArchs (arch:
    import ./mkVm.nix { inherit pkgs lib microvm nixpkgs arch buildSystem; }
  );

  # ==========================================================================
  # Generate lifecycle scripts for all architectures
  # ==========================================================================

  lifecycleByArch = lib.genAttrs supportedArchs (arch:
    microvmLib.mkLifecycleScripts { inherit arch scriptsDir; }
  );

  # ==========================================================================
  # Generate helper scripts for all architectures
  # ==========================================================================

  helpersByArch = lib.genAttrs supportedArchs (arch: {
    # Console connection scripts
    connectSerial = microvmLib.mkConnectScript { inherit arch; console = "serial"; };
    connectVirtio = microvmLib.mkConnectScript { inherit arch; console = "virtio"; };

    # Interactive login scripts (with proper terminal handling)
    loginSerial = microvmLib.mkLoginScript { inherit arch; console = "serial"; };
    loginVirtio = microvmLib.mkLoginScript { inherit arch; console = "virtio"; };

    # Run command scripts
    runCommandSerial = microvmLib.mkRunCommandScript { inherit arch; console = "serial"; };
    runCommandVirtio = microvmLib.mkRunCommandScript { inherit arch; console = "virtio"; };

    # VM status
    status = microvmLib.mkStatusScript { inherit arch; };

    # Simple test runner
    testRunner = microvmLib.mkTestRunner { inherit arch; };
  });

  # ==========================================================================
  # Generate expect-based scripts for all architectures
  # ==========================================================================

  expectByArch = lib.genAttrs supportedArchs (arch:
    microvmLib.mkExpectScripts { inherit arch scriptsDir; }
  );

  # ==========================================================================
  # Test runners for individual and combined testing
  # ==========================================================================

  # Individual architecture test runners
  testsByArch = lib.genAttrs supportedArchs (arch:
    pkgs.writeShellApplication {
      name = "xdp2-test-${arch}";
      runtimeInputs = [ pkgs.coreutils ];
      text = ''
        echo "========================================"
        echo "  XDP2 MicroVM Test: ${arch}"
        echo "========================================"
        echo ""
        ${lifecycleByArch.${arch}.fullTest}/bin/xdp2-lifecycle-full-test-${arch}
      '';
    }
  );

  # Combined test runner (all architectures sequentially)
  testAll = pkgs.writeShellApplication {
    name = "xdp2-test-all-architectures";
    runtimeInputs = [ pkgs.coreutils ];
    text = ''
      echo "========================================"
      echo "  XDP2 MicroVM Test: ALL ARCHITECTURES"
      echo "========================================"
      echo ""
      echo "Architectures: ${lib.concatStringsSep ", " supportedArchs}"
      echo ""

      FAILED=""
      PASSED=""

      for arch in ${lib.concatStringsSep " " supportedArchs}; do
        echo ""
        echo "════════════════════════════════════════"
        echo "  Testing: $arch"
        echo "════════════════════════════════════════"

        # Run the lifecycle test for this architecture
        TEST_SCRIPT=""
        case "$arch" in
          x86_64)  TEST_SCRIPT="${lifecycleByArch.x86_64.fullTest}/bin/xdp2-lifecycle-full-test-x86_64" ;;
          aarch64) TEST_SCRIPT="${lifecycleByArch.aarch64.fullTest}/bin/xdp2-lifecycle-full-test-aarch64" ;;
          riscv64) TEST_SCRIPT="${lifecycleByArch.riscv64.fullTest}/bin/xdp2-lifecycle-full-test-riscv64" ;;
        esac

        if $TEST_SCRIPT; then
          PASSED="$PASSED $arch"
          echo ""
          echo "  Result: PASS"
        else
          FAILED="$FAILED $arch"
          echo ""
          echo "  Result: FAIL"
        fi
      done

      echo ""
      echo "========================================"
      echo "  Summary"
      echo "========================================"
      if [ -n "$PASSED" ]; then
        echo "  PASSED:$PASSED"
      fi
      if [ -n "$FAILED" ]; then
        echo "  FAILED:$FAILED"
        exit 1
      else
        echo ""
        echo "  All architectures passed!"
        exit 0
      fi
    '';
  };

  # ==========================================================================
  # Flattened exports (for backwards compatibility and convenience)
  # ==========================================================================
  #
  # These provide direct access to x86_64 scripts without specifying arch,
  # maintaining backwards compatibility with existing usage.
  #

  # Default architecture for backwards compatibility
  defaultArch = "x86_64";

  # Legacy exports (x86_64)
  legacyExports = {
    # VM info
    vmProcessName = constants.getProcessName defaultArch;
    vmHostname = constants.getHostname defaultArch;

    # Simple helpers (backwards compatible names)
    testRunner = helpersByArch.${defaultArch}.testRunner;
    connectConsole = helpersByArch.${defaultArch}.connectVirtio;
    connectSerial = helpersByArch.${defaultArch}.connectSerial;
    vmStatus = helpersByArch.${defaultArch}.status;

    # Login/debug helpers
    runCommandSerial = helpersByArch.${defaultArch}.runCommandSerial;
    runCommandVirtio = helpersByArch.${defaultArch}.runCommandVirtio;
    loginSerial = helpersByArch.${defaultArch}.loginSerial;
    loginVirtio = helpersByArch.${defaultArch}.loginVirtio;

    # Expect-based helpers
    expectRunCommand = expectByArch.${defaultArch}.runCommand;
    debugVmExpect = expectByArch.${defaultArch}.debug;
    expectVerifyService = expectByArch.${defaultArch}.verifyService;

    # Lifecycle (flat structure for backwards compatibility)
    lifecycle = lifecycleByArch.${defaultArch};
  };

  # ==========================================================================
  # Flat package exports for flake
  # ==========================================================================
  #
  # This generates a flat attrset suitable for packages.* exports.
  # For example: packages.xdp2-lifecycle-full-test-x86_64
  #
  flatPackages = let
    # Helper to prefix package names
    mkArchPackages = arch: let
      lc = lifecycleByArch.${arch};
      hp = helpersByArch.${arch};
      ex = expectByArch.${arch};
    in {
      # Lifecycle scripts
      "xdp2-lifecycle-0-build-${arch}" = lc.checkBuild;
      "xdp2-lifecycle-1-check-process-${arch}" = lc.checkProcess;
      "xdp2-lifecycle-2-check-serial-${arch}" = lc.checkSerial;
      "xdp2-lifecycle-2b-check-virtio-${arch}" = lc.checkVirtio;
      "xdp2-lifecycle-3-verify-ebpf-loaded-${arch}" = lc.verifyEbpfLoaded;
      "xdp2-lifecycle-4-verify-ebpf-running-${arch}" = lc.verifyEbpfRunning;
      "xdp2-lifecycle-5-shutdown-${arch}" = lc.shutdown;
      "xdp2-lifecycle-6-wait-exit-${arch}" = lc.waitExit;
      "xdp2-lifecycle-force-kill-${arch}" = lc.forceKill;
      "xdp2-lifecycle-full-test-${arch}" = lc.fullTest;

      # Helper scripts
      "xdp2-vm-serial-${arch}" = hp.connectSerial;
      "xdp2-vm-virtio-${arch}" = hp.connectVirtio;
      "xdp2-vm-login-serial-${arch}" = hp.loginSerial;
      "xdp2-vm-login-virtio-${arch}" = hp.loginVirtio;
      "xdp2-vm-run-serial-${arch}" = hp.runCommandSerial;
      "xdp2-vm-run-virtio-${arch}" = hp.runCommandVirtio;
      "xdp2-vm-status-${arch}" = hp.status;
      "xdp2-test-${arch}" = hp.testRunner;

      # Expect scripts
      "xdp2-vm-expect-run-${arch}" = ex.runCommand;
      "xdp2-vm-debug-expect-${arch}" = ex.debug;
      "xdp2-vm-expect-verify-service-${arch}" = ex.verifyService;
    };
  in lib.foldl' (acc: arch: acc // mkArchPackages arch) {} supportedArchs;

  # Legacy flat packages (without -x86_64 suffix for backwards compat)
  legacyFlatPackages = let
    lc = lifecycleByArch.${defaultArch};
    hp = helpersByArch.${defaultArch};
    ex = expectByArch.${defaultArch};
  in {
    # Lifecycle scripts (original names)
    "xdp2-lifecycle-0-build" = lc.checkBuild;
    "xdp2-lifecycle-1-check-process" = lc.checkProcess;
    "xdp2-lifecycle-2-check-serial" = lc.checkSerial;
    "xdp2-lifecycle-2b-check-virtio" = lc.checkVirtio;
    "xdp2-lifecycle-3-verify-ebpf-loaded" = lc.verifyEbpfLoaded;
    "xdp2-lifecycle-4-verify-ebpf-running" = lc.verifyEbpfRunning;
    "xdp2-lifecycle-5-shutdown" = lc.shutdown;
    "xdp2-lifecycle-6-wait-exit" = lc.waitExit;
    "xdp2-lifecycle-force-kill" = lc.forceKill;
    "xdp2-lifecycle-full-test" = lc.fullTest;

    # Helper scripts (original names)
    "xdp2-vm-console" = hp.connectVirtio;
    "xdp2-vm-serial" = hp.connectSerial;
    "xdp2-vm-login-serial" = hp.loginSerial;
    "xdp2-vm-login-virtio" = hp.loginVirtio;
    "xdp2-vm-run-serial" = hp.runCommandSerial;
    "xdp2-vm-run-virtio" = hp.runCommandVirtio;
    "xdp2-vm-status" = hp.status;
    "xdp2-test-phase1" = hp.testRunner;

    # Expect scripts (original names)
    "xdp2-vm-expect-run" = ex.runCommand;
    "xdp2-vm-debug-expect" = ex.debug;
    "xdp2-vm-expect-verify-service" = ex.verifyService;
  };

in {
  # ==========================================================================
  # Primary exports (architecture-organized)
  # ==========================================================================

  # VM derivations by architecture
  inherit vms;

  # Lifecycle scripts by architecture (use lifecycleByArch.x86_64.fullTest etc.)
  inherit lifecycleByArch;

  # Helper scripts by architecture
  helpers = helpersByArch;

  # Expect scripts by architecture
  expect = expectByArch;

  # Test runners
  tests = testsByArch // { all = testAll; };
  inherit testsByArch testAll;

  # ==========================================================================
  # Configuration
  # ==========================================================================

  inherit constants;
  inherit supportedArchs;
  inherit scriptsDir;

  # ==========================================================================
  # Backwards compatibility exports
  # ==========================================================================
  #
  # These maintain compatibility with existing code that expects:
  #   microvms.testRunner
  #   microvms.lifecycle.fullTest
  #   etc.
  #
  # Default lifecycle (x86_64) - use microvms.lifecycle.fullTest etc.
  lifecycle = legacyExports.lifecycle;

  inherit (legacyExports)
    vmProcessName
    vmHostname
    testRunner
    connectConsole
    connectSerial
    vmStatus
    runCommandSerial
    runCommandVirtio
    loginSerial
    loginVirtio
    expectRunCommand
    debugVmExpect
    expectVerifyService
    ;

  # ==========================================================================
  # Flat package exports (for flake.nix packages.*)
  # ==========================================================================

  # All packages with architecture suffix
  packages = flatPackages // legacyFlatPackages;

  # Just the new architecture-suffixed packages
  archPackages = flatPackages;

  # Just the legacy packages (no suffix)
  legacyPackages = legacyFlatPackages;
}
