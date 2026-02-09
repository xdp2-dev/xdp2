# nix/llvm.nix
#
# LLVM/Clang configuration for XDP2
#
# This module centralizes all LLVM and Clang configuration:
# - llvmPackages selection (configurable, defaults to pkgs.llvmPackages)
# - llvm-config wrapper for correct include/lib paths
# - Environment variables for LLVM tools
# - Paths for use with substituteInPlace
#
# Usage in flake.nix:
#   llvmConfig = import ./nix/llvm.nix { inherit pkgs lib; };
#   # Or with custom version:
#   llvmConfig = import ./nix/llvm.nix { inherit pkgs lib; llvmVersion = 19; };
#

{ pkgs
, lib
, llvmVersion ? null  # null means use default llvmPackages
}:

let
  # Select llvmPackages based on version parameter
  # If no version specified, use the default llvmPackages
  llvmPackages =
    if llvmVersion == null then
      pkgs.llvmPackages
    else
      pkgs."llvmPackages_${toString llvmVersion}";

  # Extract major version from LLVM version (e.g., "18.1.8" -> "18")
  llvmMajorVersion = lib.versions.major llvmPackages.llvm.version;

  # Create a wrapper for llvm-config to include clang paths (for libclang)
  # This is needed because the xdp2-compiler uses libclang and needs correct paths
  llvm-config-wrapped = pkgs.runCommand "llvm-config-wrapped" { } ''
    mkdir -p $out/bin
    cat > $out/bin/llvm-config <<EOF
#!/${pkgs.bash}/bin/bash
if [[ "\$1" == "--includedir" ]]; then
  echo "${llvmPackages.clang-unwrapped.dev}/include"
elif [[ "\$1" == "--libdir" ]]; then
  echo "${lib.getLib llvmPackages.clang-unwrapped}/lib"
else
  ${llvmPackages.llvm.dev}/bin/llvm-config "\$@"
fi
EOF
    chmod +x $out/bin/llvm-config
  '';

in
{
  # Export the selected llvmPackages for use in other modules
  inherit llvmPackages;

  # Export the wrapper
  inherit llvm-config-wrapped;

  # LLVM version info
  version = llvmPackages.llvm.version;

  # Paths for direct use or substituteInPlace
  paths = {
    llvmConfig = "${llvmPackages.llvm.dev}/bin/llvm-config";
    llvmConfigWrapped = "${llvm-config-wrapped}/bin/llvm-config";
    clangBin = "${llvmPackages.clang}/bin/clang";
    clangxxBin = "${llvmPackages.clang}/bin/clang++";
    clangInclude = "${llvmPackages.clang-unwrapped.dev}/include";
    # Clang resource directory - use the wrapper's resource-root which has the complete structure
    # (include, lib, share) matching Ubuntu's /usr/lib/llvm-<ver>/lib/clang/<ver>
    # The libclang.lib path is incomplete (only has include, missing lib and share)
    clangResourceDir = "${llvmPackages.clang}/resource-root";
    llvmLib = "${llvmPackages.llvm}/lib";
    libclangLib = "${llvmPackages.libclang.lib}/lib";
  };

  # Environment variable exports (as shell script fragment)
  envVars = ''
    # LLVM/Clang version
    export XDP2_CLANG_VERSION="$(${llvmPackages.llvm.dev}/bin/llvm-config --version)"
    # Clang resource directory - use the wrapper's resource-root which has complete structure
    # matching Ubuntu's /usr/lib/llvm-<ver>/lib/clang/<ver>
    export XDP2_CLANG_RESOURCE_PATH="${llvmPackages.clang}/resource-root"
    # C include path for clang headers
    export XDP2_C_INCLUDE_PATH="${llvmPackages.clang}/resource-root/include"

    # LLVM/Clang settings
    export HOST_LLVM_CONFIG="${llvm-config-wrapped}/bin/llvm-config"
    export LLVM_LIBS="-L${llvmPackages.llvm}/lib"
    export CLANG_LIBS="-lclang -lLLVM -lclang-cpp"

    # libclang configuration
    export LIBCLANG_PATH="${llvmPackages.libclang.lib}/lib"
  '';

  # LD_LIBRARY_PATH addition (separate to allow conditional use)
  ldLibraryPath = "${llvmPackages.libclang.lib}/lib";
}
