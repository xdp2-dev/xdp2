# nix/microvms/mkVm.nix
#
# Parameterized MicroVM definition for eBPF testing.
# Supports multiple architectures through the `arch` parameter.
#
# Usage:
#   import ./mkVm.nix { inherit pkgs lib microvm nixpkgs; arch = "x86_64"; }
#
# Cross-compilation:
#   When buildSystem differs from target arch (e.g., building riscv64 on x86_64),
#   we use proper cross-compilation (localSystem/crossSystem) instead of binfmt
#   emulation. This is significantly faster as it uses native cross-compilers.
#
{ pkgs, lib, microvm, nixpkgs, arch, buildSystem ? "x86_64-linux" }:

let
  constants = import ./constants.nix;
  cfg = constants.architectures.${arch};
  hostname = constants.getHostname arch;
  kernelPackageName = constants.getKernelPackage arch;

  # Architecture-specific QEMU arguments
  # Note: -cpu is handled by microvm.cpu option, not extraArgs
  # Note: -machine is handled by microvm.qemu.machine option, not extraArgs
  # Note: -enable-kvm is handled by microvm.nix when cpu == null on Linux
  archQemuArgs = {
    x86_64 = [
      # KVM and CPU handled by microvm.nix (cpu = null triggers -enable-kvm -cpu host)
    ];

    aarch64 = [
      # CPU handled by microvm.cpu option (cpu = cortex-a72)
      # Machine handled by microvm.qemu.machine option
    ];

    riscv64 = [
      # CPU handled by microvm.cpu option (cpu = rv64)
      # Machine handled by microvm.qemu.machine option
      "-bios" "default"  # Use OpenSBI firmware
    ];
  };

  # QEMU machine options for microvm.nix
  # We need to explicitly set TCG acceleration for cross-architecture emulation
  # (running aarch64/riscv64 VMs on x86_64 host)
  archMachineOpts = {
    x86_64 = null;   # Use microvm.nix defaults (KVM on x86_64 host)

    aarch64 = {
      # ARM64 emulation on x86_64 host requires TCG (software emulation)
      accel = "tcg";
    };

    riscv64 = {
      # RISC-V 64-bit emulation on x86_64 host requires TCG
      accel = "tcg";
    };
  };

  # QEMU package override to disable seccomp sandbox for cross-arch emulation
  # The default QEMU has seccomp enabled, but the -sandbox option doesn't work
  # properly for cross-architecture targets (e.g., qemu-system-aarch64 on x86_64)
  qemuWithoutSandbox = pkgs.qemu.override {
    # Disable seccomp to prevent -sandbox on being added to command line
    seccompSupport = false;
  };

  # Overlay to disable tests for packages that fail under QEMU user-mode emulation
  # These packages build successfully but their test suites fail under QEMU binfmt_misc
  # due to threading, I/O timing, or QEMU plugin bugs.
  # The builds succeed; only the test phases fail.
  crossEmulationOverlay = final: prev: {
    # boehm-gc: QEMU plugin bug with threading
    #   ERROR:../plugins/core.c:292:qemu_plugin_vcpu_init__async: assertion failed
    boehmgc = prev.boehmgc.overrideAttrs (oldAttrs: {
      doCheck = false;
    });

    # libuv: I/O and event loop tests fail under QEMU emulation
    #   Test suite passes 421 individual tests but overall test harness fails
    libuv = prev.libuv.overrideAttrs (oldAttrs: {
      doCheck = false;
    });

    # libseccomp: 1 of 5118 tests fails under QEMU emulation
    #   Seccomp BPF simulation tests have timing/syscall issues under emulation
    libseccomp = prev.libseccomp.overrideAttrs (oldAttrs: {
      doCheck = false;
    });

    # meson: Tests timeout under QEMU emulation
    #   "254 long output" test times out (SIGTERM after 60s)
    meson = prev.meson.overrideAttrs (oldAttrs: {
      doCheck = false;
      doInstallCheck = false;
    });

    # gnutls: Cross-compilation fails because doc tools run on build host
    #   ./errcodes: cannot execute binary file: Exec format error
    # The build compiles errcodes/printlist for target arch, then tries to run
    # them on the build host to generate documentation. Disable docs for cross.
    gnutls = prev.gnutls.overrideAttrs (oldAttrs: {
      # Disable documentation generation which requires running target binaries
      configureFlags = (oldAttrs.configureFlags or []) ++ [ "--disable-doc" ];
      # Remove man and devdoc outputs since we disabled doc generation
      # Default outputs are: bin dev out man devdoc
      outputs = builtins.filter (o: o != "devdoc" && o != "man") (oldAttrs.outputs or [ "out" ]);
    });

    # tbb: Uses -fcf-protection=full which is x86-only
    #   cc1plus: error: '-fcf-protection=full' is not supported for this target
    # Always apply fix - harmless on x86, required for other architectures
    tbb = prev.tbb.overrideAttrs (oldAttrs: {
      # Disable Nix's CET hardening (includes -fcf-protection)
      hardeningDisable = (oldAttrs.hardeningDisable or []) ++ [ "cet" ];

      # Remove -fcf-protection from TBB's CMake files
      # GNU.cmake line 111 and Clang.cmake line 69 add this x86-specific flag
      # Use find because source extracts to subdirectory (e.g., oneTBB-2022.2.0/)
      postPatch = (oldAttrs.postPatch or "") + ''
        echo "Patching TBB cmake files to remove -fcf-protection..."
        find . -type f \( -name "GNU.cmake" -o -name "Clang.cmake" \) -exec \
          sed -i '/fcf-protection/d' {} \; -print
      '';
    });


    # Python packages that fail tests under QEMU emulation
    # Use pythonPackagesExtensions which properly propagates to all Python versions
    pythonPackagesExtensions = prev.pythonPackagesExtensions ++ [
      (pyFinal: pyPrev: {
        # psutil: Network ioctl tests fail under QEMU emulation
        #   OSError: [Errno 25] Inappropriate ioctl for device (SIOCETHTOOL)
        psutil = pyPrev.psutil.overrideAttrs (old: {
          doCheck = false;
          doInstallCheck = false;
        });

        # pytest-timeout: Timing tests fail under QEMU emulation
        #   0.01 second timeouts don't fire reliably under emulation
        pytest-timeout = pyPrev.pytest-timeout.overrideAttrs (old: {
          doCheck = false;
          doInstallCheck = false;
        });
      })
    ];
  };

  # Check that the kernel has BTF support (required for CO-RE eBPF programs)
  kernelHasBtf = pkgs.${kernelPackageName}.kernel.configfile != null &&
    builtins.match ".*CONFIG_DEBUG_INFO_BTF=y.*"
      (builtins.readFile pkgs.${kernelPackageName}.kernel.configfile) != null;

  # Assertion to fail early if BTF is not available
  _ = assert kernelHasBtf || throw ''
    ERROR: Kernel ${kernelPackageName} does not have BTF support enabled.

    BTF (BPF Type Format) is required for CO-RE eBPF programs.
    The VM guest kernel must be built with CONFIG_DEBUG_INFO_BTF=y.

    Note: The hypervisor (host) machine compiles eBPF to bytecode quickly,
    while the VM only needs to verify and JIT the pre-compiled bytecode.
    A more powerful host machine speeds up eBPF compilation significantly.

    Options:
    1. Use a different kernel package (e.g., linuxPackages_latest)
    2. Build a custom kernel with BTF enabled
    3. Use a NixOS system with BTF-enabled kernel

    Current kernel: ${kernelPackageName}
    Architecture: ${arch}
  ''; true;

