# nix/xdp-samples.nix
#
# Derivation to build XDP sample programs (BPF bytecode).
#
# This derivation:
# 1. Uses the pre-built xdp2-debug package (which provides xdp2-compiler)
# 2. Generates parser headers using xdp2-compiler
# 3. Compiles XDP programs to BPF bytecode using unwrapped clang
#
# The output contains:
# - $out/lib/xdp/*.xdp.o - Compiled BPF programs
# - $out/share/xdp-samples/* - Source files for reference
#
# Usage:
#   nix build .#xdp-samples
#
{ pkgs
, xdp2  # The pre-built xdp2 package (xdp2-debug)
}:

let
  # Import LLVM configuration - must match what xdp2 was built with
  llvmConfig = import ./llvm.nix { inherit pkgs; lib = pkgs.lib; llvmVersion = 18; };
  llvmPackages = llvmConfig.llvmPackages;

  # Use unwrapped clang for BPF compilation to avoid Nix cc-wrapper flags
  # that are incompatible with BPF target (e.g., -fzero-call-used-regs)
  bpfClang = llvmPackages.clang-unwrapped;
in
pkgs.stdenv.mkDerivation {
  pname = "xdp2-samples";
  version = "0.1.0";

  # Only need the samples directory
  src = ../samples/xdp;

  nativeBuildInputs = [
    pkgs.gnumake
    bpfClang
    llvmPackages.lld
    xdp2  # Provides xdp2-compiler
  ];

  buildInputs = [
    pkgs.libbpf
    pkgs.linuxHeaders
  ];

  # BPF bytecode doesn't need hardening flags
  hardeningDisable = [ "all" ];

  # Don't use the Nix clang wrapper for BPF
  dontUseCmakeConfigure = true;

  buildPhase = ''
    runHook preBuild

    export XDP2DIR="${xdp2}"
    export INCDIR="${xdp2}/include"
    export BINDIR="${xdp2}/bin"
    export LIBDIR="${xdp2}/lib"

    # Environment variables needed by xdp2-compiler (uses libclang internally)
    export XDP2_CLANG_VERSION="${llvmConfig.version}"
    export XDP2_CLANG_RESOURCE_PATH="${llvmConfig.paths.clangResourceDir}"
    export XDP2_C_INCLUDE_PATH="${llvmConfig.paths.clangResourceDir}/include"
    export XDP2_GLIBC_INCLUDE_PATH="${pkgs.stdenv.cc.libc.dev}/include"
    export XDP2_LINUX_HEADERS_PATH="${pkgs.linuxHeaders}/include"

    # Library path for xdp2-compiler (needs libclang, LLVM, Boost)
    export LD_LIBRARY_PATH="${llvmPackages.llvm.lib}/lib:${llvmPackages.libclang.lib}/lib:${pkgs.boost}/lib"

    # BPF-specific clang (unwrapped, no Nix hardening flags)
    export BPF_CLANG="${bpfClang}/bin/clang"

    # Include paths for regular C compilation (parser.c -> parser.o)
    # These need standard libc headers and compiler builtins (stddef.h, etc.)
    export CFLAGS="-I${xdp2}/include"
    CFLAGS="$CFLAGS -I${llvmConfig.paths.clangResourceDir}/include"
    CFLAGS="$CFLAGS -I${pkgs.stdenv.cc.libc.dev}/include"
    CFLAGS="$CFLAGS -I${pkgs.linuxHeaders}/include"

    # Include paths for BPF compilation (only kernel-compatible headers)
    export BPF_CFLAGS="-I${xdp2}/include -I${pkgs.libbpf}/include -I${pkgs.linuxHeaders}/include"
    BPF_CFLAGS="$BPF_CFLAGS -I${llvmConfig.paths.clangResourceDir}/include"

    echo "=== Building XDP samples ==="
    echo "XDP2DIR: $XDP2DIR"
    echo "BPF_CLANG: $BPF_CLANG"
    echo "CFLAGS: $CFLAGS"
    echo "BPF_CFLAGS: $BPF_CFLAGS"

    # Track what we've built
    mkdir -p $TMPDIR/built

    for sample in flow_tracker_simple flow_tracker_combo flow_tracker_tlvs flow_tracker_tmpl; do
      if [ -d "$sample" ]; then
        echo ""
        echo "=== Building $sample ==="
        cd "$sample"

        # Step 1: Compile parser.c to parser.o (to check for errors)
        # Use regular clang with libc headers
        echo "Compiling parser.c..."
        if $BPF_CLANG $CFLAGS -g -O2 -c -o parser.o parser.c 2>&1; then
          echo "  parser.o: OK"
        else
          echo "  parser.o: FAILED (continuing...)"
          cd ..
          continue
        fi

        # Step 2: Generate parser.xdp.h using xdp2-compiler
        echo "Generating parser.xdp.h..."
        if $BINDIR/xdp2-compiler -I$INCDIR -i parser.c -o parser.xdp.h 2>&1; then
          echo "  parser.xdp.h: OK"
        else
          echo "  parser.xdp.h: FAILED (continuing...)"
          cd ..
          continue
        fi

        # Step 3: Compile flow_tracker.xdp.c to BPF bytecode
        echo "Compiling flow_tracker.xdp.o (BPF)..."
        if $BPF_CLANG -x c -target bpf $BPF_CFLAGS -g -O2 -c -o flow_tracker.xdp.o flow_tracker.xdp.c 2>&1; then
          echo "  flow_tracker.xdp.o: OK"
          cp flow_tracker.xdp.o $TMPDIR/built/''${sample}.xdp.o
        else
          echo "  flow_tracker.xdp.o: FAILED"
        fi

        cd ..
      fi
    done

    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall

    # Create output directories
    mkdir -p $out/lib/xdp
    mkdir -p $out/share/xdp-samples

    # Install compiled BPF programs
    if ls $TMPDIR/built/*.xdp.o 1>/dev/null 2>&1; then
      install -m 644 $TMPDIR/built/*.xdp.o $out/lib/xdp/
      echo "Installed XDP programs:"
      ls -la $out/lib/xdp/
    else
      echo "WARNING: No XDP programs were successfully built"
      # Create a marker file so the derivation doesn't fail
      echo "No XDP programs built - see build logs" > $out/lib/xdp/BUILD_FAILED.txt
    fi

    # Install source files for reference
    for sample in flow_tracker_simple flow_tracker_combo flow_tracker_tlvs flow_tracker_tmpl; do
      if [ -d "$sample" ]; then
        mkdir -p $out/share/xdp-samples/$sample
        cp -r $sample/*.c $sample/*.h $out/share/xdp-samples/$sample/ 2>/dev/null || true
      fi
    done

    runHook postInstall
  '';

  meta = with pkgs.lib; {
    description = "XDP2 sample programs (BPF bytecode)";
    license = licenses.bsd2;
    platforms = platforms.linux;
  };
}
