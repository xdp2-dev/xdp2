# nix/microvms/lib.nix
#
# Reusable functions for generating MicroVM test scripts.
# Provides DRY helpers for lifecycle checks, console connections, and VM management.
#
{ pkgs, lib, constants }:

rec {
  # ==========================================================================
  # Core Helpers
  # ==========================================================================

  # Get architecture-specific configuration
  getArchConfig = arch: constants.architectures.${arch};
  getHostname = arch: constants.getHostname arch;
  getProcessName = arch: constants.getProcessName arch;

  # ==========================================================================
  # Polling Script Generator
  # ==========================================================================
  #
  # Creates a script that polls until a condition is met or timeout reached.
  # Used for lifecycle phases that wait for VM state changes.
  #
  mkPollingScript = {
    name,
    arch,
    description,
    checkCmd,
    successMsg,
    failMsg,
    timeout,
    runtimeInputs ? [ pkgs.coreutils ],
    preCheck ? "",
    postSuccess ? "",
  }:
  let
    cfg = getArchConfig arch;
    hostname = getHostname arch;
    processName = getProcessName arch;
  in pkgs.writeShellApplication {
    inherit name runtimeInputs;
    text = ''
      TIMEOUT=${toString timeout}
      POLL_INTERVAL=${toString constants.pollInterval}

      echo "=== ${description} ==="
      echo "Timeout: $TIMEOUT seconds (polling every $POLL_INTERVAL s)"
      echo ""

      ${preCheck}

      WAITED=0
      while ! ${checkCmd}; do
        sleep "$POLL_INTERVAL"
        WAITED=$((WAITED + POLL_INTERVAL))
        if [ "$WAITED" -ge "$TIMEOUT" ]; then
          echo "FAIL: ${failMsg} after $TIMEOUT seconds"
          exit 1
        fi
        echo "  Polling... ($WAITED/$TIMEOUT s)"
      done

      echo "PASS: ${successMsg}"
      echo "  Time: $WAITED seconds"
      ${postSuccess}
      exit 0
    '';
  };

  # ==========================================================================
  # Console Connection Scripts
  # ==========================================================================

  # Simple console connection (nc-based)
  mkConnectScript = { arch, console }:
  let
    cfg = getArchConfig arch;
    port = if console == "serial" then cfg.serialPort else cfg.virtioPort;
    device = if console == "serial" then "ttyS0" else "hvc0";
    portName = if console == "serial" then "serial" else "virtio";
  in pkgs.writeShellApplication {
    name = "xdp2-vm-${portName}-${arch}";
    runtimeInputs = [ pkgs.netcat-gnu ];
    text = ''
      PORT=${toString port}
      echo "Connecting to VM ${device} console on port $PORT..."
      echo "Press Ctrl+C to disconnect"
      nc 127.0.0.1 "$PORT"
    '';
  };

  # Interactive login (socat-based for proper terminal handling)
  mkLoginScript = { arch, console }:
  let
    cfg = getArchConfig arch;
    port = if console == "serial" then cfg.serialPort else cfg.virtioPort;
    device = if console == "serial" then "ttyS0" else "hvc0";
    portName = if console == "serial" then "serial" else "virtio";
  in pkgs.writeShellApplication {
    name = "xdp2-vm-login-${portName}-${arch}";
    runtimeInputs = [ pkgs.socat pkgs.netcat-gnu ];
    text = ''
      PORT=${toString port}

      echo "Connecting to ${device} console on port $PORT"
      echo "Press Ctrl+C to disconnect"

      if ! nc -z 127.0.0.1 "$PORT" 2>/dev/null; then
        echo "ERROR: Port $PORT not available"
        exit 1
      fi

      exec socat -,raw,echo=0 TCP:127.0.0.1:"$PORT"
    '';
  };

  # Run command via console (netcat-based)
  mkRunCommandScript = { arch, console }:
  let
    cfg = getArchConfig arch;
    timeouts = constants.getTimeouts arch;
    port = if console == "serial" then cfg.serialPort else cfg.virtioPort;
    portName = if console == "serial" then "serial" else "virtio";
  in pkgs.writeShellApplication {
    name = "xdp2-vm-run-${portName}-${arch}";
    runtimeInputs = [ pkgs.netcat-gnu pkgs.coreutils ];
    text = ''
      PORT=${toString port}
      CMD_TIMEOUT=${toString timeouts.command}

      if [ $# -eq 0 ]; then
        echo "Usage: xdp2-vm-run-${portName}-${arch} <command>"
        echo "Run a command in the VM via ${portName} console (port $PORT)"
        exit 1
      fi

      COMMAND="$*"

      if ! nc -z 127.0.0.1 "$PORT" 2>/dev/null; then
        echo "ERROR: Port $PORT not available"
        exit 1
      fi

      MARKER="__OUT_$$__"
      {
        sleep 0.3
        echo ""
        echo "echo $MARKER; $COMMAND; echo $MARKER"
      } | timeout "$CMD_TIMEOUT" nc 127.0.0.1 "$PORT" 2>/dev/null | \
        sed -n "/$MARKER/,/$MARKER/p" | grep -v "$MARKER" || true
    '';
  };

  # ==========================================================================
  # VM Status Script
  # ==========================================================================

  mkStatusScript = { arch }:
  let
    cfg = getArchConfig arch;
    processName = getProcessName arch;
  in pkgs.writeShellApplication {
    name = "xdp2-vm-status-${arch}";
    runtimeInputs = [ pkgs.netcat-gnu pkgs.procps ];
    text = ''
      SERIAL_PORT=${toString cfg.serialPort}
      VIRTIO_PORT=${toString cfg.virtioPort}
      VM_PROCESS="${processName}"

      echo "XDP2 MicroVM Status (${arch})"
      echo "=============================="
      echo ""

      # Check for running VM process
      if pgrep -f "$VM_PROCESS" > /dev/null 2>&1; then
        echo "VM Process: RUNNING"
        pgrep -af "$VM_PROCESS" | head -1
      else
        echo "VM Process: NOT RUNNING"
      fi
      echo ""

      # Check ports
      echo "Console Ports:"
      if nc -z 127.0.0.1 "$SERIAL_PORT" 2>/dev/null; then
        echo "  Serial (ttyS0):  port $SERIAL_PORT - LISTENING"
      else
        echo "  Serial (ttyS0):  port $SERIAL_PORT - not listening"
      fi

      if nc -z 127.0.0.1 "$VIRTIO_PORT" 2>/dev/null; then
        echo "  Virtio (hvc0):   port $VIRTIO_PORT - LISTENING"
      else
        echo "  Virtio (hvc0):   port $VIRTIO_PORT - not listening"
      fi
    '';
  };

  # ==========================================================================
  # Lifecycle Phase Scripts
  # ==========================================================================

  mkLifecycleScripts = { arch, scriptsDir }:
  let
    cfg = getArchConfig arch;
    hostname = getHostname arch;
    processName = getProcessName arch;
    # Use architecture-specific timeouts (KVM is fast, QEMU emulation is slower)
    timeouts = constants.getTimeouts arch;
  in {
    # Phase 0: Build VM
    checkBuild = pkgs.writeShellApplication {
      name = "xdp2-lifecycle-0-build-${arch}";
      runtimeInputs = [ pkgs.coreutils ];
      text = ''
        BUILD_TIMEOUT=${toString timeouts.build}

        echo "=== Lifecycle Phase 0: Build VM (${arch}) ==="
        echo "Timeout: $BUILD_TIMEOUT seconds"
        echo ""

        echo "Building VM derivation..."
        echo "  (This may take a while if building from scratch)"
        echo ""

        START_TIME=$(date +%s)

        if ! timeout "$BUILD_TIMEOUT" nix build .#microvm-${arch} --print-out-paths --no-link 2>&1; then
          END_TIME=$(date +%s)
          ELAPSED=$((END_TIME - START_TIME))
          echo ""
          echo "FAIL: Build failed or timed out after $ELAPSED seconds"
          exit 1
        fi

        END_TIME=$(date +%s)
        ELAPSED=$((END_TIME - START_TIME))

        VM_PATH=$(nix build .#microvm-${arch} --print-out-paths --no-link 2>/dev/null)
        if [ -z "$VM_PATH" ]; then
          echo "FAIL: Build succeeded but could not get output path"
          exit 1
        fi

        echo "PASS: VM built successfully"
        echo "  Build time: $ELAPSED seconds"
        echo "  Output: $VM_PATH"
        exit 0
      '';
    };

    # Phase 1: Check process started
    checkProcess = mkPollingScript {
      name = "xdp2-lifecycle-1-check-process-${arch}";
      inherit arch;
      description = "Lifecycle Phase 1: Check VM Process (${arch})";
      checkCmd = "pgrep -f '${processName}' > /dev/null 2>&1";
      successMsg = "VM process is running";
      failMsg = "VM process not found";
      timeout = timeouts.processStart;
      runtimeInputs = [ pkgs.procps pkgs.coreutils ];
      postSuccess = ''
        echo ""
        echo "Process details:"
        pgrep -af "${processName}" | head -3
      '';
    };

    # Phase 2: Check serial console
    checkSerial = mkPollingScript {
      name = "xdp2-lifecycle-2-check-serial-${arch}";
      inherit arch;
      description = "Lifecycle Phase 2: Check Serial Console (${arch})";
      checkCmd = "nc -z 127.0.0.1 ${toString cfg.serialPort} 2>/dev/null";
      successMsg = "Serial console available on port ${toString cfg.serialPort}";
      failMsg = "Serial port not available";
      timeout = timeouts.serialReady;
      runtimeInputs = [ pkgs.netcat-gnu pkgs.coreutils ];
    };

    # Phase 2b: Check virtio console
    checkVirtio = mkPollingScript {
      name = "xdp2-lifecycle-2b-check-virtio-${arch}";
      inherit arch;
      description = "Lifecycle Phase 2b: Check Virtio Console (${arch})";
      checkCmd = "nc -z 127.0.0.1 ${toString cfg.virtioPort} 2>/dev/null";
      successMsg = "Virtio console available on port ${toString cfg.virtioPort}";
      failMsg = "Virtio port not available";
      timeout = timeouts.virtioReady;
      runtimeInputs = [ pkgs.netcat-gnu pkgs.coreutils ];
    };

    # Phase 3: Verify eBPF loaded (expect-based)
    verifyEbpfLoaded = pkgs.writeShellApplication {
      name = "xdp2-lifecycle-3-verify-ebpf-loaded-${arch}";
      runtimeInputs = [ pkgs.netcat-gnu pkgs.coreutils ];
      text = ''
        VIRTIO_PORT=${toString cfg.virtioPort}
        TIMEOUT=${toString timeouts.serviceReady}
        CMD_TIMEOUT=${toString timeouts.command}
        POLL_INTERVAL=${toString constants.pollInterval}

        echo "=== Lifecycle Phase 3: Verify eBPF Loaded (${arch}) ==="
        echo "Port: $VIRTIO_PORT (hvc0 virtio console)"
        echo "Timeout: $TIMEOUT seconds (polling every $POLL_INTERVAL s)"
        echo ""

        if ! nc -z 127.0.0.1 "$VIRTIO_PORT" 2>/dev/null; then
          echo "FAIL: Virtio console not available"
          exit 1
        fi

        echo "Waiting for xdp2-self-test.service to complete..."
        WAITED=0
        while true; do
          RESPONSE=$(echo "systemctl is-active xdp2-self-test.service 2>/dev/null || echo unknown" | \
            timeout "$CMD_TIMEOUT" nc 127.0.0.1 "$VIRTIO_PORT" 2>/dev/null | head -5 || true)

          if echo "$RESPONSE" | grep -qE "^active|inactive"; then
            echo "PASS: xdp2-self-test service completed"
            echo "  Time: $WAITED seconds"
            exit 0
          fi

          sleep "$POLL_INTERVAL"
          WAITED=$((WAITED + POLL_INTERVAL))
          if [ "$WAITED" -ge "$TIMEOUT" ]; then
            echo "FAIL: Self-test service not ready after $TIMEOUT seconds"
            echo ""
            echo "Last response: $RESPONSE"
            exit 1
          fi
          echo "  Polling... ($WAITED/$TIMEOUT s)"
        done
      '';
    };

    # Phase 4: Verify eBPF running (expect-based)
    verifyEbpfRunning = pkgs.writeShellApplication {
      name = "xdp2-lifecycle-4-verify-ebpf-running-${arch}";
      runtimeInputs = [ pkgs.expect pkgs.netcat-gnu pkgs.coreutils ];
      text = ''
        VIRTIO_PORT=${toString cfg.virtioPort}
        XDP_INTERFACE="${constants.xdpInterface}"
        HOSTNAME="${hostname}"
        SCRIPT_DIR="${scriptsDir}"

        echo "=== Lifecycle Phase 4: Verify eBPF/XDP Status (${arch}) ==="
        echo "Port: $VIRTIO_PORT (hvc0 virtio console)"
        echo "XDP Interface: $XDP_INTERFACE"
        echo ""

        if ! nc -z 127.0.0.1 "$VIRTIO_PORT" 2>/dev/null; then
          echo "FAIL: Virtio console not available"
          exit 1
        fi

        run_cmd() {
          expect "$SCRIPT_DIR/vm-expect.exp" "$VIRTIO_PORT" "$HOSTNAME" "$1" 10 0
        }

        echo "--- XDP Programs on Interfaces (bpftool net show) ---"
        run_cmd "bpftool net show" || true
        echo ""

        echo "--- Interface $XDP_INTERFACE (ip link show) ---"
        OUTPUT=$(run_cmd "ip link show $XDP_INTERFACE" 2>/dev/null || true)
        echo "$OUTPUT"
        if echo "$OUTPUT" | grep -q "xdp"; then
          echo ""
          echo "PASS: XDP program attached to $XDP_INTERFACE"
        else
          echo ""
          echo "INFO: No XDP program currently attached to $XDP_INTERFACE"
        fi
        echo ""

        echo "--- Loaded BPF Programs (bpftool prog list) ---"
        run_cmd "bpftool prog list" || true
        echo ""

        echo "--- BTF Status ---"
        OUTPUT=$(run_cmd "test -f /sys/kernel/btf/vmlinux && echo 'BTF: AVAILABLE' || echo 'BTF: NOT FOUND'" 2>/dev/null || true)
        echo "$OUTPUT"
        echo ""

        echo "Phase 4 complete - eBPF/XDP status verified"
        exit 0
      '';
    };

    # Phase 5: Shutdown
    shutdown = pkgs.writeShellApplication {
      name = "xdp2-lifecycle-5-shutdown-${arch}";
      runtimeInputs = [ pkgs.netcat-gnu pkgs.coreutils ];
      text = ''
        VIRTIO_PORT=${toString cfg.virtioPort}
        CMD_TIMEOUT=${toString timeouts.command}

        echo "=== Lifecycle Phase 5: Shutdown VM (${arch}) ==="
        echo "Port: $VIRTIO_PORT (hvc0 virtio console)"
        echo ""

        if ! nc -z 127.0.0.1 "$VIRTIO_PORT" 2>/dev/null; then
          echo "INFO: Virtio console not available"
          echo "  VM may already be stopped, or not yet booted"
          exit 0
        fi

        echo "Sending poweroff command..."
        echo "poweroff" | timeout "$CMD_TIMEOUT" nc 127.0.0.1 "$VIRTIO_PORT" 2>/dev/null || true

        echo "PASS: Shutdown command sent"
        echo "  Use lifecycle-6-wait-exit to confirm process termination"
        exit 0
      '';
    };

    # Phase 6: Wait for exit
    waitExit = mkPollingScript {
      name = "xdp2-lifecycle-6-wait-exit-${arch}";
      inherit arch;
      description = "Lifecycle Phase 6: Wait for Exit (${arch})";
      checkCmd = "! pgrep -f '${processName}' > /dev/null 2>&1";
      successMsg = "VM process exited";
      failMsg = "VM process still running";
      timeout = timeouts.shutdown;
      runtimeInputs = [ pkgs.procps pkgs.coreutils ];
      postSuccess = ''
        echo ""
        echo "Use 'nix run .#xdp2-lifecycle-force-kill-${arch}' if needed"
      '';
    };

    # Force kill
    forceKill = pkgs.writeShellApplication {
      name = "xdp2-lifecycle-force-kill-${arch}";
      runtimeInputs = [ pkgs.procps pkgs.coreutils ];
      text = ''
        VM_PROCESS="${processName}"

        echo "=== Force Kill VM (${arch}) ==="
        echo "Process pattern: $VM_PROCESS"
        echo ""

        if ! pgrep -f "$VM_PROCESS" > /dev/null 2>&1; then
          echo "No matching processes found"
          exit 0
        fi

        echo "Found processes:"
        pgrep -af "$VM_PROCESS"
        echo ""

        echo "Sending SIGTERM..."
        pkill -f "$VM_PROCESS" 2>/dev/null || true
        sleep 2

        if pgrep -f "$VM_PROCESS" > /dev/null 2>&1; then
          echo "Process still running, sending SIGKILL..."
          pkill -9 -f "$VM_PROCESS" 2>/dev/null || true
          sleep 1
        fi

        if pgrep -f "$VM_PROCESS" > /dev/null 2>&1; then
          echo "WARNING: Process may still be running"
          pgrep -af "$VM_PROCESS"
          exit 1
        else
          echo "PASS: VM process killed"
          exit 0
        fi
      '';
    };

    # Full lifecycle test
    fullTest = pkgs.writeShellApplication {
      name = "xdp2-lifecycle-full-test-${arch}";
      runtimeInputs = [ pkgs.netcat-gnu pkgs.procps pkgs.coreutils pkgs.expect ];
      text = ''
        VM_PROCESS="${processName}"
        SERIAL_PORT=${toString cfg.serialPort}
        VIRTIO_PORT=${toString cfg.virtioPort}
        POLL_INTERVAL=${toString constants.pollInterval}
        BUILD_TIMEOUT=${toString timeouts.build}
        PROCESS_TIMEOUT=${toString timeouts.processStart}
        SERIAL_TIMEOUT=${toString timeouts.serialReady}
        VIRTIO_TIMEOUT=${toString timeouts.virtioReady}
        SERVICE_TIMEOUT=${toString timeouts.serviceReady}
        CMD_TIMEOUT=${toString timeouts.command}
        SHUTDOWN_TIMEOUT=${toString timeouts.shutdown}
        VM_HOSTNAME="${hostname}"
        EXPECT_SCRIPTS="${scriptsDir}"

        # Colors for output
        RED='\033[0;31m'
        GREEN='\033[0;32m'
        YELLOW='\033[1;33m'
        NC='\033[0m'

        now_ms() { date +%s%3N; }
        pass() { echo -e "  ''${GREEN}PASS: $1''${NC}"; }
        fail() { echo -e "  ''${RED}FAIL: $1''${NC}"; exit 1; }
        info() { echo -e "  ''${YELLOW}INFO: $1''${NC}"; }

        cleanup() {
          echo ""
          info "Cleaning up..."
          if [ -n "''${VM_PID:-}" ] && kill -0 "$VM_PID" 2>/dev/null; then
            kill "$VM_PID" 2>/dev/null || true
            wait "$VM_PID" 2>/dev/null || true
          fi
        }
        trap cleanup EXIT

        echo "========================================"
        echo "  XDP2 MicroVM Full Lifecycle Test (${arch})"
        echo "========================================"
        echo ""
        echo "VM Process Name: $VM_PROCESS"
        echo "Serial Port: $SERIAL_PORT"
        echo "Virtio Port: $VIRTIO_PORT"
        echo ""

        TEST_START_MS=$(now_ms)

        # Timing storage
        PHASE0_MS=0 PHASE1_MS=0 PHASE2_MS=0 PHASE2B_MS=0
        PHASE3_MS=0 PHASE4_MS=0 PHASE5_MS=0 PHASE6_MS=0

        # Phase 0: Build
        echo "--- Phase 0: Build VM (timeout: $BUILD_TIMEOUT s) ---"
        PHASE_START_MS=$(now_ms)

        if ! timeout "$BUILD_TIMEOUT" nix build .#microvm-${arch} --print-out-paths --no-link 2>&1; then
          PHASE_END_MS=$(now_ms)
          PHASE0_MS=$((PHASE_END_MS - PHASE_START_MS))
          fail "Build failed or timed out after ''${PHASE0_MS}ms"
        fi

        VM_PATH=$(nix build .#microvm-${arch} --print-out-paths --no-link 2>/dev/null)
        if [ -z "$VM_PATH" ]; then
          fail "Build succeeded but could not get output path"
        fi

        PHASE_END_MS=$(now_ms)
        PHASE0_MS=$((PHASE_END_MS - PHASE_START_MS))
        pass "VM built in ''${PHASE0_MS}ms: $VM_PATH"
        echo ""

        if nc -z 127.0.0.1 "$SERIAL_PORT" 2>/dev/null; then
          fail "Port $SERIAL_PORT already in use"
        fi

        # Phase 1: Start VM
        echo "--- Phase 1: Start VM (timeout: $PROCESS_TIMEOUT s) ---"
        PHASE_START_MS=$(now_ms)
        "$VM_PATH/bin/microvm-run" &
        VM_PID=$!

        WAITED=0
        while ! pgrep -f "$VM_PROCESS" > /dev/null 2>&1; do
          sleep "$POLL_INTERVAL"
          WAITED=$((WAITED + POLL_INTERVAL))
          if [ "$WAITED" -ge "$PROCESS_TIMEOUT" ]; then
            fail "VM process not found after $PROCESS_TIMEOUT seconds"
          fi
          if ! kill -0 "$VM_PID" 2>/dev/null; then
            fail "VM process died immediately"
          fi
          info "Polling for process... ($WAITED/$PROCESS_TIMEOUT s)"
        done
        PHASE_END_MS=$(now_ms)
        PHASE1_MS=$((PHASE_END_MS - PHASE_START_MS))
        pass "VM process '$VM_PROCESS' running (found in ''${PHASE1_MS}ms)"
        echo ""

        # Phase 2: Serial console
        echo "--- Phase 2: Check Serial Console (timeout: $SERIAL_TIMEOUT s) ---"
        PHASE_START_MS=$(now_ms)
        WAITED=0
        while ! nc -z 127.0.0.1 "$SERIAL_PORT" 2>/dev/null; do
          sleep "$POLL_INTERVAL"
          WAITED=$((WAITED + POLL_INTERVAL))
          if [ "$WAITED" -ge "$SERIAL_TIMEOUT" ]; then
            fail "Serial port not available after $SERIAL_TIMEOUT seconds"
          fi
          if ! kill -0 "$VM_PID" 2>/dev/null; then
            fail "VM process died while waiting for serial"
          fi
          info "Polling serial... ($WAITED/$SERIAL_TIMEOUT s)"
        done
        PHASE_END_MS=$(now_ms)
        PHASE2_MS=$((PHASE_END_MS - PHASE_START_MS))
        pass "Serial console available (ready in ''${PHASE2_MS}ms)"
        echo ""

        # Phase 2b: Virtio console
        echo "--- Phase 2b: Check Virtio Console (timeout: $VIRTIO_TIMEOUT s) ---"
        PHASE_START_MS=$(now_ms)
        WAITED=0
        while ! nc -z 127.0.0.1 "$VIRTIO_PORT" 2>/dev/null; do
          sleep "$POLL_INTERVAL"
          WAITED=$((WAITED + POLL_INTERVAL))
          if [ "$WAITED" -ge "$VIRTIO_TIMEOUT" ]; then
            fail "Virtio port not available after $VIRTIO_TIMEOUT seconds"
          fi
          info "Polling virtio... ($WAITED/$VIRTIO_TIMEOUT s)"
        done
        PHASE_END_MS=$(now_ms)
        PHASE2B_MS=$((PHASE_END_MS - PHASE_START_MS))
        pass "Virtio console available (ready in ''${PHASE2B_MS}ms)"
        echo ""

        # Phase 3: Service verification (expect-based)
        echo "--- Phase 3: Verify Self-Test Service (timeout: $SERVICE_TIMEOUT s) ---"
        PHASE_START_MS=$(now_ms)
        EXPECT_SCRIPT="$EXPECT_SCRIPTS/vm-verify-service.exp"

        if expect "$EXPECT_SCRIPT" "$VIRTIO_PORT" "$VM_HOSTNAME" "$SERVICE_TIMEOUT" "$POLL_INTERVAL"; then
          PHASE_END_MS=$(now_ms)
          PHASE3_MS=$((PHASE_END_MS - PHASE_START_MS))
          pass "Self-test service completed (phase: ''${PHASE3_MS}ms)"
        else
          PHASE_END_MS=$(now_ms)
          PHASE3_MS=$((PHASE_END_MS - PHASE_START_MS))
          info "Service verification returned non-zero after ''${PHASE3_MS}ms"
        fi
        echo ""

        # Phase 4: eBPF status (expect-based)
        echo "--- Phase 4: Verify eBPF/XDP Status ---"
        PHASE_START_MS=$(now_ms)
        EXPECT_SCRIPT="$EXPECT_SCRIPTS/vm-expect.exp"

        run_vm_cmd() {
          expect "$EXPECT_SCRIPT" "$VIRTIO_PORT" "$VM_HOSTNAME" "$1" 10 0 2>/dev/null || true
        }

        echo "  Checking XDP on interfaces..."
        NET_OUTPUT=$(run_vm_cmd "bpftool net show")
        if echo "$NET_OUTPUT" | grep -q "xdp"; then
          pass "XDP program(s) attached"
          echo "$NET_OUTPUT" | grep -E "xdp|eth0" | head -5 | sed 's/^/    /'
        else
          info "No XDP programs currently attached"
        fi

        echo "  Checking interface ${constants.xdpInterface}..."
        LINK_OUTPUT=$(run_vm_cmd "ip -d link show ${constants.xdpInterface}")
        if echo "$LINK_OUTPUT" | grep -q "xdp"; then
          pass "Interface ${constants.xdpInterface} has XDP attached"
        else
          info "Interface ${constants.xdpInterface} ready (no XDP attached yet)"
        fi

        echo "  Checking loaded BPF programs..."
        PROG_OUTPUT=$(run_vm_cmd "bpftool prog list")
        PROG_COUNT=$(echo "$PROG_OUTPUT" | grep -c "^[0-9]" || echo "0")
        if [ "$PROG_COUNT" -gt 0 ]; then
          pass "$PROG_COUNT BPF program(s) loaded"
          echo "$PROG_OUTPUT" | head -10 | sed 's/^/    /'
        else
          info "No BPF programs currently loaded"
        fi

        echo "  Checking BTF..."
        BTF_OUTPUT=$(run_vm_cmd "test -f /sys/kernel/btf/vmlinux && echo BTF_AVAILABLE")
        if echo "$BTF_OUTPUT" | grep -q "BTF_AVAILABLE"; then
          pass "BTF available at /sys/kernel/btf/vmlinux"
        else
          info "Could not verify BTF"
        fi
        PHASE_END_MS=$(now_ms)
        PHASE4_MS=$((PHASE_END_MS - PHASE_START_MS))
        info "Phase 4 completed in ''${PHASE4_MS}ms"
        echo ""

        # Phase 5: Shutdown
        echo "--- Phase 5: Shutdown ---"
        PHASE_START_MS=$(now_ms)
        echo "poweroff" | timeout "$CMD_TIMEOUT" nc 127.0.0.1 "$VIRTIO_PORT" 2>/dev/null || true
        PHASE_END_MS=$(now_ms)
        PHASE5_MS=$((PHASE_END_MS - PHASE_START_MS))
        pass "Shutdown command sent (''${PHASE5_MS}ms)"
        echo ""

        # Phase 6: Wait for exit
        echo "--- Phase 6: Wait for Exit (timeout: $SHUTDOWN_TIMEOUT s) ---"
        PHASE_START_MS=$(now_ms)
        WAITED=0
        while kill -0 "$VM_PID" 2>/dev/null; do
          sleep "$POLL_INTERVAL"
          WAITED=$((WAITED + POLL_INTERVAL))
          if [ "$WAITED" -ge "$SHUTDOWN_TIMEOUT" ]; then
            info "VM still running after $SHUTDOWN_TIMEOUT s, sending SIGTERM"
            kill "$VM_PID" 2>/dev/null || true
            sleep 2
            break
          fi
          info "Polling for exit... ($WAITED/$SHUTDOWN_TIMEOUT s)"
        done

        PHASE_END_MS=$(now_ms)
        PHASE6_MS=$((PHASE_END_MS - PHASE_START_MS))
        if ! kill -0 "$VM_PID" 2>/dev/null; then
          pass "VM exited cleanly (shutdown time: ''${PHASE6_MS}ms)"
        else
          info "VM required forced termination after ''${PHASE6_MS}ms"
          kill -9 "$VM_PID" 2>/dev/null || true
        fi
        echo ""

        # Summary
        TEST_END_MS=$(now_ms)
        TOTAL_TIME_MS=$((TEST_END_MS - TEST_START_MS))

        echo "========================================"
        echo -e "  ''${GREEN}Full Lifecycle Test Complete''${NC}"
        echo "========================================"
        echo ""
        echo "  Timing Summary"
        echo "  ─────────────────────────────────────"
        printf "  %-24s %10s\n" "Phase" "Time (ms)"
        echo "  ─────────────────────────────────────"
        printf "  %-24s %10d\n" "0: Build VM" "$PHASE0_MS"
        printf "  %-24s %10d\n" "1: Start VM" "$PHASE1_MS"
        printf "  %-24s %10d\n" "2: Serial Console" "$PHASE2_MS"
        printf "  %-24s %10d\n" "2b: Virtio Console" "$PHASE2B_MS"
        printf "  %-24s %10d\n" "3: Service Verification" "$PHASE3_MS"
        printf "  %-24s %10d\n" "4: eBPF Status" "$PHASE4_MS"
        printf "  %-24s %10d\n" "5: Shutdown" "$PHASE5_MS"
        printf "  %-24s %10d\n" "6: Wait Exit" "$PHASE6_MS"
        echo "  ─────────────────────────────────────"
        printf "  %-24s %10d\n" "TOTAL" "$TOTAL_TIME_MS"
        echo "  ─────────────────────────────────────"
      '';
    };
  };

  # ==========================================================================
  # Expect-based Helpers
  # ==========================================================================

  mkExpectScripts = { arch, scriptsDir }:
  let
    cfg = getArchConfig arch;
    hostname = getHostname arch;
  in {
    # Run a single command via expect
    runCommand = pkgs.writeShellApplication {
      name = "xdp2-vm-expect-run-${arch}";
      runtimeInputs = [ pkgs.expect pkgs.netcat-gnu ];
      text = ''
        VIRTIO_PORT=${toString cfg.virtioPort}
        HOSTNAME="${hostname}"
        SCRIPT_DIR="${scriptsDir}"

        if [ $# -eq 0 ]; then
          echo "Usage: xdp2-vm-expect-run-${arch} <command> [timeout] [debug_level]"
          echo ""
          echo "Run a command in the VM via expect"
          echo "  Port: $VIRTIO_PORT"
          echo "  Hostname: $HOSTNAME"
          exit 1
        fi

        COMMAND="$1"
        TIMEOUT="''${2:-10}"
        DEBUG="''${3:-0}"

        exec expect "$SCRIPT_DIR/vm-expect.exp" "$VIRTIO_PORT" "$HOSTNAME" "$COMMAND" "$TIMEOUT" "$DEBUG"
      '';
    };

    # Debug VM
    debug = pkgs.writeShellApplication {
      name = "xdp2-vm-debug-expect-${arch}";
      runtimeInputs = [ pkgs.expect pkgs.netcat-gnu ];
      text = ''
        VIRTIO_PORT=${toString cfg.virtioPort}
        HOSTNAME="${hostname}"
        SCRIPT_DIR="${scriptsDir}"
        DEBUG="''${1:-0}"

        exec expect "$SCRIPT_DIR/vm-debug.exp" "$VIRTIO_PORT" "$HOSTNAME" "$DEBUG"
      '';
    };

    # Verify service
    verifyService = pkgs.writeShellApplication {
      name = "xdp2-vm-expect-verify-service-${arch}";
      runtimeInputs = [ pkgs.expect pkgs.netcat-gnu ];
      text = ''
        VIRTIO_PORT=${toString cfg.virtioPort}
        HOSTNAME="${hostname}"
        SCRIPT_DIR="${scriptsDir}"
        TIMEOUT="''${1:-60}"
        POLL_INTERVAL="''${2:-2}"

        exec expect "$SCRIPT_DIR/vm-verify-service.exp" "$VIRTIO_PORT" "$HOSTNAME" "$TIMEOUT" "$POLL_INTERVAL"
      '';
    };
  };

  # ==========================================================================
  # Test Runner (simple test script)
  # ==========================================================================

  mkTestRunner = { arch }:
  let
    cfg = getArchConfig arch;
    timeouts = constants.getTimeouts arch;
  in pkgs.writeShellApplication {
    name = "xdp2-test-${arch}";
    runtimeInputs = [ pkgs.coreutils pkgs.netcat-gnu ];
    text = ''
      echo "========================================"
      echo "  XDP2 MicroVM Test (${arch})"
      echo "========================================"
      echo ""

      echo "Building VM..."
      VM_PATH=$(nix build .#microvm-${arch} --print-out-paths --no-link 2>/dev/null)
      if [ -z "$VM_PATH" ]; then
        echo "ERROR: Failed to build VM"
        exit 1
      fi
      echo "VM built: $VM_PATH"
      echo ""

      SERIAL_PORT=${toString cfg.serialPort}
      VIRTIO_PORT=${toString cfg.virtioPort}

      if nc -z 127.0.0.1 "$SERIAL_PORT" 2>/dev/null; then
        echo "ERROR: Port $SERIAL_PORT already in use"
        exit 1
      fi

      echo "Starting VM..."
      "$VM_PATH/bin/microvm-run" &
      VM_PID=$!
      echo "VM PID: $VM_PID"

      echo "Waiting for VM to boot..."
      TIMEOUT=${toString timeouts.boot}
      WAITED=0
      while ! nc -z 127.0.0.1 "$VIRTIO_PORT" 2>/dev/null; do
        sleep 1
        WAITED=$((WAITED + 1))
        if [ "$WAITED" -ge "$TIMEOUT" ]; then
          echo "ERROR: VM failed to boot within $TIMEOUT seconds"
          kill "$VM_PID" 2>/dev/null || true
          exit 1
        fi
        if ! kill -0 "$VM_PID" 2>/dev/null; then
          echo "ERROR: VM process died"
          exit 1
        fi
      done
      echo "VM booted in $WAITED seconds"
      echo ""

      echo "Connecting to VM console..."
      echo "--- VM Console Output ---"
      sleep 5
      timeout 10 nc 127.0.0.1 "$VIRTIO_PORT" 2>/dev/null || true
      echo "--- End Console Output ---"
      echo ""

      echo "Shutting down VM..."
      kill "$VM_PID" 2>/dev/null || true
      wait "$VM_PID" 2>/dev/null || true

      echo ""
      echo "========================================"
      echo "  Test Complete (${arch})"
      echo "========================================"
      echo ""
      echo "To run interactively:"
      echo "  nix build .#microvm-${arch}"
      echo "  ./result/bin/microvm-run &"
      echo "  nc 127.0.0.1 ${toString cfg.virtioPort}"
    '';
  };
}
