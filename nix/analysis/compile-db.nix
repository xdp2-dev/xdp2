# nix/analysis/compile-db.nix
#
# Generate compile_commands.json for XDP2.
#
# Unlike the reference Nix project (which uses Meson's built-in compile DB
# generation), XDP2 uses Make. We parse `make V=1 VERBOSE=1` output directly
# because bear's LD_PRELOAD fails in the Nix sandbox, and compiledb doesn't
# recognize Nix wrapper paths as compilers.
#

{
  pkgs,
  lib,
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

  # Python script to generate compile_commands.json from make build output.
  genCompileDbScript = pkgs.writeText "gen-compile-db.py" ''
    import json, os, re, sys

    make_output = sys.argv[1]
    output_file = sys.argv[2]
    store_src = sys.argv[3]
    source_root = sys.argv[4]

    build_prefix = "/build/" + source_root

    entries = []
    current_dir = None

    with open(make_output) as f:
        raw_lines = f.readlines()

    print(f"Raw lines read: {len(raw_lines)}", file=sys.stderr)

    # Join backslash-continued lines, stripping continuation indentation
    lines = []
    buf = ""
    for raw in raw_lines:
        stripped = raw.rstrip('\n').rstrip('\r')
        if stripped.rstrip().endswith('\\'):
            s = stripped.rstrip()
            buf += s[:-1].rstrip() + " "
        else:
            if buf:
                # This is a continuation line - strip leading whitespace
                buf += stripped.lstrip()
            else:
                buf = stripped
            lines.append(buf)
            buf = ""
    if buf:
        lines.append(buf)

    print(f"Joined lines: {len(lines)}", file=sys.stderr)

    c_lines = [l for l in lines if ' -c ' in l]
    print(f"Compilation lines found: {len(c_lines)}", file=sys.stderr)

    for line in lines:
        # Track directory changes from make -w
        m = re.match(r"make\[\d+\]: Entering directory '(.+)'", line)
        if m:
            current_dir = m.group(1)
            continue

        # Match C/C++ compilation commands: must contain -c flag
        if ' -c ' not in line:
            continue

        # Find source file: last token matching *.c, *.cc, *.cpp, *.cxx
        tokens = line.split()
        src_file = None
        for token in reversed(tokens):
            if re.match(r'.*\.(?:c|cc|cpp|cxx|C)$', token):
                src_file = token
                break
        if not src_file:
            continue

        directory = current_dir or os.getcwd()

        # Normalize paths
        abs_file = src_file
        if not os.path.isabs(src_file):
            abs_file = os.path.normpath(os.path.join(directory, src_file))

        # Fix sandbox paths to store paths
        abs_file = abs_file.replace(build_prefix, store_src)
        directory = directory.replace(build_prefix, store_src)
        cmd = line.strip().replace(build_prefix, store_src)

        entries.append({
            "directory": directory,
            "command": cmd,
            "file": abs_file,
        })

    with open(output_file, "w") as f:
        json.dump(entries, f, indent=2)

    print(f"Generated {len(entries)} compile commands", file=sys.stderr)
  '';

in
pkgs.stdenv.mkDerivation {
  pname = "xdp2-compilation-db";
  version = "0.1.0";

  src = ../..;

  nativeBuildInputs = nativeBuildInputs ++ [
    pkgs.buildPackages.python3
  ];
  inherit buildInputs;

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

  dontFixup = true;
  doCheck = false;

  # Replicate derivation.nix's postPatch
  postPatch = ''
    substituteInPlace thirdparty/cppfront/Makefile \
      --replace-fail 'include ../../src/config.mk' '# config.mk not needed for standalone build'

    substituteInPlace src/configure.sh \
      --replace-fail 'CC_GCC="gcc"' 'CC_GCC="''${CC_GCC:-gcc}"' \
      --replace-fail 'CC_CXX="g++"' 'CC_CXX="''${CC_CXX:-g++}"'
  '';

  # Replicate derivation.nix's configurePhase
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

  # Build prerequisites, then use compiledb to capture compile commands
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

    # Build cppfront first (needed by xdp2-compiler)
    echo "Building cppfront..."
    cd thirdparty/cppfront
    $HOST_CXX -std=c++20 source/cppfront.cpp -o cppfront-compiler
    cd ../..

    # Build xdp2-compiler (needed for source generation)
    echo "Building xdp2-compiler..."
    cd src/tools/compiler
    make -j''${NIX_BUILD_CORES:-1}
    cd ../../..

    # Build xdp2 with verbose output and capture all compiler invocations.
    # We parse the real build output because:
    # - bear's LD_PRELOAD doesn't work in Nix sandbox
    # - compiledb doesn't recognize Nix wrapper paths as compilers
    # Use -j1 to prevent interleaved output that breaks line continuation parsing.
    # Use both V=1 and VERBOSE=1 for full command echoing.
    echo "Building xdp2 libraries (capturing compile commands)..."
    cd src
    make V=1 VERBOSE=1 -j1 -wk 2>&1 | tee "$NIX_BUILD_TOP/make-build.log" || true
    cd ..

    runHook postBuild
  '';

  installPhase = ''
    mkdir -p $out

    ${pkgs.buildPackages.python3}/bin/python3 \
      ${genCompileDbScript} \
      "$NIX_BUILD_TOP/make-build.log" \
      "$out/compile_commands.json" \
      "${../..}" \
      "$sourceRoot"
  '';
}