# Determine if we need cross-compilation
# Cross-compilation is needed when the build system differs from target system
needsCross = buildSystem != cfg.nixSystem;

# Create pre-overlayed pkgs for the target system
# This ensures ALL packages (including transitive deps like TBB) use the overlay
#
# IMPORTANT: For cross-compilation, we use localSystem/crossSystem instead of
# just setting system. This tells Nix to use native cross-compilers rather than
# falling back to binfmt_misc emulation (which is extremely slow).
#
# - localSystem: Where we BUILD (host machine, e.g., x86_64-linux)
# - crossSystem: Where binaries RUN (target, e.g., riscv64-linux)
#
overlayedPkgs = import nixpkgs (
  if needsCross then {
    # True cross-compilation: use native cross-compiler toolchain
    localSystem = buildSystem;
    crossSystem = cfg.nixSystem;
    overlays = [ crossEmulationOverlay ];
    config = { allowUnfree = true; };
  } else {
    # Native build: building for the same system we're on
    system = cfg.nixSystem;
    overlays = [ crossEmulationOverlay ];
    config = { allowUnfree = true; };
  }
);

in (nixpkgs.lib.nixosSystem {
  # Pass our pre-overlayed pkgs to nixosSystem
  # The pkgs argument is used as the basis for all package evaluation
  pkgs = overlayedPkgs;

  # Also pass specialArgs to make overlayedPkgs available in modules
  specialArgs = { inherit overlayedPkgs; };

  modules = [
    # MicroVM module
    microvm.nixosModules.microvm

    # CRITICAL: Force use of our pre-overlayed pkgs everywhere
    # We use multiple mechanisms to ensure packages come from our overlay
    ({ lib, ... }: {
      # Set _module.args.pkgs to our overlayed pkgs
      # This is the most direct way to control what pkgs modules receive
      _module.args.pkgs = lib.mkForce overlayedPkgs;

      # Also set nixpkgs.pkgs to prevent the nixpkgs module from reconstructing
      nixpkgs.pkgs = lib.mkForce overlayedPkgs;

      # Don't let it use localSystem/crossSystem to rebuild
      # (these would cause it to re-import nixpkgs without our overlay)
      nixpkgs.hostPlatform = lib.mkForce (overlayedPkgs.stdenv.hostPlatform);
      nixpkgs.buildPlatform = lib.mkForce (overlayedPkgs.stdenv.buildPlatform);
    })

    # VM configuration
    ({ config, pkgs, ... }:
    let
      # bpftools package (provides bpftool command)
      bpftools = pkgs.bpftools;

      # Self-test script using writeShellApplication for correctness
      selfTestScript = pkgs.writeShellApplication {
        name = "xdp2-self-test";
        runtimeInputs = [
          pkgs.coreutils
          pkgs.iproute2
          bpftools
        ];
        text = ''
          echo "========================================"
          echo "  XDP2 MicroVM Self-Test"
          echo "========================================"
          echo ""
          echo "Architecture: $(uname -m)"
          echo "Kernel: $(uname -r)"
          echo "Hostname: $(hostname)"
          echo ""

          # Check BTF availability
          echo "--- BTF Check ---"
          if [ -f /sys/kernel/btf/vmlinux ]; then
            echo "BTF: AVAILABLE"
            ls -la /sys/kernel/btf/vmlinux
          else
            echo "BTF: NOT AVAILABLE"
            echo "ERROR: BTF is required for CO-RE eBPF programs"
            exit 1
          fi
          echo ""

          # Check bpftool
          echo "--- bpftool Check ---"
          bpftool version
          echo ""

          # Probe BPF features
          echo "--- BPF Features (first 15) ---"
          bpftool feature probe kernel 2>/dev/null | head -15 || true
          echo ""

          # Check XDP support
          echo "--- XDP Support ---"
          if bpftool feature probe kernel 2>/dev/null | grep -q "xdp"; then
            echo "XDP: SUPPORTED"
          else
            echo "XDP: Check manually"
          fi
          echo ""

          # Check network interface for XDP
          echo "--- Network Interface (${constants.xdpInterface}) ---"
          if ip link show ${constants.xdpInterface} >/dev/null 2>&1; then
            echo "Interface: ${constants.xdpInterface} AVAILABLE"
            ip link show ${constants.xdpInterface}
          else
            echo "Interface: ${constants.xdpInterface} NOT FOUND"
            echo "Available interfaces:"
            ip link show
          fi
          echo ""

          echo "========================================"
          echo "  Self-Test Complete: SUCCESS"
          echo "========================================"
        '';
      };
    in {
      # ==================================================================
      # MINIMAL SYSTEM - eBPF testing only
      # ==================================================================
      # Disable everything not needed for eBPF/XDP testing to minimize
      # build time and dependencies (especially for cross-compilation)

      # Disable all documentation (pulls in texlive, gtk-doc, etc.)
      documentation.enable = false;
      documentation.man.enable = false;
      documentation.doc.enable = false;
      documentation.info.enable = false;
      documentation.nixos.enable = false;

      # Disable unnecessary services
      security.polkit.enable = false;
      services.udisks2.enable = false;
      programs.command-not-found.enable = false;

      # Minimal fonts (none needed for headless eBPF testing)
      fonts.fontconfig.enable = false;

      # Disable nix-daemon and nix tools in the VM
      # We only need to run pre-compiled eBPF programs, not build packages
      nix.enable = false;

      # Disable XDG MIME database (pulls in shared-mime-info + glib)
      xdg.mime.enable = false;

      # Use a minimal set of supported filesystems (no btrfs, etc.)
      boot.supportedFilesystems = lib.mkForce [ "vfat" "ext4" ];

      # Disable firmware (not needed in VM)
      hardware.enableRedistributableFirmware = false;

      # ==================================================================
      # Basic NixOS configuration
      # ==================================================================

      system.stateVersion = "26.05";
      networking.hostName = hostname;

      # ==================================================================
      # MicroVM configuration
      # ==================================================================

      microvm = {
        hypervisor = "qemu";
        mem = cfg.mem;
        vcpu = cfg.vcpu;

        # Set CPU explicitly for non-KVM architectures to prevent -enable-kvm
        # When cpu is null and host is Linux, microvm.nix adds -enable-kvm
        # For cross-arch emulation (TCG), we set the CPU to prevent this
        # For KVM (x86_64 on x86_64 host), leave null to get -enable-kvm -cpu host
        cpu = if cfg.useKvm then null else cfg.qemuCpu;

        # No persistent storage needed for testing
        volumes = [];

        # Network interface for XDP testing
        interfaces = [{
          type = "user";  # QEMU user networking (NAT to host)
          id = "eth0";
          mac = constants.tapConfig.mac;
        }];

        # Mount host Nix store for instant access to binaries
        shares = [{
          source = "/nix/store";
          mountPoint = "/nix/store";
          tag = "nix-store";
          proto = "9p";
        }];

        # QEMU configuration
        qemu = {
          # Disable default serial console (we configure our own)
          serialConsole = false;

          # Machine type (virt for aarch64/riscv64, pc for x86_64)
          machine = cfg.qemuMachine;

          # Use QEMU without seccomp for cross-arch emulation
          # The -sandbox option doesn't work properly for cross-arch targets
          package = if cfg.useKvm then pkgs.qemu_kvm else qemuWithoutSandbox;

          extraArgs = archQemuArgs.${arch} ++ [
            # VM identification
            "-name" "${hostname},process=${hostname}"

            # Serial console on TCP port (for boot messages)
            "-serial" "tcp:127.0.0.1:${toString cfg.serialPort},server,nowait"

            # Virtio console (faster, for interactive use)
            "-device" "virtio-serial-pci"
            "-chardev" "socket,id=virtcon,port=${toString cfg.virtioPort},host=127.0.0.1,server=on,wait=off"
            "-device" "virtconsole,chardev=virtcon"

            # Kernel command line - CRITICAL for NixOS boot
            # microvm.nix doesn't generate -append for non-microvm machine types
            # We must include init= to tell initrd where the NixOS system is
            "-append" (builtins.concatStringsSep " " ([
              "console=${cfg.consoleDevice},115200"
              "console=hvc0"
              "reboot=t"
              "panic=-1"
              "loglevel=4"
              "init=${config.system.build.toplevel}/init"
            ] ++ config.boot.kernelParams))
          ];
        } // (if archMachineOpts.${arch} != null then {
          # Provide machineOpts for architectures not built-in to microvm.nix
          machineOpts = archMachineOpts.${arch};
        } else {});
      };

      # ==================================================================
      # Kernel configuration
      # ==================================================================

      boot.kernelPackages = pkgs.${kernelPackageName};

      # Console configuration (architecture-specific serial device)
      boot.kernelParams = [
        "console=${cfg.consoleDevice},115200"  # Serial first (for early boot)
        "console=hvc0"                          # Virtio console (becomes primary)
        # Boot options to handle failures gracefully
        "systemd.default_standard_error=journal+console"
        "systemd.show_status=true"
      ];

      # Ensure 9p kernel modules are available in initrd for mounting /nix/store
      boot.initrd.availableKernelModules = [
        "9p"
        "9pnet"
        "9pnet_virtio"
        "virtio_pci"
        "virtio_console"
      ];

      # Force initrd to continue booting even if something fails
      # This avoids dropping to emergency shell with locked root
      boot.initrd.systemd.emergencyAccess = true;

      # eBPF sysctls
      boot.kernel.sysctl = {
        "net.core.bpf_jit_enable" = 1;
        "kernel.unprivileged_bpf_disabled" = 0;
      };

      # ==================================================================
      # User configuration
      # ==================================================================

      # Auto-login for testing
      services.getty.autologinUser = "root";
      # Use pre-computed hash for "test" password (works with sulogin)
      users.users.root.hashedPassword = "$6$xyz$LH8r4wzLEMW8IaOSNSaJiXCrfvBsXKjJhBauJQIFsT7xbKkNdM0xQx7gQZt.z6G.xj2wX0qxGm.7eVxJqkDdH0";
      # Disable emergency mode - boot continues even if something fails
      systemd.enableEmergencyMode = false;

      # ==================================================================
      # Test tools
      # ==================================================================

      environment.systemPackages = with pkgs; [
        bpftools
        iproute2
        tcpdump
        ethtool
        coreutils
        procps
        util-linux
        selfTestScript
        # Note: XDP samples are compiled to BPF bytecode on the host using
        # clang -target bpf, then loaded in the VM. No need for clang here.
      ];

      # ==================================================================
      # Self-test service
      # ==================================================================

      systemd.services.xdp2-self-test = {
        description = "XDP2 MicroVM Self-Test";
        after = [ "multi-user.target" ];
        wantedBy = [ "multi-user.target" ];

        serviceConfig = {
          Type = "oneshot";
          RemainAfterExit = true;
          ExecStart = "${selfTestScript}/bin/xdp2-self-test";
        };
      };
    })
  ];
}).config.microvm.declaredRunner
