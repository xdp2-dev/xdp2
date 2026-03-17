# nix/shell-functions/clean.nix
#
# Clean shell functions for XDP2
#
# Functions:
# - clean-cppfront: Clean cppfront build artifacts
# - clean-xdp2-compiler: Clean xdp2-compiler build artifacts
# - clean-xdp2: Clean xdp2 build artifacts
# - clean-all: Clean all build artifacts
#
# Note: These functions depend on navigation functions from navigation.nix
#
# Usage in flake.nix:
#   cleanFns = import ./nix/shell-functions/clean.nix { };
#

{ }:

''
  # Clean cppfront build artifacts
  clean-cppfront() {
    if [ -n "$XDP2_NIX_DEBUG" ]; then
      local debug_level=$XDP2_NIX_DEBUG
    else
      local debug_level=0
    fi

    # Navigate to repository root first
    navigate-to-repo-root || return 1

    # Navigate to cppfront directory
    navigate-to-component "thirdparty/cppfront" || return 1

    # Debug output for clean command
    if [ "$debug_level" -gt 3 ]; then
      echo "[DEBUG] About to run: rm -f cppfront-compiler"
    fi
    rm -f cppfront-compiler  # Remove the binary directly since Makefile has no clean target

    # Return to repository root
    navigate-to-repo-root || return 1
  }

  # Clean xdp2-compiler build artifacts
  clean-xdp2-compiler() {
    if [ -n "$XDP2_NIX_DEBUG" ]; then
      local debug_level=$XDP2_NIX_DEBUG
    else
      local debug_level=0
    fi

    # Navigate to repository root first
    navigate-to-repo-root || return 1

    # Navigate to xdp2-compiler directory
    navigate-to-component "src/tools/compiler" || return 1

    # Debug output for clean command
    if [ "$debug_level" -gt 3 ]; then
      echo "[DEBUG] About to run: make clean"
    fi
    make clean || true  # Don't fail if clean fails

    # Return to repository root
    navigate-to-repo-root || return 1
  }

  # Clean xdp2 build artifacts
  clean-xdp2() {
    if [ -n "$XDP2_NIX_DEBUG" ]; then
      local debug_level=$XDP2_NIX_DEBUG
    else
      local debug_level=0
    fi

    # Navigate to repository root first
    navigate-to-repo-root || return 1

    # Navigate to src directory
    navigate-to-component "src" || return 1

    # Debug output for clean command
    if [ "$debug_level" -gt 3 ]; then
      echo "[DEBUG] About to run: make clean"
    fi
    make clean || true  # Don't fail if clean fails

    # Return to repository root
    navigate-to-repo-root || return 1
  }

  # Clean all build artifacts
  clean-all() {
    echo "Cleaning all build artifacts..."

    # Clean each component using centralized clean functions
    clean-cppfront
    clean-xdp2-compiler
    clean-xdp2

    echo "✓ All build artifacts cleaned"
  }
''
