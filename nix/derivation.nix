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
}:

let
  llvmPackages = llvmConfig.llvmPackages;
in
pkgs.stdenv.mkDerivation rec {
  pname = "xdp2";
  version = "0.1.0";

  src = ./..;

  # Nix-specific patches for xdp2-compiler
  #
  # These patches are required because xdp2-compiler uses libclang's ClangTool API
  # directly, which bypasses the Nix clang wrapper. The wrapper normally handles
  # include path setup, but ClangTool doesn't benefit from it.
  #
  # See documentation/nix/phase6_segfault_defect.md for full investigation details.
  patches = [
    # Patch 1: System include paths for Nix environment
    #
    # Problem: ClangTool bypasses the Nix clang wrapper which normally adds
    # -isystem flags for system headers. Without these, header resolution fails
    # and the AST contains RecoveryExpr/contains-errors nodes.
    #
    # Solution: Read include paths from environment variables set by the Nix
    # derivation (XDP2_C_INCLUDE_PATH, XDP2_GLIBC_INCLUDE_PATH, XDP2_LINUX_HEADERS_PATH)
    # and add them as -isystem arguments to ClangTool.
    #
    # Also adds optional debug diagnostic output (enable with -DXDP2_COMPILER_DEBUG)
    # using LLVM 21 compatible API.
    ./patches/01-nix-clang-system-includes.patch

    # Patch 2: Null check for tentative definitions
    #
    # Problem: C tentative definitions like "static const struct T name;" create
    # VarDecls where hasInit() behavior differs between clang versions:
    #   - Ubuntu clang 18.1.3: hasInit() returns false
    #   - Nix clang 18.1.8+: hasInit() returns true with void-type InitListExpr
    #
    # When getAs<RecordType>() is called on a void type, it returns nullptr,
    # causing a segfault when ->getDecl() is called.
    #
    # Solution: Check if getAs<RecordType>() returns null and skip tentative
    # definitions. The actual definition will be processed when encountered later.
    ./patches/02-tentative-definition-null-check.patch
  ];

  inherit nativeBuildInputs buildInputs;

  # Disable hardening flags that interfere with XDP/BPF code
  hardeningDisable = [ "all" ];

  # Set up environment variables for the build
  HOST_CC = "${pkgs.gcc}/bin/gcc";
  HOST_CXX = "${pkgs.gcc}/bin/g++";
  HOST_LLVM_CONFIG = "${llvmConfig.llvm-config-wrapped}/bin/llvm-config";
  XDP2_CLANG_VERSION = llvmConfig.version;
  XDP2_CLANG_RESOURCE_PATH = llvmConfig.paths.clangResourceDir;

  # Add LLVM/Clang libs to library path
  LD_LIBRARY_PATH = lib.makeLibraryPath [
    llvmPackages.llvm
    llvmPackages.libclang.lib
    pkgs.boost
  ];

  # Post-patch phase: Fix paths and apply Nix-specific patches
  postPatch = ''
    # Fix cppfront Makefile to use source directory path
    substituteInPlace thirdparty/cppfront/Makefile \
      --replace-fail 'include ../../src/config.mk' '# config.mk not needed for standalone build'

    # Add functional header to cppfront (required for newer GCC)
    sed -i '1i#include <functional>\n#include <unordered_map>\n' thirdparty/cppfront/include/cpp2util.h
  '';

  # Configure phase: Generate config.mk
  configurePhase = ''
    runHook preConfigure

    cd src

    # Set up environment for configure
    export CC="${pkgs.gcc}/bin/gcc"
    export CXX="${pkgs.gcc}/bin/g++"
    export HOST_CC="$CC"
    export HOST_CXX="$CXX"
    export HOST_LLVM_CONFIG="${llvmConfig.llvm-config-wrapped}/bin/llvm-config"

    # Run configure script
    bash configure.sh --build-opt-parser

    # Fix PATH_ARG for Nix environment (remove hardcoded paths)
    if grep -q 'PATH_ARG="--with-path=' config.mk; then
      sed -i 's|PATH_ARG="--with-path=.*"|PATH_ARG=""|' config.mk
    fi

    cd ..

    runHook postConfigure
  '';

  # Build phase: Build all components in order
  buildPhase = ''
    runHook preBuild

    # Set up environment
    export HOST_CC="${pkgs.gcc}/bin/gcc"
    export HOST_CXX="${pkgs.gcc}/bin/g++"
    export HOST_LLVM_CONFIG="${llvmConfig.llvm-config-wrapped}/bin/llvm-config"
    export NIX_BUILD_CORES=$NIX_BUILD_CORES
    export XDP2_CLANG_VERSION="${llvmConfig.version}"
    export XDP2_CLANG_RESOURCE_PATH="${llvmConfig.paths.clangResourceDir}"

    # Include paths for xdp2-compiler's libclang usage
    # These are needed because ClangTool bypasses the Nix clang wrapper
    export XDP2_C_INCLUDE_PATH="${llvmConfig.paths.clangResourceDir}/include"
    export XDP2_GLIBC_INCLUDE_PATH="${pkgs.stdenv.cc.libc.dev}/include"
    export XDP2_LINUX_HEADERS_PATH="${pkgs.linuxHeaders}/include"

    # 1. Build cppfront compiler
    echo "Building cppfront..."
    cd thirdparty/cppfront
    $HOST_CXX -std=c++20 source/cppfront.cpp -o cppfront-compiler
    cd ../..

    # 2. Build xdp2-compiler
    echo "Building xdp2-compiler..."
    cd src/tools/compiler
    make -j$NIX_BUILD_CORES
    cd ../../..

    # 3. Build main xdp2 project
    echo "Building xdp2..."
    cd src
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
