# nix/tests/xdp-build.nix
#
# Build verification test for XDP-only samples
#
# STATUS: BLOCKED
# XDP build tests are currently blocked on architectural issues.
# See documentation/nix/xdp-bpf-compatibility-defect.md for details.
#
# The following samples would be tested once issues are resolved:
# - flow_tracker_simple
# - flow_tracker_tlvs
# - flow_tracker_tmpl
#
# Usage:
#   nix build .#tests.xdp-build
#   ./result/bin/xdp2-test-xdp-build
#

{ pkgs, xdp2 }:

pkgs.writeShellApplication {
  name = "xdp2-test-xdp-build";

  runtimeInputs = [
    pkgs.coreutils
  ];

  text = ''
    echo "=== XDP2 XDP Build Verification Test ==="
    echo ""
    echo "STATUS: BLOCKED"
    echo ""
    echo "XDP build tests are currently blocked pending architectural fixes:"
    echo ""
    echo "1. BPF Stack Limitations"
    echo "   - XDP2_METADATA_TEMP_* macros generate code exceeding BPF stack"
    echo "   - Error: 'stack arguments are not supported'"
    echo ""
    echo "2. Template API Mismatch"
    echo "   - src/templates/xdp2/xdp_def.template.c uses old ctrl.hdr.* API"
    echo "   - Error: 'no member named hdr in struct xdp2_ctrl_data'"
    echo ""
    echo "Affected samples:"
    echo "   - flow_tracker_simple"
    echo "   - flow_tracker_tlvs"
    echo "   - flow_tracker_tmpl"
    echo ""
    echo "See: documentation/nix/xdp-bpf-compatibility-defect.md"
    echo ""
    echo "==================================="
    echo "        TEST STATUS: SKIPPED"
    echo "==================================="
    exit 0
  '';
}
