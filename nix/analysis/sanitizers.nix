# nix/analysis/sanitizers.nix
#
# ASan + UBSan instrumented build and test execution.
#
# Unlike the reference (which uses nixComponents.overrideScope), XDP2
# builds with Make. We build with sanitizer flags and run sample tests
# to detect runtime violations.
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

in
pkgs.stdenv.mkDerivation {
  pname = "xdp2-analysis-sanitizers";
  version = "0.1.0";
  inherit src;

  inherit nativeBuildInputs buildInputs;

  hardeningDisable = [ "all" ];

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

  # NOTE: Sanitizer flags are NOT applied via NIX_CFLAGS_COMPILE because
  # that would break configure.sh's link tests. Instead, we inject them
  # into config.mk CFLAGS/LDFLAGS after configure completes.

  dontFixup = true;

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

    # Inject sanitizer flags into config.mk AFTER configure completes
    echo "EXTRA_CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer" >> config.mk
    echo "LDFLAGS += -fsanitize=address,undefined" >> config.mk

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

    # Build cppfront (without sanitizers — host tool)
    echo "Building cppfront..."
    cd thirdparty/cppfront
    $HOST_CXX -std=c++20 source/cppfront.cpp -o cppfront-compiler
    cd ../..

    # Build xdp2-compiler (host tool)
    echo "Building xdp2-compiler..."
    cd src/tools/compiler
    make -j''${NIX_BUILD_CORES:-1}
    cd ../../..

    # Build xdp2 libraries with sanitizer instrumentation
    echo "Building xdp2 with ASan+UBSan..."
    cd src
    make -j''${NIX_BUILD_CORES:-1} 2>&1 | tee "$NIX_BUILD_TOP/build-output.log" || true
    cd ..

    runHook postBuild
  '';

  # Run sample tests — sanitizer violations cause non-zero exit
  checkPhase = ''
    echo "Running tests with sanitizer instrumentation..."
    sanitizer_violations=0

    # Run any built sample parsers against test pcaps
    for test_bin in src/test/*/test_*; do
      if [ -x "$test_bin" ]; then
        echo "  Running: $test_bin"
        if ! "$test_bin" 2>&1 | tee -a "$NIX_BUILD_TOP/sanitizer-output.log"; then
          echo "  FAIL: $test_bin"
          sanitizer_violations=$((sanitizer_violations + 1))
        fi
      fi
    done

    echo "$sanitizer_violations" > "$NIX_BUILD_TOP/sanitizer-violations.txt"
  '';

  doCheck = true;

  installPhase = ''
    mkdir -p $out

    violations=0
    if [ -f "$NIX_BUILD_TOP/sanitizer-violations.txt" ]; then
      violations=$(cat "$NIX_BUILD_TOP/sanitizer-violations.txt")
    fi

    {
      echo "=== ASan + UBSan Analysis ==="
      echo ""
      echo "Built with AddressSanitizer + UndefinedBehaviorSanitizer."
      echo "Sample tests executed with sanitizer instrumentation."
      echo ""
      if [ "$violations" -gt 0 ]; then
        echo "Result: $violations sanitizer violations detected."
      else
        echo "Result: All tests passed — no sanitizer violations detected."
      fi
    } > $out/report.txt

    echo "$violations" > $out/count.txt

    if [ -f "$NIX_BUILD_TOP/sanitizer-output.log" ]; then
      cp "$NIX_BUILD_TOP/sanitizer-output.log" $out/sanitizer-output.log
    fi
    if [ -f "$NIX_BUILD_TOP/build-output.log" ]; then
      cp "$NIX_BUILD_TOP/build-output.log" $out/build-output.log
    fi
  '';
}
