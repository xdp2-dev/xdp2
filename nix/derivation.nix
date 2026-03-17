# nix/derivation.nix
#
# Package derivation for XDP2
#
# This module defines the actual XDP2 package using stdenv.mkDerivation.
# It enables `nix build` support and follows nixpkgs conventions.
#
# Build order:
# 1. Patch source files (postPatch)
# 2. Run configure script (configurePhase)
# 3. Build cppfront, xdp2-compiler, then xdp2 (buildPhase)
# 4. Install binaries and libraries (installPhase)
#
# Usage in flake.nix:
#   packages.default = import ./nix/derivation.nix {
#     inherit pkgs lib llvmConfig;
#     inherit (import ./nix/packages.nix { inherit pkgs llvmPackages; }) nativeBuildInputs buildInputs;
#   };
#

{ pkgs
, lib
, llvmConfig
, nativeBuildInputs
, buildInputs
  # Enable XDP2 assertions (for debugging/testing)
  # Default: false (production build, zero overhead)
, enableAsserts ? false
}:

let
  llvmPackages = llvmConfig.llvmPackages;

  # For cross-compilation, use buildPackages for host tools
  # buildPackages contains packages that run on the BUILD machine
  hostPkgs = pkgs.buildPackages;

  # Native compiler for the BUILD machine (x86_64)
  # IMPORTANT: Use stdenv.cc, NOT gcc, because in cross-compilation context
  # buildPackages.gcc returns the cross-compiler, not the native compiler
  hostCC = hostPkgs.stdenv.cc;

  # Python with scapy for configure checks (runs on HOST)
  hostPython = hostPkgs.python3.withPackages (p: [ p.scapy ]);

  # Wrapper scripts for HOST_CC/HOST_CXX that include Boost and libpcap paths
  # The configure script calls these directly to test Boost/libpcap availability
  # Use hostPkgs (buildPackages) so these run on the build machine
  # Use full paths to the Nix gcc wrapper to ensure proper include handling
  host-gcc = hostPkgs.writeShellScript "host-gcc" ''
    exec ${hostCC}/bin/gcc \
      -I${hostPkgs.boost.dev}/include \
      -I${hostPkgs.libpcap}/include \
      -L${hostPkgs.boost}/lib \
      -L${hostPkgs.libpcap.lib}/lib \
      "$@"
  '';

  host-gxx = hostPkgs.writeShellScript "host-g++" ''
    exec ${hostCC}/bin/g++ \
      -I${hostPkgs.boost.dev}/include \
      -I${hostPkgs.libpcap}/include \
      -I${hostPython}/include/python3.13 \
      -L${hostPkgs.boost}/lib \
      -L${hostPkgs.libpcap.lib}/lib \
      -L${hostPython}/lib \
      -Wl,-rpath,${hostPython}/lib \
      "$@"
  '';

  # Detect cross-compilation
  isCrossCompilation = pkgs.stdenv.buildPlatform != pkgs.stdenv.hostPlatform;

  # Target compiler (for libraries that run on the target)
  targetCC = "${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}cc";
  targetCXX = "${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}c++";
