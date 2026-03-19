# nix/analysis/clang-tidy.nix
#
# clang-tidy runner for XDP2's C codebase.
#
# Adapted from the reference C++ implementation:
# - Finds .c and .h files instead of .cc
# - Uses C-appropriate checks (no cppcoreguidelines, modernize)
# - No custom plugin (nixTidyChecks not applicable to XDP2)
#

{
  pkgs,
  mkCompileDbReport,
}:

let
  runner = pkgs.writeShellApplication {
    name = "run-clang-tidy-analysis";
    runtimeInputs = with pkgs; [
      clang-tools
      coreutils
      findutils
      gnugrep
    ];
    text = ''
      compile_db="$1"
      source_dir="$2"
      output_dir="$3"

      echo "=== clang-tidy Analysis (C) ==="
      echo "Using compilation database: $compile_db"

      # Find all .c source files in library and tool directories
      find "$source_dir/src" -name '*.c' -not -path '*/test*' -print0 | \
        xargs -0 -P "$(nproc)" -I{} \
          clang-tidy \
            -p "$compile_db" \
            --header-filter='src/.*' \
            --checks='-*,bugprone-*,cert-*,clang-analyzer-*,misc-*,readability-*' \
            {} \
        > "$output_dir/report.txt" 2>&1 || true

      findings=$(grep -c ': warning:\|: error:' "$output_dir/report.txt" || echo "0")
      echo "$findings" > "$output_dir/count.txt"
    '';
  };
in
mkCompileDbReport "clang-tidy" runner
