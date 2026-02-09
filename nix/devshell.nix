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