in
pkgs.stdenv.mkDerivation rec {
  pname = if enableAsserts then "xdp2-debug" else "xdp2";
  version = "0.1.0";

  src = ./..;

  # Nix-specific patches for xdp2-compiler
  #
  # NOTE: Most Nix compatibility is now handled directly in the source code:
  # - System include paths: src/tools/compiler/src/clang-tool-config.cpp
  # - Null checks: src/tools/compiler/include/xdp2gen/ast-consumer/proto-tables.h
  # - Assertions: src/tools/compiler/include/xdp2gen/assert.h
  #
  # See documentation/nix/clang-tool-refactor-plan.md for details.
  patches = [
    # No patches currently required - fixes are in source code
  ];

  inherit nativeBuildInputs buildInputs;

  # Disable hardening flags that interfere with XDP/BPF code
  hardeningDisable = [ "all" ];

  # Set up environment variables for the build
  # HOST_CC/CXX run on the build machine (for xdp2-compiler, cppfront)
  HOST_CC = "${hostCC}/bin/gcc";
  HOST_CXX = "${hostCC}/bin/g++";
  HOST_LLVM_CONFIG = "${llvmConfig.llvm-config-wrapped}/bin/llvm-config";
  XDP2_CLANG_VERSION = llvmConfig.version;
  XDP2_CLANG_RESOURCE_PATH = llvmConfig.paths.clangResourceDir;

  # Add LLVM/Clang libs to library path (use host versions for xdp2-compiler)
  LD_LIBRARY_PATH = lib.makeLibraryPath [
    llvmPackages.llvm
    llvmPackages.libclang.lib
    hostPkgs.boost
  ];

  # Compiler flags - enable assertions for debug builds
  NIX_CFLAGS_COMPILE = lib.optionalString enableAsserts "-DXDP2_ENABLE_ASSERTS=1";

  # Post-patch phase: Fix paths and apply Nix-specific patches
  postPatch = ''
    # Fix cppfront Makefile to use source directory path
    substituteInPlace thirdparty/cppfront/Makefile \
      --replace-fail 'include ../../src/config.mk' '# config.mk not needed for standalone build'

    # Add functional header to cppfront (required for newer GCC)
    sed -i '1i#include <functional>\n#include <unordered_map>\n' thirdparty/cppfront/include/cpp2util.h

    # Patch configure.sh to use CC_GCC from environment (for cross-compilation)
    # The original script sets CC_GCC="gcc" unconditionally, but we need it to
    # respect our host-gcc wrapper which includes the correct include paths
    substituteInPlace src/configure.sh \
      --replace-fail 'CC_GCC="gcc"' 'CC_GCC="''${CC_GCC:-gcc}"' \
      --replace-fail 'CC_CXX="g++"' 'CC_CXX="''${CC_CXX:-g++}"'
  '';

  # Configure phase: Generate config.mk
  configurePhase = ''
    runHook preConfigure

    cd src

    # Add host tools to PATH so configure.sh can find them
    # This is needed for cross-compilation where only cross-tools are in PATH
    # Use hostCC (stdenv.cc) which is the native x86_64 compiler
    # Also add hostPython which has scapy for the scapy check
    export PATH="${hostCC}/bin:${hostPython}/bin:$PATH"

    # Set up CC_GCC and CC_CXX to use our wrapper scripts that include boost/libpcap paths
    # This is needed because configure.sh uses these for compile tests
    export CC_GCC="${host-gcc}"
    export CC_CXX="${host-gxx}"

    # Set up environment for configure using the Boost-aware wrapper scripts
    export CC="${host-gcc}"
    export CXX="${host-gxx}"

    # Set up PKG_CONFIG_PATH to find HOST libraries (Python, etc.)
    # This ensures configure.sh finds x86_64 Python, not RISC-V Python
    export PKG_CONFIG_PATH="${hostPython}/lib/pkgconfig:$PKG_CONFIG_PATH"
    export HOST_CC="$CC"
    export HOST_CXX="$CXX"
    export HOST_LLVM_CONFIG="${llvmConfig.llvm-config-wrapped}/bin/llvm-config"

    # Set clang resource path BEFORE configure runs so it gets written to config.mk
    # This is critical for xdp2-compiler to find clang headers at runtime
    export XDP2_CLANG_VERSION="${llvmConfig.version}"
    export XDP2_CLANG_RESOURCE_PATH="${llvmConfig.paths.clangResourceDir}"
    export XDP2_C_INCLUDE_PATH="${llvmConfig.paths.clangResourceDir}/include"

    # Run configure script with debug output
    export CONFIGURE_DEBUG_LEVEL=7
    bash configure.sh --build-opt-parser

    # Fix PATH_ARG for Nix environment (remove hardcoded paths)
    if grep -q 'PATH_ARG="--with-path=' config.mk; then
      sed -i 's|PATH_ARG="--with-path=.*"|PATH_ARG=""|' config.mk
    fi

    # Fix HOST_CC/HOST_CXX in config.mk to use our wrapper scripts with correct paths
    # configure.sh writes "HOST_CC := gcc" which won't find HOST libraries
    sed -i 's|^HOST_CC := gcc$|HOST_CC := ${host-gcc}|' config.mk
    sed -i 's|^HOST_CXX := g++$|HOST_CXX := ${host-gxx}|' config.mk

    # Add HOST boost library paths to LDFLAGS for xdp2-compiler
    echo "HOST_LDFLAGS := -L${hostPkgs.boost}/lib -Wl,-rpath,${hostPkgs.boost}/lib" >> config.mk

    cd ..

    runHook postConfigure
  '';

  # Build phase: Build all components in order
  buildPhase = ''
    runHook preBuild

    # Set up environment
    # HOST_CC/CXX run on the build machine (for xdp2-compiler, cppfront)
    # Use hostCC (stdenv.cc) which is the native x86_64 compiler
    export HOST_CC="${hostCC}/bin/gcc"
    export HOST_CXX="${hostCC}/bin/g++"
    export HOST_LLVM_CONFIG="${llvmConfig.llvm-config-wrapped}/bin/llvm-config"
    export NIX_BUILD_CORES=$NIX_BUILD_CORES
    export XDP2_CLANG_VERSION="${llvmConfig.version}"
    export XDP2_CLANG_RESOURCE_PATH="${llvmConfig.paths.clangResourceDir}"

    # Include paths for xdp2-compiler's libclang usage
    # These are needed because ClangTool bypasses the Nix clang wrapper
    # Use host (build machine) paths since xdp2-compiler runs on host
    export XDP2_C_INCLUDE_PATH="${llvmConfig.paths.clangResourceDir}/include"
    export XDP2_GLIBC_INCLUDE_PATH="${hostPkgs.stdenv.cc.libc.dev}/include"
    export XDP2_LINUX_HEADERS_PATH="${hostPkgs.linuxHeaders}/include"

    # 1. Build cppfront compiler (runs on host)
    echo "Building cppfront..."
    cd thirdparty/cppfront
    $HOST_CXX -std=c++20 source/cppfront.cpp -o cppfront-compiler
    cd ../..

    # 2. Build xdp2-compiler (runs on host, needs host LLVM)
    echo "Building xdp2-compiler..."
    cd src/tools/compiler
    make -j$NIX_BUILD_CORES
    cd ../../..

    # 3. Build main xdp2 project (libraries for target)
    echo "Building xdp2..."
    cd src

    # NOTE: parse_dump was previously skipped due to a std::optional assertion failure
    # in LLVM pattern matching. Fixed in main.cpp by adding null check for next_proto_data.
    # See documentation/nix/clang-tool-refactor-log.md for details.

    ${lib.optionalString isCrossCompilation ''
      echo "Cross-compilation detected: ${pkgs.stdenv.hostPlatform.config}"
      echo "  Target CC: ${targetCC}"
      echo "  Target CXX: ${targetCXX}"
      # Override CC/CXX in config.mk for target libraries
      sed -i "s|^CC :=.*|CC := ${targetCC}|" config.mk
      sed -i "s|^CXX :=.*|CXX := ${targetCXX}|" config.mk
      # Add include paths for cross-compilation
      INCLUDE_FLAGS="-I$(pwd)/include -I${pkgs.linuxHeaders}/include"
      sed -i "s|^EXTRA_CFLAGS :=.*|EXTRA_CFLAGS := $INCLUDE_FLAGS|" config.mk
      if ! grep -q "^EXTRA_CFLAGS" config.mk; then
        echo "EXTRA_CFLAGS := $INCLUDE_FLAGS" >> config.mk
      fi
    ''}

    make -j$NIX_BUILD_CORES
    cd ..

    runHook postBuild
  '';

  # Install phase: Install binaries and libraries
  installPhase = ''
    runHook preInstall

    # Create output directories
    mkdir -p $out/bin
    mkdir -p $out/lib
    mkdir -p $out/include
    mkdir -p $out/share/xdp2

    # Install xdp2-compiler
    install -m 755 src/tools/compiler/xdp2-compiler $out/bin/

    # Install cppfront-compiler (useful for development)
    install -m 755 thirdparty/cppfront/cppfront-compiler $out/bin/

    # Install libraries (if any are built as shared)
    find src/lib -name "*.so" -exec install -m 755 {} $out/lib/ \; 2>/dev/null || true
    find src/lib -name "*.a" -exec install -m 644 {} $out/lib/ \; 2>/dev/null || true

    # Install headers (use -L to dereference symlinks like arch -> platform/...)
    cp -rL src/include/* $out/include/ 2>/dev/null || true

    # Install templates
    cp -r src/templates $out/share/xdp2/ 2>/dev/null || true

    runHook postInstall
  '';

  meta = with lib; {
    description = "XDP2 packet processing framework";
    longDescription = ''
      XDP2 is a high-performance packet processing framework that uses
      eBPF/XDP for fast packet handling in the Linux kernel.
    '';
    homepage = "https://github.com/xdp2/xdp2";
    license = licenses.mit;  # Update if different
    platforms = platforms.linux;
    maintainers = [ ];
  };
}
