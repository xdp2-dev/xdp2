# nix/devshell.nix
#
# Development shell configuration for XDP2
#
# This module creates the development shell with all necessary
# packages, environment variables, and shell functions.
#
# Usage in flake.nix:
#   devshell = import ./nix/devshell.nix {
#     inherit pkgs lib llvmConfig packages compilerConfig envVars;
#   };
#   devShells.default = devshell;
#

{ pkgs
, lib
, llvmConfig
, packages
, compilerConfig
, envVars
}:

let
  llvmPackages = llvmConfig.llvmPackages;

  # Shared configuration
  sharedConfig = {
    # Compiler info for display
    compilerInfo = "GCC";

    # Configurable threshold for stale config warnings
    configAgeWarningDays = 14;
  };

  # Shellcheck function registry - list of all bash functions to validate
  # IMPORTANT: When adding or removing bash functions, update this list
  shellcheckFunctionRegistry = [
    "smart-configure"
    "build-cppfront"
    "check-cppfront-age"
    "build-xdp2-compiler"
    "build-xdp2"
    "build-all"
    "clean-all"
    "check-platform-compatibility"
    "detect-repository-root"
    "setup-locale-support"
    "xdp2-help"
    "navigate-to-repo-root"
    "navigate-to-component"
    "add-to-path"
    "clean-cppfront"
    "clean-xdp2-compiler"
    "clean-xdp2"
    "apply-nix-patches"
    "revert-nix-patches"
    "check-nix-patches"
  ];

  # Import shell function modules
  navigationFns = import ./shell-functions/navigation.nix { };
  cleanFns = import ./shell-functions/clean.nix { };
  buildFns = import ./shell-functions/build.nix { };
  configureFns = import ./shell-functions/configure.nix {
    configAgeWarningDays = sharedConfig.configAgeWarningDays;
  };
  validationFns = import ./shell-functions/validation.nix {
    inherit lib shellcheckFunctionRegistry;
  };
  asciiArtFn = import ./shell-functions/ascii-art.nix { };

  # Shell snippets
  disable-exit-fn = ''
    disable-exit() {
      set +e
    }
  '';

  shell-aliases = ''
    alias xdp2-src='cd src'
    alias xdp2-samples='cd samples'
    alias xdp2-docs='cd documentation'
    alias xdp2-cppfront='cd thirdparty/cppfront'
  '';

  colored-prompt = ''
    export PS1="\[\033[0;32m\][XDP2-${sharedConfig.compilerInfo}] \[\033[01;34m\][\u@\h:\w]\$ \[\033[0m\]"
  '';

  minimal-shell-entry = ''
    echo "🚀 === XDP2 Development Shell ==="
    echo "📦 Compiler: ${sharedConfig.compilerInfo}"
    echo "🔧 GCC and Clang are available in the environment"
    echo "🐛 Debugging tools: gdb, valgrind, strace, ltrace"
    echo "🎯 Ready to develop! 'xdp2-help' for help"
  '';

  # Debug snippets - check XDP2_NIX_DEBUG shell variable at runtime
  # Usage: XDP2_NIX_DEBUG=7 nix develop
  debug-compiler-selection = ''
    if [ "''${XDP2_NIX_DEBUG:-0}" -gt 4 ]; then
      echo "=== COMPILER SELECTION ==="
      echo "Using compiler: ${sharedConfig.compilerInfo}"
      echo "HOST_CC: $HOST_CC"
      echo "HOST_CXX: $HOST_CXX"
      $HOST_CC --version
      $HOST_CXX --version
      echo "=== End compiler selection ==="
    fi
  '';

  debug-environment-vars = ''
    if [ "''${XDP2_NIX_DEBUG:-0}" -gt 5 ]; then
      echo "=== Environment Variables ==="
      env
      echo "=== End Environment Variables ==="
    fi
  '';

  # Nix patch management function
  applyNixPatchesFn = ''
    apply-nix-patches() {
      local repo_root
      repo_root=$(git rev-parse --show-toplevel 2>/dev/null || echo ".")
      local patches_dir="$repo_root/nix/patches"
      local applied_marker="$repo_root/.nix-patches-applied"

      if [ ! -d "$patches_dir" ]; then
        echo "No patches directory found at $patches_dir"
        return 1
      fi

      if [ -f "$applied_marker" ]; then
        echo "Nix patches already applied. Use 'revert-nix-patches' to undo."
        return 0
      fi

      echo "Applying Nix-specific patches..."
      cd "$repo_root" || return 1

      for patch in "$patches_dir"/*.patch; do
        if [ -f "$patch" ]; then
          echo "  Applying: $(basename "$patch")"
          if ! patch -p0 --forward --silent < "$patch" 2>/dev/null; then
            echo "    (already applied or failed)"
          fi
        fi
      done

      touch "$applied_marker"
      echo "Patches applied. Run 'revert-nix-patches' to undo."
    }

    revert-nix-patches() {
      local repo_root
      repo_root=$(git rev-parse --show-toplevel 2>/dev/null || echo ".")
      local patches_dir="$repo_root/nix/patches"
      local applied_marker="$repo_root/.nix-patches-applied"

      if [ ! -f "$applied_marker" ]; then
        echo "No patches to revert (marker not found)"
        return 0
      fi

      echo "Reverting Nix-specific patches..."
      cd "$repo_root" || return 1

      # Apply patches in reverse order
      for patch in $(ls -r "$patches_dir"/*.patch 2>/dev/null); do
        if [ -f "$patch" ]; then
          echo "  Reverting: $(basename "$patch")"
          patch -p0 --reverse --silent < "$patch" 2>/dev/null || true
        fi
      done

      rm -f "$applied_marker"
      echo "Patches reverted."
    }

    check-nix-patches() {
      local repo_root
      repo_root=$(git rev-parse --show-toplevel 2>/dev/null || echo ".")
      local applied_marker="$repo_root/.nix-patches-applied"

      if [ -f "$applied_marker" ]; then
        echo "Nix patches are APPLIED"
      else
        echo "Nix patches are NOT applied"
        echo "Run 'apply-nix-patches' to apply them for xdp2-compiler development"
      fi
    }
  '';

  # Combined build functions (ordered to avoid SC2218 - functions called before definition)
  build-functions = ''
    # Navigation functions (from nix/shell-functions/navigation.nix)
    ${navigationFns}

    # Clean functions (from nix/shell-functions/clean.nix)
    ${cleanFns}

    # Build functions (from nix/shell-functions/build.nix)
    ${buildFns}

    # Validation and help functions (from nix/shell-functions/validation.nix)
    ${validationFns}

    # Nix patch management functions
    ${applyNixPatchesFn}
  '';

in
pkgs.mkShell {
  packages = packages.allPackages;

  shellHook = ''
    ${envVars}

    ${build-functions}

    check-platform-compatibility
    detect-repository-root
    setup-locale-support

    ${debug-compiler-selection}
    ${debug-environment-vars}

    ${asciiArtFn}

    ${configureFns}
    smart-configure

    ${shell-aliases}

    ${colored-prompt}

    ${disable-exit-fn}

    ${minimal-shell-entry}
  '';
}
