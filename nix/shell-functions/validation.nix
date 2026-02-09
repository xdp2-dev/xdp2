# nix/shell-functions/validation.nix
#
# Validation and help shell functions for XDP2
#
# Functions:
# - check-platform-compatibility: Check if running on Linux
# - setup-locale-support: Configure locale settings
# - run-shellcheck: Validate all shell functions with shellcheck
# - xdp2-help: Display help information
#
# Usage in flake.nix:
#   validationFns = import ./nix/shell-functions/validation.nix {
#     inherit lib;
#     shellcheckFunctionRegistry = [ "func1" "func2" ... ];
#   };
#

{ lib, shellcheckFunctionRegistry ? [] }:

let
  # Generate shellcheck validation function
  totalFunctions = builtins.length shellcheckFunctionRegistry;

  # Generate individual function checks
  functionChecks = lib.concatStringsSep "\n" (map (name: ''
    echo "Checking ${name}..."
    if declare -f "${name}" >/dev/null 2>&1; then
      # Create temporary script with function definition
      # TODO use mktemp and trap 'rm -f "$temp_script"' EXIT
      local temp_script="/tmp/validate_${name}.sh"
      declare -f "${name}" > "$temp_script"
      echo "#!/bin/bash" > "$temp_script.tmp"
      cat "$temp_script" >> "$temp_script.tmp"
      mv "$temp_script.tmp" "$temp_script"

      # Run shellcheck on the function
      if shellcheck -s bash "$temp_script" 2>/dev/null; then
        echo "✓ ${name} passed shellcheck validation"
        passed_functions=$((passed_functions + 1))
      else
        echo "✗ ${name} failed shellcheck validation:"
        shellcheck -s bash "$temp_script"
        failed_functions+=("${name}")
      fi
      rm -f "$temp_script"
    else
      echo "✗ ${name} not found"
      failed_functions+=("${name}")
    fi
    echo ""
  '') shellcheckFunctionRegistry);

  # Generate failed functions reporting
  failedFunctionsReporting = lib.concatStringsSep "\n" (map (name: ''
    if [[ "$${failed_functions[*]}" == *"${name}"* ]]; then
      echo "  - ${name}"
    fi
  '') shellcheckFunctionRegistry);

  # Bash variable expansion helpers for setup-locale-support
  bashVarExpansion = "$";
  bashDefaultSyntax = "{LANG:-C.UTF-8}";
  bashDefaultSyntaxLC = "{LC_ALL:-C.UTF-8}";

in
''
  # Check platform compatibility (Linux only)
  check-platform-compatibility() {
    if [ "$(uname)" != "Linux" ]; then
      echo "⚠️  PLATFORM COMPATIBILITY NOTICE
==================================

🍎 You are running on $(uname) (not Linux)

The XDP2 development environment includes Linux-specific packages
like libbpf that are not available on $(uname) systems.

📋 Available platforms:
   ✅ Linux (x86_64-linux, aarch64-linux, etc.)
   ❌ macOS (x86_64-darwin, aarch64-darwin)
   ❌ Other Unix systems

Exiting development shell..."
      exit 1
    fi
  }

  # Setup locale support for cross-distribution compatibility
  setup-locale-support() {
    # Only set locale if user hasn't already configured it
    if [ -z "$LANG" ] || [ -z "$LC_ALL" ]; then
      # Try to use system default, fallback to C.UTF-8
      export LANG=${bashVarExpansion}${bashDefaultSyntax}
      export LC_ALL=${bashVarExpansion}${bashDefaultSyntaxLC}
    fi

    # Verify locale is available (only if locale command exists)
    if command -v locale >/dev/null 2>&1; then
      if ! locale -a 2>/dev/null | grep -q "$LANG"; then
        # Fallback to C.UTF-8 if user's locale is not available
        export LANG=C.UTF-8
        export LC_ALL=C.UTF-8
      fi
    fi
  }

  # Run shellcheck validation on all registered shell functions
  run-shellcheck() {
    if [ -n "$XDP2_NIX_DEBUG" ]; then
      local debug_level=$XDP2_NIX_DEBUG
    else
      local debug_level=0
    fi

    echo "Running shellcheck validation on shell functions..."

    local failed_functions=()
    local total_functions=${toString totalFunctions}
    local passed_functions=0

    # Pre-generated function checks from Nix
    ${functionChecks}

    # Report results
    echo "=== Shellcheck Validation Complete ==="
    echo "Total functions: $total_functions"
    echo "Passed: $passed_functions"
    echo "Failed: $((total_functions - passed_functions))"

    if [ $((total_functions - passed_functions)) -eq 0 ]; then
      echo "✓ All functions passed shellcheck validation"
      return 0
    else
      echo "✗ Some functions failed validation:"
      # Pre-generated failed functions reporting from Nix
      ${failedFunctionsReporting}
      return 1
    fi
  }

  # Display help information for XDP2 development shell
  xdp2-help() {
    echo "🚀 === XDP2 Development Shell Help ===

📦 Compiler: GCC
🔧 GCC and Clang are available in the environment.
🐛 Debugging tools: gdb, valgrind, strace, ltrace

🔍 DEBUGGING:
  XDP2_NIX_DEBUG=0         - No extra debug. Default
  XDP2_NIX_DEBUG=3         - Basic debug
  XDP2_NIX_DEBUG=5         - Show compiler selection and config.mk
  XDP2_NIX_DEBUG=7         - Show all debug info

🔧 BUILD COMMANDS:
  build-cppfront           - Build cppfront compiler
  build-xdp2-compiler      - Build xdp2 compiler
  build-xdp2               - Build main XDP2 project
  build-all                - Build all components

🧹 CLEAN COMMANDS:
  clean-cppfront           - Clean cppfront build artifacts
  clean-xdp2-compiler      - Clean xdp2-compiler build artifacts
  clean-xdp2               - Clean xdp2 build artifacts
  clean-all                - Clean all build artifacts

🔍 VALIDATION:
  run-shellcheck           - Validate all shell functions

📁 PROJECT STRUCTURE:
  • src/                   - Main source code
  • tools/                 - Build tools and utilities
  • thirdparty/            - Third-party dependencies
  • samples/               - Example code and parsers
  • documentation/         - Project documentation

🎯 Ready to develop! 'xdp2-help' for help"
  }
''
