# nix/env-vars.nix
#
# Environment variable definitions for XDP2
#
# This module defines all environment variables needed for:
# - Compiler configuration (CC, CXX, HOST_CC, HOST_CXX)
# - LLVM/Clang paths (via llvmConfig)
# - Python configuration
# - Library paths
# - Build configuration
#
# Usage in flake.nix:
#   envVars = import ./nix/env-vars.nix {
#     inherit pkgs llvmConfig packages;
#     compilerConfig = { cc = pkgs.gcc; cxx = pkgs.gcc; ccBin = "gcc"; cxxBin = "g++"; };
#     configAgeWarningDays = 14;
#   };
#

{ pkgs
, llvmConfig
, packages
, compilerConfig
, configAgeWarningDays ? 14
}:

''
  # Compiler settings
  export CC=${compilerConfig.cc}/bin/${compilerConfig.ccBin}
  export CXX=${compilerConfig.cxx}/bin/${compilerConfig.cxxBin}
  export HOST_CC=${compilerConfig.cc}/bin/${compilerConfig.ccBin}
  export HOST_CXX=${compilerConfig.cxx}/bin/${compilerConfig.cxxBin}

  # LLVM/Clang environment variables (from llvmConfig module)
  # Sets: XDP2_CLANG_VERSION, XDP2_CLANG_RESOURCE_PATH, XDP2_C_INCLUDE_PATH,
  #       HOST_LLVM_CONFIG, LLVM_LIBS, CLANG_LIBS, LIBCLANG_PATH
  ${llvmConfig.envVars}

  # Glibc include path for xdp2-compiler (needed because libclang bypasses clang wrapper)
  export XDP2_GLIBC_INCLUDE_PATH="${pkgs.stdenv.cc.libc.dev}/include"

  # Linux kernel headers (provides <linux/types.h> etc.)
  export XDP2_LINUX_HEADERS_PATH="${pkgs.linuxHeaders}/include"

  # LD_LIBRARY_PATH for libclang
  export LD_LIBRARY_PATH=${llvmConfig.ldLibraryPath}:$LD_LIBRARY_PATH

  # Python environment
  export CFLAGS_PYTHON="$(pkg-config --cflags python3-embed)"
  export LDFLAGS_PYTHON="$(pkg-config --libs python3-embed)"
  export PYTHON_VER=3
  export PYTHONPATH="${pkgs.python3}/lib/python3.13/site-packages:$PYTHONPATH"

  # Boost libraries (boost_system is header-only since Boost 1.69)
  export BOOST_LIBS="-lboost_wave -lboost_thread -lboost_filesystem -lboost_program_options"

  # Other libraries
  export LIBS="-lpthread -ldl -lutil"
  export PATH_ARG=""

  # Build configuration
  export PKG_CONFIG_PATH=${pkgs.lib.makeSearchPath "lib/pkgconfig" packages.allPackages}
  export XDP2_COMPILER_DEBUG=1

  # Configuration management
  export CONFIG_AGE_WARNING_DAYS=${toString configAgeWarningDays}
''
