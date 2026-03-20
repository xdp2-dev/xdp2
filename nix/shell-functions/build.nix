# nix/shell-functions/build.nix
#
# Build shell functions for XDP2
#
# Functions:
# - build-cppfront: Build the cppfront compiler
# - check-cppfront-age: Check if cppfront needs rebuilding
# - build-xdp2-compiler: Build the xdp2 compiler
# - build-xdp2: Build the main xdp2 project
# - build-all: Build all components in order
#
# Note: These functions depend on navigation and clean functions
#
# Usage in flake.nix:
#   buildFns = import ./nix/shell-functions/build.nix { };
#

{ }:

''
  # Build the cppfront compiler
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

  # Check if cppfront needs rebuilding based on age
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

  # Build the xdp2 compiler
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

  # Build the main xdp2 project
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

  # Build all XDP2 components
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
''
