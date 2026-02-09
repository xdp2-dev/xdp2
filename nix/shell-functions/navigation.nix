# nix/shell-functions/navigation.nix
#
# Navigation shell functions for XDP2
#
# Functions:
# - detect-repository-root: Detect and export XDP2_REPO_ROOT
# - navigate-to-repo-root: Change to repository root directory
# - navigate-to-component: Change to a component subdirectory
# - add-to-path: Add a directory to PATH if not already present
#
# Usage in flake.nix:
#   navigationFns = import ./nix/shell-functions/navigation.nix { };
#

{ }:

''
  # Detect and export the repository root directory
  detect-repository-root() {
    XDP2_REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || pwd)
    export XDP2_REPO_ROOT

    if [ ! -d "$XDP2_REPO_ROOT" ]; then
      echo "⚠ WARNING: Could not detect valid repository root"
      XDP2_REPO_ROOT="$PWD"
    else
      echo "📁 Repository root: $XDP2_REPO_ROOT"
    fi
  }

  # Navigate to the repository root directory
  navigate-to-repo-root() {
    if [ -n "$XDP2_REPO_ROOT" ]; then
      cd "$XDP2_REPO_ROOT" || return 1
    else
      echo "✗ ERROR: XDP2_REPO_ROOT not set"
      return 1
    fi
  }

  # Navigate to a component subdirectory
  navigate-to-component() {
    local component="$1"
    local target_dir="$XDP2_REPO_ROOT/$component"

    if [ ! -d "$target_dir" ]; then
      echo "✗ ERROR: Component directory not found: $target_dir"
      return 1
    fi

    cd "$target_dir" || return 1
  }

  # Add path to PATH environment variable if not already present
  add-to-path() {
    local path_to_add="$1"

    if [ -n "$XDP2_NIX_DEBUG" ]; then
      local debug_level=$XDP2_NIX_DEBUG
    else
      local debug_level=0
    fi

    # Check if path is already in PATH
    if [[ ":$PATH:" == *":$path_to_add:"* ]]; then
      if [ "$debug_level" -gt 3 ]; then
        echo "[DEBUG] Path already in PATH: $path_to_add"
      fi
      return 0
    fi

    # Add path to beginning of PATH
    if [ "$debug_level" -gt 3 ]; then
      echo "[DEBUG] Adding to PATH: $path_to_add"
      echo "[DEBUG] PATH before: $PATH"
    fi

    export PATH="$path_to_add:$PATH"

    if [ "$debug_level" -gt 3 ]; then
      echo "[DEBUG] PATH after: $PATH"
    fi
  }
''
