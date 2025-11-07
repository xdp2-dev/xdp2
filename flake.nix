#
# flake.nix for XDP2 - Development Shell Only
#
# WARNING - THIS FLAKE IS CURRENTLY BROKEN (2025/11/06) FIXES COMING SOON
#
# This flake.nix provides a fast development environment for the XDP2 project
#
# To enter the development environment:
# nix develop

# If flakes are not enabled, use the following command to enter the development environment:
# nix --extra-experimental-features 'nix-command flakes' develop .
#
# To enable flakes, you may need to enable them in your system configuration:
# test -d /etc/nix || sudo mkdir /etc/nix
# echo 'experimental-features = nix-command flakes' | sudo tee -a /etc/nix/nix.conf
#
# Debugging:
# XDP2_NIX_DEBUG=7 nix develop --verbose --print-build-logs
#
# Not really sure what the difference between the two is, but the second one is faster
# nix --extra-experimental-features 'nix-command flakes' --option eval-cache false develop
# nix --extra-experimental-features 'nix-command flakes' develop --no-write-lock-file
#
# nix --extra-experimental-features 'nix-command flakes' print-dev-env --json
#
# Recommended term
# export TERM=xterm-256color
#
{
  description = "XDP2 development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        lib = nixpkgs.lib;
        llvmP = pkgs.llvmPackages_20;

        # Create a Python environment with scapy
        pythonWithScapy = pkgs.python3.withPackages (ps: [ ps.scapy ]);


        sharedConfig = {

          # Debug configuration
          nixDebug = let
            envDebug = builtins.getEnv "XDP2_NIX_DEBUG";
          in
            if envDebug == "" then 0 else builtins.fromJSON envDebug;

          # GCC-only configuration.  These variables could be used to select clang
          useGCC = true;
          selectedCCPkgs = pkgs.gcc;
          selectedCXXPkgs = pkgs.gcc;
          selectedCCBin = "gcc";
          selectedCXXBin = "g++";
          compilerInfo = "GCC";

          configAgeWarningDays = 14;  # Configurable threshold for stale config warnings


          # https://nixos.wiki/wiki/C#Hardening_flags
          # hardeningDisable = [ "fortify" "fortify3" "stackprotector" "strictoverflow" ];
          # Disable all hardening flags for now, but might restore some later
          hardeningDisable = [ "all" ];

          # Library packages
          corePackages = with pkgs; [
            # Build tools
            gnumake pkg-config bison flex
            # Core utilities
            bash coreutils gnused gawk gnutar xz git
            # Libraries
            boost
            libpcap
            libelf
            libbpf
            pythonWithScapy
            # Development tools
            graphviz
            bpftools
            # Compilers
            gcc
            llvmP.clang
            llvmP.llvm.dev
            llvmP.clang-unwrapped
            llvmP.libclang
            llvmP.lld
            # Debugging tools
            glibc_multi.bin
            gdb
            valgrind
            strace
            ltrace
            # Code quality
            shellcheck
            # ASCII art generator for logo display
            jp2a
            # Locale support for cross-distribution compatibility
            glibcLocales
          ];

          buildInputs = with pkgs; [
            boost
            libpcap
            libelf
            libbpf
            pythonWithScapy
            llvmP.llvm
            llvmP.llvm.dev
            llvmP.clang-unwrapped
            llvmP.libclang
            llvmP.lld
          ];

          nativeBuildInputs = [
            pkgs.pkg-config
            llvmP.clang
            llvmP.llvm.dev
          ];
        };

        # Create a wrapper for llvm-config to include clang paths (for libclang)
        llvm-config-wrapped = pkgs.runCommand "llvm-config-wrapped" { } ''
          mkdir -p $out/bin
          cat > $out/bin/llvm-config <<EOF
          #!${pkgs.bash}/bin/bash
          if [[ "\$1" == "--includedir" ]]; then
            echo "${llvmP.clang-unwrapped.dev}/include"
          elif [[ "\$1" == "--libdir" ]]; then
            echo "${lib.getLib llvmP.clang-unwrapped}/lib"
          else
            ${llvmP.llvm.dev}/bin/llvm-config "\$@"
          fi
          EOF
          chmod +x $out/bin/llvm-config
        '';

        # Environment variables for development shell
        sharedEnvVars = ''
          # Compiler settings (GCC-only)
          export CC=${sharedConfig.selectedCCPkgs}/bin/${sharedConfig.selectedCCBin}
          export CXX=${sharedConfig.selectedCXXPkgs}/bin/${sharedConfig.selectedCXXBin}
          export HOST_CC=${sharedConfig.selectedCCPkgs}/bin/${sharedConfig.selectedCCBin}
          export HOST_CXX=${sharedConfig.selectedCXXPkgs}/bin/${sharedConfig.selectedCXXBin}

          # Clang environment variables for xdp2-compiler
          export XDP2_CLANG_VERSION="$(${llvmP.llvm.dev}/bin/llvm-config --version)"
          export XDP2_C_INCLUDE_PATH="${llvmP.clang-unwrapped.dev}/include/clang"

          # NOTE: We intentionally do NOT set XDP2_CLANG_RESOURCE_PATH here.
          # Let clang auto-detect its resource directory, which works correctly in Nix
          # via the clang-wrapper that sets up the resource-root symlink.
          # export XDP2_CLANG_RESOURCE_PATH="${llvmP.clang-unwrapped.dev}/include/clang"

          # Python environment
          export CFLAGS_PYTHON="$(pkg-config --cflags python3-embed)"
          export LDFLAGS_PYTHON="$(pkg-config --libs python3-embed)"
          export PYTHON_VER=3
          export PYTHONPATH="${pkgs.python3}/lib/python3.13/site-packages:$PYTHONPATH"

          # LLVM/Clang settings
          export HOST_LLVM_CONFIG="${llvm-config-wrapped}/bin/llvm-config"
          export LLVM_LIBS="-L${llvmP.llvm}/lib"
          export CLANG_LIBS="-lclang -lLLVM -lclang-cpp"

          # libclang configuration
          export LIBCLANG_PATH=${llvmP.libclang.lib}/lib
          export LD_LIBRARY_PATH=${llvmP.libclang.lib}/lib:$LD_LIBRARY_PATH

          # Boost libraries
          export BOOST_LIBS="-lboost_wave -lboost_thread -lboost_filesystem -lboost_system -lboost_program_options"

          # Other libraries
          export LIBS="-lpthread -ldl -lutil"
          export PATH_ARG=""

          # Build configuration
          export PKG_CONFIG_PATH=${pkgs.lib.makeSearchPath "lib/pkgconfig" sharedConfig.corePackages}
          export XDP2_COMPILER_DEBUG=1

          # Configuration management
          export CONFIG_AGE_WARNING_DAYS=${toString sharedConfig.configAgeWarningDays}
        '';

        # Smart configure script execution with age checking
        # This simply includes a check to see if the config.mk file exists, and
        # it generates a warning if the file is older than the threshold
        smart-configure = ''
          smart-configure() {
            local config_file="./src/config.mk"
            local warning_days=${toString sharedConfig.configAgeWarningDays}

            if [ -f "$config_file" ]; then
              echo "✓ config.mk found, skipping configure step"

              # Check age of config.mk
              local file_time
              file_time=$(stat -c %Y "$config_file")
              local current_time
              current_time=$(date +%s)
              local age_days=$(( (current_time - file_time) / 86400 ))

              if [ "$age_days" -gt "$warning_days" ]; then
                echo "⚠️  WARNING: config.mk is $age_days days old (threshold: $warning_days days)"
                echo "   Consider running 'configure' manually if you've made changes to:"
                echo "   • Build configuration"
                echo "   • Compiler settings"
                echo "   • Library paths"
                echo "   • Platform-specific settings"
                echo ""
              else
                echo "✓ config.mk is up to date ($age_days days old)"
              fi
            else
              echo "config.mk not found, running configure script..."
              cd src || return 1
            rm -f config.mk
              ./configure.sh --build-opt-parser

              # Apply PATH_ARG fix for Nix environment
            if grep -q 'PATH_ARG="--with-path=' config.mk; then
                echo "Applying PATH_ARG fix for Nix environment..."
              sed -i 's|PATH_ARG="--with-path=.*"|PATH_ARG=""|' config.mk
            fi
            echo "PATH_ARG in config.mk: $(grep '^PATH_ARG=' config.mk)"

              cd .. || return 1
              echo "✓ config.mk generated successfully"
            fi
          }
        '';

        # Individual build function definitions
        build-cppfront-fn = ''
          build-cppfront() {
            if [ -n "$XDP2_NIX_DEBUG" ]; then
              local debug_level=$XDP2_NIX_DEBUG
            else
              local debug_level=0
            fi
            local start_time=""
            local end_time=""

            if [ "$debug_level" -gt 3 ]; then
              start_time=$(date +%s)
              echo "[DEBUG] build-cppfront started at $(date)"
            fi

            # Level 1: Function start
            if [ "$debug_level" -ge 1 ]; then
              echo "[DEBUG] Starting build-cppfront function"
            fi

            # Clean
            if [ "$debug_level" -ge 2 ]; then
              echo "[DEBUG] Cleaning cppfront build directory"
            fi
            echo "Cleaning and building cppfront-compiler..."

            # Navigate to repository root first
            navigate-to-repo-root || return 1

            # Clean previous build artifacts (before navigating to component)
            clean-cppfront

            # Navigate to cppfront directory
            navigate-to-component "thirdparty/cppfront" || return 1

            # Apply essential header fix for cppfront
            if [ "$debug_level" -ge 3 ]; then
              echo "[DEBUG] Applying cppfront header fix"
              printf "sed -i '1i#include <functional>\\n#include <unordered_map>\\n' include/cpp2util.h\n"
            fi
            sed -i '1i#include <functional>\n#include <unordered_map>\n' include/cpp2util.h

            # Level 3: Build step details
            if [ "$debug_level" -ge 3 ]; then
              echo "[DEBUG] Building cppfront-compiler with make"
            fi

            # Build cppfront with error checking
            if HOST_CXX="$CXX" HOST_CC="$CC" make -j"$NIX_BUILD_CORES"; then
              echo "✓ cppfront make completed successfully"
            else
              echo "✗ ERROR: cppfront make failed"
              return 1
            fi

            # Return to repository root
            navigate-to-repo-root || return 1

            # Add to the PATH
            add-to-path "$PWD/thirdparty/cppfront"

            # Level 2: Validation step
            if [ "$debug_level" -ge 2 ]; then
              echo "[DEBUG] Validating cppfront-compiler binary"
            fi

            # Validate binary was created
            if [ -x "./thirdparty/cppfront/cppfront-compiler" ]; then
              echo "✓ cppfront-compiler binary created and executable"

              # Test the binary runs correctly
              echo "Testing cppfront-compiler..."
              set +e  # Temporarily disable exit on error

              # Debug output for validation command
              if [ "$debug_level" -gt 3 ]; then
                echo "[DEBUG] About to run: ./thirdparty/cppfront/cppfront-compiler -version"
              fi
              ./thirdparty/cppfront/cppfront-compiler -version
              test_exit_code=$?
              set -e  # Re-enable exit on error

              if [ "$test_exit_code" -eq 0 ] || [ "$test_exit_code" -eq 1 ]; then
                echo "✓ cppfront-compiler runs correctly (exit code: $test_exit_code)"
              else
                echo "⚠ WARNING: cppfront-compiler returned unexpected exit code: $test_exit_code"
                echo "But binary exists and is executable, continuing..."
              fi
            else
              echo "✗ ERROR: cppfront-compiler binary not found or not executable"
              return 1
            fi

            # End timing for debug levels > 3
            if [ "$debug_level" -gt 3 ]; then
              end_time=$(date +%s)
              local duration=$((end_time - start_time))
              echo "[DEBUG] build-cppfront completed in $duration seconds"
            fi

            echo "cppfront-compiler built and validated successfully ( ./thirdparty/cppfront/cppfront-compiler )"
          }
        '';

        check-cppfront-age-fn = ''
          check-cppfront-age() {
            if [ -n "$XDP2_NIX_DEBUG" ]; then
              local debug_level=$XDP2_NIX_DEBUG
            else
              local debug_level=0
            fi
            local start_time=""
            local end_time=""

            # Start timing for debug levels > 3
            if [ "$debug_level" -gt 3 ]; then
              start_time=$(date +%s)
              echo "[DEBUG] check-cppfront-age started at $(date)"
            fi

            # Level 1: Function start
            if [ "$debug_level" -ge 1 ]; then
              echo "[DEBUG] Starting check-cppfront-age function"
            fi

            local cppfront_binary="thirdparty/cppfront/cppfront-compiler"

            # Level 2: File check
            if [ "$debug_level" -ge 2 ]; then
              echo "[DEBUG] Checking cppfront binary: $cppfront_binary"
            fi

            if [ -f "$cppfront_binary" ]; then
              local file_time
              file_time=$(stat -c %Y "$cppfront_binary")
              local current_time
              current_time=$(date +%s)
              local age_days=$(( (current_time - file_time) / 86400 ))

              # Level 3: Age calculation details
              if [ "$debug_level" -ge 3 ]; then
                echo "[DEBUG] File modification time: $file_time"
                echo "[DEBUG] Current time: $current_time"
                echo "[DEBUG] Calculated age: $age_days days"
              fi

              if [ "$age_days" -gt 7 ]; then
                echo "cppfront is $age_days days old, rebuilding..."
                build-cppfront
              else
                echo "cppfront is up to date ($age_days days old)"
              fi
            else
              echo "cppfront not found, building..."
              build-cppfront
            fi

            # End timing for debug levels > 3
            if [ "$debug_level" -gt 3 ]; then
              end_time=$(date +%s)
              local duration=$((end_time - start_time))
              echo "[DEBUG] check-cppfront-age completed in $duration seconds"
            fi
          }
        '';

        build-xdp2-compiler-fn = ''
          build-xdp2-compiler() {
            if [ -n "$XDP2_NIX_DEBUG" ]; then
              local debug_level=$XDP2_NIX_DEBUG
            else
              local debug_level=0
            fi
            local start_time=""
            local end_time=""

            # Start timing for debug levels > 3
            if [ "$debug_level" -gt 3 ]; then
              start_time=$(date +%s)
              echo "[DEBUG] build-xdp2-compiler started at $(date)"
            fi

            # Level 1: Function start
            if [ "$debug_level" -ge 1 ]; then
              echo "[DEBUG] Starting build-xdp2-compiler function"
            fi

            # Level 2: Clean step
            if [ "$debug_level" -ge 2 ]; then
              echo "[DEBUG] Cleaning xdp2-compiler build directory"
            fi
            echo "Cleaning and building xdp2-compiler..."

            # Navigate to repository root first
            navigate-to-repo-root || return 1

            # Clean previous build artifacts (before navigating to component)
            clean-xdp2-compiler

            # Navigate to xdp2-compiler directory
            navigate-to-component "src/tools/compiler" || return 1

            # Level 3: Build step details
            if [ "$debug_level" -ge 3 ]; then
              echo "[DEBUG] Building xdp2-compiler with make"
            fi

            # Build xdp2-compiler with error checking
            if CFLAGS_PYTHON="$CFLAGS_PYTHON" LDFLAGS_PYTHON="$LDFLAGS_PYTHON" make -j"$NIX_BUILD_CORES"; then
              echo "✓ xdp2-compiler make completed successfully"
            else
              echo "✗ ERROR: xdp2-compiler make failed"
              return 1
            fi

            # Level 2: Validation step
            if [ "$debug_level" -ge 2 ]; then
              echo "[DEBUG] Validating xdp2-compiler binary"
            fi

            # Validate binary was created
            if [ -x "./xdp2-compiler" ]; then
              echo "✓ xdp2-compiler binary created and executable"

              # Test the binary runs correctly
              echo "Testing xdp2-compiler..."
              set +e  # Temporarily disable exit on error

              # Debug output for validation command
              if [ "$debug_level" -gt 3 ]; then
                echo "[DEBUG] About to run: ./xdp2-compiler --help"
              fi
              ./xdp2-compiler --help
              test_exit_code=$?
              set -e  # Re-enable exit on error

              if [ "$test_exit_code" -eq 0 ] || [ "$test_exit_code" -eq 1 ]; then
                echo "✓ xdp2-compiler runs correctly (exit code: $test_exit_code)"
              else
                echo "⚠ WARNING: xdp2-compiler returned unexpected exit code: $test_exit_code"
                echo "But binary exists and is executable, continuing..."
              fi
            else
              echo "✗ ERROR: xdp2-compiler binary not found or not executable"
              return 1
            fi

            # Return to repository root
            navigate-to-repo-root || return 1

            # End timing for debug levels > 3
            if [ "$debug_level" -gt 3 ]; then
              end_time=$(date +%s)
              local duration=$((end_time - start_time))
              echo "[DEBUG] build-xdp2-compiler completed in $duration seconds"
            fi

            echo "xdp2-compiler built and validated successfully ( ./src/tools/compiler/xdp2-compiler )"
          }
        '';

        build-xdp2-fn = ''
          build-xdp2() {
            if [ -n "$XDP2_NIX_DEBUG" ]; then
              local debug_level=$XDP2_NIX_DEBUG
            else
              local debug_level=0
            fi
            local start_time=""
            local end_time=""

            # Start timing for debug levels > 3
            if [ "$debug_level" -gt 3 ]; then
              start_time=$(date +%s)
              echo "[DEBUG] build-xdp2 started at $(date)"
            fi

            # Level 1: Function start
            if [ "$debug_level" -ge 1 ]; then
              echo "[DEBUG] Starting build-xdp2 function"
            fi

            # Level 2: Clean step
            if [ "$debug_level" -ge 2 ]; then
              echo "[DEBUG] Cleaning xdp2 project build directory"
            fi
            echo "Cleaning and building xdp2 project..."

            # Navigate to repository root first
            navigate-to-repo-root || return 1

            # Clean previous build artifacts (before navigating to component)
            clean-xdp2

            # Navigate to src directory
            navigate-to-component "src" || return 1

            # Ensure xdp2-compiler is available in PATH
            add-to-path "$PWD/tools/compiler"
            echo "Added tools/compiler to PATH"

            # Level 3: Build step details
            if [ "$debug_level" -ge 3 ]; then
              echo "[DEBUG] Building xdp2 project with make"
            fi

            # Build the main xdp2 project
            if make -j"$NIX_BUILD_CORES"; then
              echo "✓ xdp2 project make completed successfully"
            else
              echo "✗ ERROR: xdp2 project make failed"
              echo "   Check the error messages above for details"
              return 1
            fi

            # Return to repository root
            navigate-to-repo-root || return 1

            # End timing for debug levels > 3
            if [ "$debug_level" -gt 3 ]; then
              end_time=$(date +%s)
              local duration=$((end_time - start_time))
              echo "[DEBUG] build-xdp2 completed in $duration seconds"
            fi

            echo "xdp2 project built successfully"
          }
        '';

        build-all-fn = ''
          build-all() {
            if [ -n "$XDP2_NIX_DEBUG" ]; then
              local debug_level=$XDP2_NIX_DEBUG
            else
              local debug_level=0
            fi

            echo "Building all XDP2 components..."

            if [ "$debug_level" -ge 3 ]; then
              echo "[DEBUG] Building cppfront: build-cppfront"
            fi
            build-cppfront

            if [ "$debug_level" -ge 3 ]; then
              echo "[DEBUG] Building xdp2-compiler: build-xdp2-compiler"
            fi
            build-xdp2-compiler

            if [ "$debug_level" -ge 3 ]; then
              echo "[DEBUG] Building xdp2: build-xdp2"
            fi
            build-xdp2

            echo "✓ All components built successfully"
          }
        '';

        clean-all-fn = ''
          clean-all() {
            echo "Cleaning all build artifacts..."

            # Clean each component using centralized clean functions
            clean-cppfront
            clean-xdp2-compiler
            clean-xdp2

            echo "✓ All build artifacts cleaned"
          }
        '';

        # Shellcheck function registry - list of all bash functions that should be validated by shellcheck
        # IMPORTANT: When adding or removing bash functions, update this list accordingly
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

        # Generate complete shellcheck validation function in Nix
        generate-shellcheck-validation = let
          functionNames = shellcheckFunctionRegistry;
          totalFunctions = builtins.length functionNames;

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
          '') functionNames);

          # Generate failed functions reporting
          failedFunctionsReporting = lib.concatStringsSep "\n" (map (name: ''
            if [[ "$${failed_functions[*]}" == *"${name}"* ]]; then
              echo "  - ${name}"
            fi
          '') functionNames);

        in ''
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
        '';

        run-shellcheck-fn = generate-shellcheck-validation;

        disable-exit-fn = ''
          disable-exit() {
            set +e
          }
        '';

        platform-compatibility-check-fn = ''
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
        '';

        detect-repository-root-fn = ''
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
        '';

        setup-locale-support-fn = let
          bashVarExpansion = "$";
          bashDefaultSyntax = "{LANG:-C.UTF-8}";
          bashDefaultSyntaxLC = "{LC_ALL:-C.UTF-8}";
        in ''
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
        '';

        xdp2-help-fn = ''
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

        ascii-art-logo = ''
          if command -v jp2a >/dev/null 2>&1 && [ -f "./documentation/images/xdp2-big.png" ]; then
            echo "$(jp2a --colors ./documentation/images/xdp2-big.png)"
            echo ""
          else
            echo "🚀 === XDP2 Development Shell ==="
          fi
        '';

        minimal-shell-entry = ''
          echo "🚀 === XDP2 Development Shell ==="
          echo "📦 Compiler: ${sharedConfig.compilerInfo}"
          echo "🔧 GCC and Clang are available in the environment"
          echo "🐛 Debugging tools: gdb, valgrind, strace, ltrace"
          echo "🎯 Ready to develop! 'xdp2-help' for help"
        '';

        debug-compiler-selection = ''
            if [ ${toString sharedConfig.nixDebug} -gt 4 ]; then
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
            if [ ${toString sharedConfig.nixDebug} -gt 5 ]; then
              echo "=== Environment Variables ==="
              env
              echo "=== End Environment Variables ==="
            fi
        '';


        navigate-to-repo-root-fn = ''
          navigate-to-repo-root() {
            if [ -n "$XDP2_REPO_ROOT" ]; then
              cd "$XDP2_REPO_ROOT" || return 1
            else
              echo "✗ ERROR: XDP2_REPO_ROOT not set"
              return 1
            fi
          }
        '';

        navigate-to-component-fn = ''
          navigate-to-component() {
            local component="$1"
            local target_dir="$XDP2_REPO_ROOT/$component"

            if [ ! -d "$target_dir" ]; then
              echo "✗ ERROR: Component directory not found: $target_dir"
              return 1
            fi

            cd "$target_dir" || return 1
          }
        '';

        add-to-path-fn = ''
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
        '';

        clean-cppfront-fn = ''
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
        '';

        clean-xdp2-compiler-fn = ''
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
        '';

        clean-xdp2-fn = ''
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
        '';

        # Combined build functions (ordered to avoid SC2218 - functions called before definition)
        build-functions = ''
          # Navigation functions (called by all other functions)
          ${navigate-to-repo-root-fn}
          ${navigate-to-component-fn}

          # Utility functions
          ${add-to-path-fn}
          ${check-cppfront-age-fn}

          # Individual clean functions (called by build functions and clean-build)
          ${clean-cppfront-fn}
          ${clean-xdp2-compiler-fn}
          ${clean-xdp2-fn}
          # Individual build functions (called by build-all)
          ${build-cppfront-fn}
          ${build-xdp2-compiler-fn}
          ${build-xdp2-fn}
          # Composite functions (call the individual functions above)
          ${build-all-fn}
          ${clean-all-fn}
          # Validation and help functions
          ${platform-compatibility-check-fn}
          ${detect-repository-root-fn}
          ${setup-locale-support-fn}
          ${run-shellcheck-fn}
          ${xdp2-help-fn}
        '';

      in
      {
        devShells.default = pkgs.mkShell {
          packages = sharedConfig.corePackages;

          shellHook = ''
            ${sharedEnvVars}

            ${build-functions}

            check-platform-compatibility
            detect-repository-root
            setup-locale-support

            ${debug-compiler-selection}
            ${debug-environment-vars}

            ${ascii-art-logo}

            ${smart-configure}
            smart-configure

            ${shell-aliases}

            ${colored-prompt}

            ${disable-exit-fn}

            ${minimal-shell-entry}
          '';
        };
      });
}
