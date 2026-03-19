# nix/analysis/clang-analyzer.nix
#
# Clang Static Analyzer (scan-build) for XDP2's C codebase.
#
# Adapted from the reference C++ implementation:
# - Uses C-specific checkers (core.*, security.*, unix.*, alpha.security.*)
# - No C++ checkers (cplusplus.*, alpha.cplusplus.*)
# - Builds via Make instead of Meson+Ninja
#

{
  lib,
  pkgs,
  src,
  llvmConfig,
  nativeBuildInputs,
  buildInputs,
}:

let
  llvmPackages = llvmConfig.llvmPackages;
  hostPkgs = pkgs.buildPackages;
  hostCC = hostPkgs.stdenv.cc;
  hostPython = hostPkgs.python3.withPackages (p: [ p.scapy ]);

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

  scanBuildCheckers = lib.concatStringsSep " " [
    "-enable-checker core.NullDereference"
    "-enable-checker core.DivideZero"
    "-enable-checker core.UndefinedBinaryOperatorResult"
    "-enable-checker core.uninitialized.Assign"
    "-enable-checker security.FloatLoopCounter"
    "-enable-checker security.insecureAPI.getpw"
    "-enable-checker security.insecureAPI.gets"
    "-enable-checker security.insecureAPI.vfork"
    "-enable-checker unix.Malloc"
    "-enable-checker unix.MallocSizeof"
    "-enable-checker unix.MismatchedDeallocator"
    "-enable-checker alpha.security.ArrayBoundV2"
    "-enable-checker alpha.unix.SimpleStream"
  ];

in
pkgs.stdenv.mkDerivation {
  pname = "xdp2-analysis-clang-analyzer";
  version = "0.1.0";
  inherit src;

  nativeBuildInputs = nativeBuildInputs ++ [
    pkgs.clang-analyzer
  ];
  inherit buildInputs;

  hardeningDisable = [ "all" ];
  dontFixup = true;
  doCheck = false;

  HOST_CC = "${hostCC}/bin/gcc";
  HOST_CXX = "${hostCC}/bin/g++";
  HOST_LLVM_CONFIG = "${llvmConfig.llvm-config-wrapped}/bin/llvm-config";
  XDP2_CLANG_VERSION = llvmConfig.version;
  XDP2_CLANG_RESOURCE_PATH = llvmConfig.paths.clangResourceDir;

  LD_LIBRARY_PATH = lib.makeLibraryPath [
    llvmPackages.llvm
    llvmPackages.libclang.lib
    hostPkgs.boost
  ];

  postPatch = ''
    substituteInPlace thirdparty/cppfront/Makefile \
      --replace-fail 'include ../../src/config.mk' '# config.mk not needed for standalone build'

    sed -i '1i#include <functional>\n#include <unordered_map>\n' thirdparty/cppfront/include/cpp2util.h

    substituteInPlace src/configure.sh \
      --replace-fail 'CC_GCC="gcc"' 'CC_GCC="''${CC_GCC:-gcc}"' \
      --replace-fail 'CC_CXX="g++"' 'CC_CXX="''${CC_CXX:-g++}"'
  '';

  configurePhase = ''
    runHook preConfigure

    cd src

    export PATH="${hostCC}/bin:${hostPython}/bin:$PATH"
    export CC_GCC="${host-gcc}"
    export CC_CXX="${host-gxx}"
    export CC="${host-gcc}"
    export CXX="${host-gxx}"
    export PKG_CONFIG_PATH="${hostPython}/lib/pkgconfig:$PKG_CONFIG_PATH"
    export HOST_CC="$CC"
    export HOST_CXX="$CXX"
    export HOST_LLVM_CONFIG="${llvmConfig.llvm-config-wrapped}/bin/llvm-config"
    export XDP2_CLANG_VERSION="${llvmConfig.version}"
    export XDP2_CLANG_RESOURCE_PATH="${llvmConfig.paths.clangResourceDir}"
    export XDP2_C_INCLUDE_PATH="${llvmConfig.paths.clangResourceDir}/include"
    export CONFIGURE_DEBUG_LEVEL=7

    bash configure.sh --build-opt-parser

    if grep -q 'PATH_ARG="--with-path=' config.mk; then
      sed -i 's|PATH_ARG="--with-path=.*"|PATH_ARG=""|' config.mk
    fi

    sed -i 's|^HOST_CC := gcc$|HOST_CC := ${host-gcc}|' config.mk
    sed -i 's|^HOST_CXX := g++$|HOST_CXX := ${host-gxx}|' config.mk
    echo "HOST_LDFLAGS := -L${hostPkgs.boost}/lib -Wl,-rpath,${hostPkgs.boost}/lib" >> config.mk

    cd ..

    runHook postConfigure
  '';

  buildPhase = ''
    runHook preBuild

    export HOST_CC="${hostCC}/bin/gcc"
    export HOST_CXX="${hostCC}/bin/g++"
    export HOST_LLVM_CONFIG="${llvmConfig.llvm-config-wrapped}/bin/llvm-config"
    export XDP2_CLANG_VERSION="${llvmConfig.version}"
    export XDP2_CLANG_RESOURCE_PATH="${llvmConfig.paths.clangResourceDir}"
    export XDP2_C_INCLUDE_PATH="${llvmConfig.paths.clangResourceDir}/include"
    export XDP2_GLIBC_INCLUDE_PATH="${hostPkgs.stdenv.cc.libc.dev}/include"
    export XDP2_LINUX_HEADERS_PATH="${hostPkgs.linuxHeaders}/include"

    # Build cppfront first
    echo "Building cppfront..."
    cd thirdparty/cppfront
    $HOST_CXX -std=c++20 source/cppfront.cpp -o cppfront-compiler
    cd ../..

    # Build xdp2-compiler
    echo "Building xdp2-compiler..."
    cd src/tools/compiler
    make -j''${NIX_BUILD_CORES:-1}
    cd ../../..

    # Build xdp2 libraries wrapped with scan-build.
    # Use full path to clang-analyzer's scan-build (properly wrapped with Nix shebang).
    # The one from llvmPackages.clang has a broken /usr/bin/env shebang.
    echo "Running scan-build on xdp2..."
    cd src
    ${pkgs.clang-analyzer}/bin/scan-build \
      --use-analyzer=${llvmPackages.clang}/bin/clang \
      ${scanBuildCheckers} \
      -o "$NIX_BUILD_TOP/scan-results" \
      make -j''${NIX_BUILD_CORES:-1} \
      2>&1 | tee "$NIX_BUILD_TOP/scan-build.log" || true
    cd ..

    runHook postBuild
  '';

  installPhase = ''
    mkdir -p $out

    # Copy HTML reports if produced
    if [ -d "$NIX_BUILD_TOP/scan-results" ] && [ "$(ls -A "$NIX_BUILD_TOP/scan-results" 2>/dev/null)" ]; then
      mkdir -p $out/html-report
      cp -r "$NIX_BUILD_TOP/scan-results"/* $out/html-report/ 2>/dev/null || true
    fi

    # Extract finding count from scan-build output
    findings=$(grep -oP '\d+ bugs? found' "$NIX_BUILD_TOP/scan-build.log" | grep -oP '^\d+' || echo "0")
    echo "$findings" > $out/count.txt

    cp "$NIX_BUILD_TOP/scan-build.log" $out/report.txt

    {
      echo "=== Clang Static Analyzer (C) ==="
      echo ""
      echo "Path-sensitive analysis with C-specific checkers."
      echo "Findings: $findings"
    } > $out/summary.txt
  '';
}
