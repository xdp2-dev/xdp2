# nix/samples/default.nix
#
# Pre-built sample binaries for XDP2
#
# This module builds XDP2 sample binaries at Nix build time, which is essential
# for cross-compilation scenarios (e.g., building for RISC-V on x86_64).
#
# The key insight is that xdp2-compiler runs on the HOST (x86_64), generating
# .p.c files, which are then compiled with the TARGET toolchain (e.g., RISC-V gcc)
# and linked against TARGET libraries.
#
# Usage in flake.nix:
#   prebuiltSamples = import ./nix/samples {
#     inherit pkgs;                       # Host pkgs (for xdp2-compiler's libclang)
#     xdp2 = xdp2-debug;                 # Host xdp2 (for xdp2-compiler binary)
#     xdp2Target = xdp2-debug-riscv64;   # Target xdp2 (for libraries)
#     targetPkgs = pkgsCrossRiscv;       # Target pkgs for binaries
#   };
#
# For native builds, pass targetPkgs = pkgs and xdp2Target = xdp2 (or omit them).
#

{ pkgs
, xdp2                      # Host xdp2 (provides xdp2-compiler)
, xdp2Target ? xdp2         # Target xdp2 (provides libraries to link against)
, targetPkgs ? pkgs
}:

let
  lib = pkgs.lib;

  # LLVM config for xdp2-compiler's libclang (runs on host)
  llvmConfig = import ../llvm.nix { inherit pkgs; lib = pkgs.lib; };

  # Source directory (repository root)
  srcRoot = ../..;

  # Common build function for parser samples
  # These samples build userspace binaries that parse pcap files
  buildParserSample = { name, srcDir, targets, extraBuildCommands ? "" }:
    targetPkgs.stdenv.mkDerivation {
      pname = "xdp2-sample-${name}";
      version = xdp2.version or "0.1.0";

      src = srcDir;

      # Host tools (run on build machine)
      nativeBuildInputs = [
        pkgs.gnumake
      ];

      # Target libraries (linked into target binaries)
      buildInputs = [
        targetPkgs.libpcap
        targetPkgs.libpcap.lib
      ];

      # Disable hardening for BPF compatibility
      hardeningDisable = [ "all" ];

      # Environment for xdp2-compiler (runs on host)
      XDP2_C_INCLUDE_PATH = "${llvmConfig.paths.clangResourceDir}/include";
      XDP2_GLIBC_INCLUDE_PATH = "${pkgs.stdenv.cc.libc.dev}/include";
      XDP2_LINUX_HEADERS_PATH = "${pkgs.linuxHeaders}/include";

      buildPhase = ''
        runHook preBuild

        # Set up paths
        # xdp2-compiler is from HOST xdp2
        export PATH="${xdp2}/bin:$PATH"

        # Verify xdp2-compiler is available
        echo "Using xdp2-compiler from: ${xdp2}/bin/xdp2-compiler"
        ${xdp2}/bin/xdp2-compiler --version || true

        # Use $CC from stdenv (correctly set for cross-compilation)
        echo "Using compiler: $CC"
        echo "Target xdp2 (libraries): ${xdp2Target}"

        # Build each target
        ${lib.concatMapStringsSep "\n" (target: ''
          echo "Building ${target}..."

          # Find the source file (either target.c or parser.c)
          if [ -f "${target}.c" ]; then
            SRCFILE="${target}.c"
          elif [ -f "parser.c" ]; then
            SRCFILE="parser.c"
          else
            echo "ERROR: Cannot find source file for ${target}"
            exit 1
          fi

          # First compile with target gcc to check for errors
          # Use xdp2Target for includes (target headers)
          $CC \
            -I${xdp2Target}/include \
            -I${targetPkgs.libpcap}/include \
            -c -o ${target}.o "$SRCFILE" || true

          # Generate optimized parser code with xdp2-compiler (runs on host)
          # Use host xdp2 for compiler, but target xdp2 headers
          ${xdp2}/bin/xdp2-compiler \
            -I${xdp2Target}/include \
            -i "$SRCFILE" \
            -o ${target}.p.c

          # Compile the final binary for target architecture
          # Link against xdp2Target libraries (RISC-V)
          $CC \
            -I${xdp2Target}/include \
            -I${targetPkgs.libpcap}/include \
            -L${xdp2Target}/lib \
            -L${targetPkgs.libpcap.lib}/lib \
            -Wl,-rpath,${xdp2Target}/lib \
            -Wl,-rpath,${targetPkgs.libpcap.lib}/lib \
            -g \
            -o ${target} ${target}.p.c \
            -lpcap -lxdp2 -lcli -lsiphash
        '') targets}

        ${extraBuildCommands}

        runHook postBuild
      '';

      installPhase = ''
        runHook preInstall

        mkdir -p $out/bin
        mkdir -p $out/share/xdp2-samples/${name}

        # Install binaries
        ${lib.concatMapStringsSep "\n" (target: ''
          install -m 755 ${target} $out/bin/
        '') targets}

        # Copy source files for reference
        cp -r . $out/share/xdp2-samples/${name}/

        runHook postInstall
      '';

      meta = {
        description = "XDP2 ${name} sample (pre-built)";
        platforms = lib.platforms.linux;
      };
    };

  # Build flow_tracker_combo sample (userspace + XDP)
  buildFlowTrackerCombo = targetPkgs.stdenv.mkDerivation {
    pname = "xdp2-sample-flow-tracker-combo";
    version = xdp2.version or "0.1.0";

    src = srcRoot + "/samples/xdp/flow_tracker_combo";

    nativeBuildInputs = [
      pkgs.gnumake
    ];

    buildInputs = [
      targetPkgs.libpcap
      targetPkgs.libpcap.lib
    ];

    hardeningDisable = [ "all" ];

    XDP2_C_INCLUDE_PATH = "${llvmConfig.paths.clangResourceDir}/include";
    XDP2_GLIBC_INCLUDE_PATH = "${pkgs.stdenv.cc.libc.dev}/include";
    XDP2_LINUX_HEADERS_PATH = "${pkgs.linuxHeaders}/include";

    buildPhase = ''
      runHook preBuild

      export PATH="${xdp2}/bin:$PATH"

      echo "Building flow_tracker_combo..."
      echo "Using compiler: $CC"
      echo "Target xdp2 (libraries): ${xdp2Target}"

      # First compile parser.c to check for errors
      $CC \
        -I${xdp2Target}/include \
        -I${targetPkgs.libpcap}/include \
        -c -o parser.o parser.c || true

      # Generate optimized parser code
      ${xdp2}/bin/xdp2-compiler \
        -I${xdp2Target}/include \
        -i parser.c \
        -o parser.p.c

      # Build flow_parser binary
      $CC \
        -I${xdp2Target}/include \
        -I${targetPkgs.libpcap}/include \
        -L${xdp2Target}/lib \
        -L${targetPkgs.libpcap.lib}/lib \
        -Wl,-rpath,${xdp2Target}/lib \
        -Wl,-rpath,${targetPkgs.libpcap.lib}/lib \
        -g \
        -o flow_parser flow_parser.c parser.p.c \
        -lpcap -lxdp2 -lcli -lsiphash

      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall

      mkdir -p $out/bin
      mkdir -p $out/share/xdp2-samples/flow-tracker-combo

      install -m 755 flow_parser $out/bin/
      cp -r . $out/share/xdp2-samples/flow-tracker-combo/

      runHook postInstall
    '';

    meta = {
      description = "XDP2 flow_tracker_combo sample (pre-built)";
      platforms = lib.platforms.linux;
    };
  };

  # Define samples once to avoid rebuilding them in 'all'
  simpleParser = buildParserSample {
    name = "simple-parser";
    srcDir = srcRoot + "/samples/parser/simple_parser";
    targets = [ "parser_notmpl" "parser_tmpl" ];
  };

  offsetParser = buildParserSample {
    name = "offset-parser";
    srcDir = srcRoot + "/samples/parser/offset_parser";
    targets = [ "parser" ];
  };

  portsParser = buildParserSample {
    name = "ports-parser";
    srcDir = srcRoot + "/samples/parser/ports_parser";
    targets = [ "parser" ];
  };

in rec {
  # Parser samples
  simple-parser = simpleParser;
  offset-parser = offsetParser;
  ports-parser = portsParser;

  # XDP samples (userspace component only)
  flow-tracker-combo = buildFlowTrackerCombo;

  # Combined package with all samples (reuses existing derivations)
  all = targetPkgs.symlinkJoin {
    name = "xdp2-samples-all";
    paths = [
      simple-parser
      offset-parser
      ports-parser
      flow-tracker-combo
    ];
  };
}
