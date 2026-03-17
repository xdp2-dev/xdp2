# nix/microvms/constants.nix
#
# Configuration constants for XDP2 MicroVM test infrastructure.
#
# Phase 2: x86_64, aarch64, riscv64
# See: documentation/nix/microvm-phase2-arm-riscv-plan.md
#
rec {
  # ==========================================================================
  # Port allocation scheme for multiple MicroVMs
  # ==========================================================================
  #
  # Base port: 23500 (well out of common service ranges)
  # Each architecture gets a block of 10 ports:
  #   - x86_64:  23500-23509
  #   - aarch64: 23510-23519
  #   - riscv64: 23520-23529
  #   - riscv32: 23530-23539
  #
  # Within each block:
  #   +0 = serial console (ttyS0/ttyAMA0)
  #   +1 = virtio console (hvc0)
  #   +2 = reserved (future: GDB, monitor, etc.)
  #   +3-9 = reserved for kernel variants
  #
  portBase = 23500;

  # Port offset per architecture
  archPortOffset = {
    x86_64 = 0;
    aarch64 = 10;
    riscv64 = 20;
    riscv32 = 30;
  };

  # Helper to calculate ports for an architecture
  getPorts = arch: let
    base = portBase + archPortOffset.${arch};
  in {
    serial = base;      # +0
    virtio = base + 1;  # +1
  };

  # ==========================================================================
  # Architecture Definitions
  # ==========================================================================

  architectures = {
    x86_64 = {
      # Nix system identifier
      nixSystem = "x86_64-linux";

      # QEMU configuration
      qemuMachine = "pc";
      qemuCpu = "host";  # Use host CPU features with KVM
      useKvm = true;     # x86_64 can use KVM on x86_64 host

      # Console device (architecture-specific)
      consoleDevice = "ttyS0";

      # Console ports (TCP) - using port allocation scheme
      serialPort = portBase + archPortOffset.x86_64;       # 23500
      virtioPort = portBase + archPortOffset.x86_64 + 1;   # 23501

      # VM resources
      mem = 1024;  # 1GB RAM
      vcpu = 2;    # 2 vCPUs

      # Description
      description = "x86_64 (KVM accelerated)";
    };

    aarch64 = {
      # Nix system identifier
      nixSystem = "aarch64-linux";

      # QEMU configuration
      qemuMachine = "virt";
      qemuCpu = "cortex-a72";
      useKvm = false;  # Cross-arch emulation (QEMU TCG)

      # Console device (aarch64 uses ttyAMA0, not ttyS0)
      consoleDevice = "ttyAMA0";

      # Console ports (TCP)
      serialPort = portBase + archPortOffset.aarch64;       # 23510
      virtioPort = portBase + archPortOffset.aarch64 + 1;   # 23511

      # VM resources
      mem = 1024;
      vcpu = 2;

      # Description
      description = "aarch64 (ARM64, QEMU emulated)";
    };

    riscv64 = {
      # Nix system identifier
      nixSystem = "riscv64-linux";

      # QEMU configuration
      qemuMachine = "virt";
      qemuCpu = "rv64";  # Default RISC-V 64-bit CPU
      useKvm = false;    # Cross-arch emulation (QEMU TCG)

      # Console device
      consoleDevice = "ttyS0";

      # Console ports (TCP)
      serialPort = portBase + archPortOffset.riscv64;       # 23520
      virtioPort = portBase + archPortOffset.riscv64 + 1;   # 23521

      # VM resources
      mem = 1024;
      vcpu = 2;

      # Description
      description = "riscv64 (RISC-V 64-bit, QEMU emulated)";
    };
  };

  # ==========================================================================
  # Kernel configuration
  # ==========================================================================

  # Use linuxPackages_latest for cross-arch VMs (better BTF/eBPF support)
  # Use stable linuxPackages for KVM (x86_64) for stability
  getKernelPackage = arch:
    if architectures.${arch}.useKvm or false
    then "linuxPackages"         # Stable for KVM (x86_64)
    else "linuxPackages_latest"; # Latest for emulated (better BTF)

  # Legacy: default kernel package (for backwards compatibility)
  kernelPackage = "linuxPackages";

  # ==========================================================================
  # Lifecycle timing configuration
  # ==========================================================================

  # Polling interval for lifecycle checks (seconds)
  # Use 1 second for most checks; shell sleep doesn't support sub-second easily
  pollInterval = 1;

  # Per-phase timeouts (seconds)
  # KVM is fast, so timeouts can be relatively short
  timeouts = {
    # Phase 0: Build timeout
    # Building from scratch can take a while (kernel, systemd, etc.)
    # 10 minutes should be enough for most cases
    build = 600;

    # Phase 1: Process should start almost immediately
    processStart = 5;

    # Phase 2: Serial console available (early boot)
    # QEMU starts quickly, but kernel needs to initialize serial
    serialReady = 30;

    # Phase 2b: Virtio console available (requires virtio drivers)
    virtioReady = 45;

    # Phase 3: Self-test service completion
    # Depends on systemd reaching multi-user.target
    serviceReady = 60;

    # Phase 4: Command response timeout
    # Individual commands via netcat
    command = 5;

    # Phase 5-6: Shutdown
    # Graceful shutdown should be quick with systemd
    shutdown = 30;

    # Legacy aliases for compatibility
    boot = 60;
  };

  # Timeouts for QEMU emulated architectures (slower than KVM)
  # NOTE: build timeout is longer because we compile QEMU without seccomp
  # support (qemu-for-vm-tests), which takes 15-30 minutes from scratch.
  # Once cached, subsequent builds are fast. Runtime is fine once built.
  timeoutsQemu = {
    build = 2400;           # 40 minutes (QEMU compilation from source)
    processStart = 5;
    serialReady = 30;
    virtioReady = 45;
    serviceReady = 120;     # Emulation slows systemd boot
    command = 10;
    shutdown = 30;
    boot = 120;
  };

  # Timeouts for slow QEMU emulation (RISC-V is particularly slow)
  # NOTE: RISC-V VMs require cross-compiled kernel/userspace plus
  # QEMU compilation, which can take 45-60 minutes total.
  # Runtime is slower due to full software emulation.
  timeoutsQemuSlow = {
    build = 3600;           # 60 minutes (QEMU + cross-compiled packages)
    processStart = 10;
    serialReady = 60;
    virtioReady = 90;
    serviceReady = 180;     # RISC-V emulation is slow
    command = 15;
    shutdown = 60;
    boot = 180;
  };

  # Get appropriate timeouts for an architecture
  getTimeouts = arch:
    if architectures.${arch}.useKvm or false
    then timeouts            # KVM (fast)
    else if arch == "riscv64" || arch == "riscv32"
    then timeoutsQemuSlow    # RISC-V is particularly slow
    else timeoutsQemu;       # Other emulated archs (aarch64)

  # ==========================================================================
  # VM naming
  # ==========================================================================

  vmNamePrefix = "xdp2-test";

  # Architecture name mapping (for valid hostnames - no underscores allowed)
  archHostname = {
    x86_64 = "x86-64";
    aarch64 = "aarch64";
    riscv64 = "riscv64";
    riscv32 = "riscv32";
  };

  # Helper to get full VM hostname (must be valid hostname - no underscores)
  getHostname = arch: "xdp2-test-${archHostname.${arch}}";

  # VM process name (used for ps matching)
  # This matches the -name argument passed to QEMU
  getProcessName = arch: "xdp2-test-${archHostname.${arch}}";

  # ==========================================================================
  # Network interface for eBPF/XDP testing
  # ==========================================================================

  # The interface name inside the VM where XDP programs will be attached
  # Using a TAP interface for realistic network testing
  xdpInterface = "eth0";

  # TAP interface configuration (for QEMU user networking)
  tapConfig = {
    # QEMU user networking provides NAT to host
    # The guest sees this as eth0
    model = "virtio-net-pci";
    mac = "52:54:00:12:34:56";
  };
}
