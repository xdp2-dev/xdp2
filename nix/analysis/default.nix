# nix/analysis/default.nix
#
# Static analysis infrastructure entry point for XDP2.
#
# Ported from the reference Nix project's analysis framework,
# adapted for XDP2's C codebase and Make-based build system.
#
# Provides 8 analysis tools at 3 levels:
#   quick:    clang-tidy + cppcheck
#   standard: + flawfinder, clang-analyzer, gcc-warnings
#   deep:     + gcc-analyzer, semgrep, sanitizers
#
# Usage:
#   nix build .#analysis-quick
#   nix build .#analysis-standard
#   nix build .#analysis-deep
#

{
  pkgs,
  lib,
  llvmConfig,
  packagesModule,
  src,
}:

let
  # ── Compilation database ────────────────────────────────────────

  compilationDb = import ./compile-db.nix {
    inherit lib pkgs llvmConfig;
    inherit (packagesModule) nativeBuildInputs buildInputs;
  };

  # ── Helper for tools that need compilation database ─────────────

  mkCompileDbReport = name: script:
    pkgs.runCommand "xdp2-analysis-${name}"
      {
        nativeBuildInputs = [ script ];
      }
      ''
        mkdir -p $out
        ${lib.getExe script} ${compilationDb} ${src} $out
      '';

  # ── Helper for tools that work on raw source ────────────────────

  mkSourceReport = name: script:
    pkgs.runCommand "xdp2-analysis-${name}"
      {
        nativeBuildInputs = [ script ];
      }
      ''
        mkdir -p $out
        ${lib.getExe script} ${src} $out
      '';

  # ── Individual tool targets ────────────────────────────────────

  clang-tidy = import ./clang-tidy.nix {
    inherit pkgs mkCompileDbReport;
  };

  cppcheck = import ./cppcheck.nix {
    inherit pkgs mkCompileDbReport;
  };

  flawfinder = import ./flawfinder.nix {
    inherit pkgs mkSourceReport;
  };

  semgrep = import ./semgrep.nix {
    inherit pkgs mkSourceReport;
  };

  gccTargets = import ./gcc.nix {
    inherit lib pkgs src llvmConfig;
    inherit (packagesModule) nativeBuildInputs buildInputs;
  };

  clang-analyzer = import ./clang-analyzer.nix {
    inherit lib pkgs src llvmConfig;
    inherit (packagesModule) nativeBuildInputs buildInputs;
  };

  sanitizers = import ./sanitizers.nix {
    inherit lib pkgs src llvmConfig;
    inherit (packagesModule) nativeBuildInputs buildInputs;
  };

  # ── Triage system path ──────────────────────────────────────────

  triagePath = "${src}/nix/analysis/triage";

  # ── Combined targets ───────────────────────────────────────────

  quick = pkgs.runCommand "xdp2-analysis-quick" { nativeBuildInputs = [ pkgs.python3 ]; } ''
    mkdir -p $out
    ln -s ${clang-tidy} $out/clang-tidy
    ln -s ${cppcheck} $out/cppcheck
    python3 ${triagePath} $out --output-dir $out/triage
    {
      echo "=== Analysis Summary (quick) ==="
      echo ""
      echo "clang-tidy:      $(cat ${clang-tidy}/count.txt) findings"
      echo "cppcheck:        $(cat ${cppcheck}/count.txt) findings"
      echo "triage:          $(cat $out/triage/count.txt) high-confidence findings"
      echo ""
      echo "Run 'nix build .#analysis-standard' for more thorough analysis."
    } > $out/summary.txt
    cat $out/summary.txt
  '';

  standard = pkgs.runCommand "xdp2-analysis-standard" { nativeBuildInputs = [ pkgs.python3 ]; } ''
    mkdir -p $out
    ln -s ${clang-tidy} $out/clang-tidy
    ln -s ${cppcheck} $out/cppcheck
    ln -s ${flawfinder} $out/flawfinder
    ln -s ${clang-analyzer} $out/clang-analyzer
    ln -s ${gccTargets.gcc-warnings} $out/gcc-warnings
    python3 ${triagePath} $out --output-dir $out/triage
    {
      echo "=== Analysis Summary (standard) ==="
      echo ""
      echo "clang-tidy:      $(cat ${clang-tidy}/count.txt) findings"
      echo "cppcheck:        $(cat ${cppcheck}/count.txt) findings"
      echo "flawfinder:      $(cat ${flawfinder}/count.txt) findings"
      echo "clang-analyzer:  $(cat ${clang-analyzer}/count.txt) findings"
      echo "gcc-warnings:    $(cat ${gccTargets.gcc-warnings}/count.txt) findings"
      echo "triage:          $(cat $out/triage/count.txt) high-confidence findings"
      echo ""
      echo "Run 'nix build .#analysis-deep' for full analysis including"
      echo "GCC -fanalyzer, semgrep, and sanitizer builds."
    } > $out/summary.txt
    cat $out/summary.txt
  '';

  deep = pkgs.runCommand "xdp2-analysis-deep" { nativeBuildInputs = [ pkgs.python3 ]; } ''
    mkdir -p $out
    ln -s ${clang-tidy} $out/clang-tidy
    ln -s ${cppcheck} $out/cppcheck
    ln -s ${flawfinder} $out/flawfinder
    ln -s ${clang-analyzer} $out/clang-analyzer
    ln -s ${gccTargets.gcc-warnings} $out/gcc-warnings
    ln -s ${gccTargets.gcc-analyzer} $out/gcc-analyzer
    ln -s ${semgrep} $out/semgrep
    ln -s ${sanitizers} $out/sanitizers
    python3 ${triagePath} $out --output-dir $out/triage
    {
      echo "=== Analysis Summary (deep) ==="
      echo ""
      echo "clang-tidy:      $(cat ${clang-tidy}/count.txt) findings"
      echo "cppcheck:        $(cat ${cppcheck}/count.txt) findings"
      echo "flawfinder:      $(cat ${flawfinder}/count.txt) findings"
      echo "clang-analyzer:  $(cat ${clang-analyzer}/count.txt) findings"
      echo "gcc-warnings:    $(cat ${gccTargets.gcc-warnings}/count.txt) findings"
      echo "gcc-analyzer:    $(cat ${gccTargets.gcc-analyzer}/count.txt) findings"
      echo "semgrep:         $(cat ${semgrep}/count.txt) findings"
      echo "sanitizers:      $(cat ${sanitizers}/count.txt) findings"
      echo "triage:          $(cat $out/triage/count.txt) high-confidence findings"
      echo ""
      echo "All analysis tools completed."
    } > $out/summary.txt
    cat $out/summary.txt
  '';

in
{
  inherit
    clang-tidy
    cppcheck
    flawfinder
    clang-analyzer
    semgrep
    sanitizers
    quick
    standard
    deep
    ;
  inherit (gccTargets) gcc-warnings gcc-analyzer;
}
