# nix/analysis/gcc.nix
#
# GCC-based analysis: gcc-warnings and gcc-analyzer.
#
# Adapted from the reference C++ implementation:
# - Uses NIX_CFLAGS_COMPILE instead of NIX_CXXFLAGS_COMPILE
# - Adds C-specific flags: -Wstrict-prototypes, -Wold-style-definition,
#   -Wmissing-prototypes, -Wbad-function-cast
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

  gccWarningFlags = [
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-Wformat=2"
    "-Wformat-security"
    "-Wshadow"
    "-Wcast-qual"
    "-Wcast-align"
    "-Wwrite-strings"
    "-Wpointer-arith"
    "-Wconversion"
    "-Wsign-conversion"
    "-Wduplicated-cond"
    "-Wduplicated-branches"
    "-Wlogical-op"
    "-Wnull-dereference"
    "-Wdouble-promotion"
    "-Wfloat-equal"
    "-Walloca"
    "-Wvla"
    "-Werror=return-type"
    "-Werror=format-security"
    # C-specific warnings
    "-Wstrict-prototypes"
    "-Wold-style-definition"
    "-Wmissing-prototypes"
    "-Wbad-function-cast"
  ];

  mkGccAnalysisBuild = name: extraFlags:
    pkgs.stdenv.mkDerivation {
      pname = "xdp2-analysis-${name}";
      version = "0.1.0";
      inherit src;

      inherit nativeBuildInputs buildInputs;

      hardeningDisable = [ "all" ];

      env.NIX_CFLAGS_COMPILE = lib.concatStringsSep " " extraFlags;

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

      dontFixup = true;
      doCheck = false;

      postPatch = ''
        substituteInPlace thirdparty/cppfront/Makefile \
          --replace-fail 'include ../../src/config.mk' '# config.mk not needed for standalone build'

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

        # Build xdp2 libraries and capture all compiler output
        echo "Building xdp2 with ${name} flags..."
        cd src
        make -j''${NIX_BUILD_CORES:-1} 2>&1 | tee "$NIX_BUILD_TOP/build-output.log" || true
        cd ..

        runHook postBuild
      '';

      installPhase = ''
        mkdir -p $out
        # Extract warning/error lines from the build output
        grep -E ': warning:|: error:' "$NIX_BUILD_TOP/build-output.log" > $out/report.txt || true
        findings=$(wc -l < $out/report.txt)
        echo "$findings" > $out/count.txt

        # Include full build log for reference
        cp "$NIX_BUILD_TOP/build-output.log" $out/full-build.log

        {
          echo "=== ${name} Analysis ==="
          echo ""
          echo "Flags: ${lib.concatStringsSep " " extraFlags}"
          echo "Findings: $findings warnings/errors"
          if [ "$findings" -gt 0 ]; then
            echo ""
            echo "=== Warnings ==="
            cat $out/report.txt
          fi
        } > $out/summary.txt
      '';
    };

in
{
  gcc-warnings = mkGccAnalysisBuild "gcc-warnings" gccWarningFlags;

  gcc-analyzer = mkGccAnalysisBuild "gcc-analyzer" [
    "-fanalyzer"
    "-fdiagnostics-plain-output"
  ];
}
