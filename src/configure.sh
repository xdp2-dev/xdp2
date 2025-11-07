#!/bin/bash
#
# configure.sh
#
# configure.sh is an experimental version of ./configure, which has some tweaks
# Major changes including:
# - supporting detection of different versions of clang on ubuntu systems
# - adjustments to the c++ embedded code to support clang18 and clang20
#
# This is not autoconf generated
#
INCLUDE=${1:-"$PWD/include"}

# Output file which is input to Makefile
CONFIG=config.mk
# Make a temp directory in build tree
TMPDIR=$(mktemp -d config.XXXXXX)
trap 'status=$?; rm -rf $TMPDIR; exit $status' EXIT HUP INT QUIT TERM

# Global variable to store discovered clang libraries
CLANG_LIBS_DISCOVERED=""

# Debug function for progressive debugging (0-7, like syslog)
# Level 0: No debug output (default)
# Level 1-2: Basic information (detected tools, paths)
# Level 3-4: Command execution details
# Level 5-6: Full command output, environment variables
# Level 7: Maximum verbosity (all intermediate steps, test program contents)
debug_print() {
	local level=$1
	shift
	if [ "${CONFIGURE_DEBUG_LEVEL:-0}" -ge "$level" ]; then
		echo "[DEBUG-$level] $*" >&2
	fi
}

check_prog()
{
    echo -n "$2"
    # fails shellcheck SC2015
    command -v "$1" >/dev/null 2>&1 && (echo "$3:=y" >> $CONFIG; echo "yes") ||
					(echo "no"; return 1)
}

check_libpcap()
{
	cat >"$TMPDIR"/pcaptest.c <<EOF
#include <pcap.h>
int main(int argc, char **argv)
{
	pcap_t *p;
	char errbuf[PCAP_ERRBUF_SIZE];

	p = pcap_open_offline("foo", &errbuf[0]);
	pcap_close(p);
	return (0);
}
EOF
	$CC_GCC -o "$TMPDIR"/pcaptest "$TMPDIR"/pcaptest.c 	\
				-lpcap > /dev/null 2>"$TMPDIR"/pcaplog

	case $? in
		0)	;;
		*)	echo libpcap missing or broken\! 1>&2
			echo ERROR LOG:
			cat "$TMPDIR"/pcaplog
			exit 1
			;;
	esac
	rm -f "$TMPDIR"/pcaptest.c "$TMPDIR"/pcaptest
}

check_boostwave()
{
	cat >"$TMPDIR"/wavetest.cpp <<EOF
#include <boost/wave.hpp>

struct test : boost::wave::context_policies::default_preprocessing_hooks {
};

int main(int argc, char **argv)
{
	return (0);
}
EOF
        $HOST_CXX -o "$TMPDIR"/wavetest "$TMPDIR"/wavetest.cpp 		\
						-lboost_system -lboost_wave
	case $? in
		0)	;;
		*)	echo Boost.Wave missing or broken\! 1>&2
			exit 1
			;;
	esac
	rm -f "$TMPDIR"/wavetest.cpp "$TMPDIR"/wavetest
}

check_boostthread()
{
	cat >"$TMPDIR"/threadtest.cpp <<EOF
#include <boost/thread.hpp>

int main(int argc, char **argv)
{
	{
		boost::mutex m;
	}
	return (0);
}
EOF
	$HOST_CXX -o "$TMPDIR"/threadtest "$TMPDIR"/threadtest.cpp		\
			-lboost_thread   -lboost_system >/dev/null 2>&1
	case $? in
		0)	;;
		*)	echo Boost.Thread missing or broken\! 1>&2
			exit 1
			;;
	esac
	rm -f "$TMPDIR"/threadtest.cpp "$TMPDIR"/threadtest
}

check_boostsystem()
{
	debug_print 6 "Boost.System: Starting check"
	cat >"$TMPDIR"/systemtest.cpp <<EOF
#include <boost/system/error_code.hpp>

int main(int argc, char **argv)
{
	{
		boost::system::error_code ec;
	}
	return (0);
}
EOF
	if [ "${CONFIGURE_DEBUG_LEVEL:-0}" -ge 5 ]; then
		$HOST_CXX -o "$TMPDIR"/systemtest "$TMPDIR"/systemtest.cpp -lboost_system
		COMPILE_EXIT=$?
	else
		$HOST_CXX -o "$TMPDIR"/systemtest "$TMPDIR"/systemtest.cpp		\
						-lboost_system > /dev/null 2>&1
		COMPILE_EXIT=$?
	fi
	case $COMPILE_EXIT in
		0)
			debug_print 6 "Boost.System: Check PASSED"
			;;
		*)
			debug_print 6 "Boost.System: Check FAILED (exit code: $COMPILE_EXIT)"
			echo Boost.System missing or broken\! 1>&2
			exit 1
			;;
	esac
	rm -f "$TMPDIR"/systemtest.cpp "$TMPDIR"/systemtest
}

check_boostfilesystem()
{
	cat >"$TMPDIR"/filesystemtest.cpp <<EOF
#include <boost/filesystem/path.hpp>

int main(int argc, char **argv)
{
	{
		boost::filesystem::path p;
	}
	return (0);
}
EOF
	$HOST_CXX -o "$TMPDIR"/filesystemtest "$TMPDIR"/filesystemtest.cpp	\
			-lboost_system -lboost_filesystem > /dev/null 2>&1
	case $? in
		0)	;;
		*)	echo Boost.Filesystem missing or broken\! 1>&2
			exit 1
			;;
	esac
	rm -f "$TMPDIR"/filesystemtest.cpp "$TMPDIR"/filesystemtest
}

check_clang_lib()
{
	debug_print 1 "Clang.Lib: Starting check"
	debug_print 4 "Clang.Lib: HOST_CXX=$HOST_CXX"
	debug_print 4 "Clang.Lib: HOST_LLVM_CONFIG=$HOST_LLVM_CONFIG"

	cat > "$TMPDIR"/clang_lib.cpp <<EOF
#include <clang/Frontend/CompilerInstance.h>
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "clang/Lex/PreprocessorOptions.h"

static llvm::cl::OptionCategory MyToolCategory("my-tool options");

int main(int argc, const char **argv)
{
  // Define our own category to be compatible with all Clang versions,
  // avoiding issues with getClangSyntaxOnlyCategory/getSyntaxOnlyToolCategory.
  static llvm::cl::OptionCategory TestCategory("Test Tool");
  auto ExpectedParser = clang::tooling::CommonOptionsParser::create(argc, argv, TestCategory);
  return 0;
}
EOF

	debug_print 7 "Clang.Lib: Test program created at $TMPDIR/clang_lib.cpp"
	if [ "${CONFIGURE_DEBUG_LEVEL:-0}" -ge 7 ]; then
		debug_print 7 "Clang.Lib: Test program contents:"
		cat "$TMPDIR"/clang_lib.cpp >&2
	fi

	# Test if llvm-config exists and works before using it
	if ! command -v "$HOST_LLVM_CONFIG" >/dev/null 2>&1 && [ ! -f "$HOST_LLVM_CONFIG" ]; then
		debug_print 1 "Clang.Lib: Error: $HOST_LLVM_CONFIG not found!"
		echo "Error: $HOST_LLVM_CONFIG not found!" 1>&2
		echo "Clang library missing or broken! (Failed in check_clang_lib() #1)" 1>&2
		exit 1
	fi

	# Get llvm-config flags
	LLVM_LDFLAGS=`$HOST_LLVM_CONFIG --ldflags 2>&1`
	LLVM_CXXFLAGS=`$HOST_LLVM_CONFIG --cxxflags 2>&1`
	LLVM_LIBDIR=`$HOST_LLVM_CONFIG --libdir 2>&1`
	debug_print 4 "Clang.Lib: llvm-config --ldflags: $LLVM_LDFLAGS"
	debug_print 4 "Clang.Lib: llvm-config --cxxflags: $LLVM_CXXFLAGS"
	debug_print 4 "Clang.Lib: llvm-config --libdir: $LLVM_LIBDIR"
	LLVM_LIBS=`$HOST_LLVM_CONFIG --libs 2>/dev/null`
	debug_print 4 "Clang.Lib: llvm-config --libs: $LLVM_LIBS"

	# Discover clang libraries needed for the test program
	# The test program uses clang::tooling::CommonOptionsParser, so we need:
	# 1. clang-cpp (C++ interface library) - shared library preferred
	# 2. clangTooling (tooling functionality) - exact match libclangTooling.a/so
	CLANG_LIBS_FOUND=""

	if [ -d "$LLVM_LIBDIR" ]; then
		debug_print 5 "Clang.Lib: Scanning $LLVM_LIBDIR for required clang libraries..."

		# Find clang-cpp shared library - look for exact name first, then versioned
		# Try exact match first: libclang-cpp.so
		CLANG_CPP_LIB=""
		if [ -f "$LLVM_LIBDIR/libclang-cpp.so" ] || [ -L "$LLVM_LIBDIR/libclang-cpp.so" ]; then
			CLANG_CPP_LIB="$LLVM_LIBDIR/libclang-cpp.so"
		else
			# Find versioned: libclang-cpp.so.18.1 or libclang-cpp.so.18
			# Include both regular files and symlinks
			CLANG_CPP_LIB=$(find "$LLVM_LIBDIR" -maxdepth 1 -name "libclang-cpp.so*" \( -type f -o -type l \) 2>/dev/null | sort -V | head -1)
		fi

		if [ -n "$CLANG_CPP_LIB" ] && { [ -f "$CLANG_CPP_LIB" ] || [ -L "$CLANG_CPP_LIB" ]; }; then
			# Check if there's a symlink libclang-cpp.so (without version)
			if [ -L "$LLVM_LIBDIR/libclang-cpp.so" ] || [ -f "$LLVM_LIBDIR/libclang-cpp.so" ]; then
				# Use -l flag if symlink exists
				CLANG_LIBS_FOUND="$CLANG_LIBS_FOUND -lclang-cpp"
				debug_print 4 "Clang.Lib: Found clang-cpp: $(basename $CLANG_CPP_LIB) -> -lclang-cpp (via symlink)"
			else
				# Use full path if no symlink exists
				CLANG_LIBS_FOUND="$CLANG_LIBS_FOUND $CLANG_CPP_LIB"
				debug_print 4 "Clang.Lib: Found clang-cpp: $(basename $CLANG_CPP_LIB) -> using full path"
			fi
		else
			debug_print 2 "Clang.Lib: Warning: clang-cpp library not found in $LLVM_LIBDIR"
		fi

		# Find clangTooling library - must be exact match libclangTooling.a or .so
		# This provides CommonOptionsParser and other tooling functionality
		CLANG_TOOLING_LIB=""
		if [ -f "$LLVM_LIBDIR/libclangTooling.so" ]; then
			CLANG_TOOLING_LIB="$LLVM_LIBDIR/libclangTooling.so"
		elif [ -f "$LLVM_LIBDIR/libclangTooling.a" ]; then
			CLANG_TOOLING_LIB="$LLVM_LIBDIR/libclangTooling.a"
		fi

		if [ -n "$CLANG_TOOLING_LIB" ] && [ -f "$CLANG_TOOLING_LIB" ]; then
			# Static libraries can use -l flag, shared libraries too if symlink exists
			if echo "$CLANG_TOOLING_LIB" | grep -q "\.a$"; then
				# Static library - use -l flag
				CLANG_LIBS_FOUND="$CLANG_LIBS_FOUND -lclangTooling"
				debug_print 4 "Clang.Lib: Found clangTooling: $(basename $CLANG_TOOLING_LIB) -> -lclangTooling"
			else
				# Shared library - check for symlink
				if [ -L "$LLVM_LIBDIR/libclangTooling.so" ] || [ -f "$LLVM_LIBDIR/libclangTooling.so" ]; then
					CLANG_LIBS_FOUND="$CLANG_LIBS_FOUND -lclangTooling"
					debug_print 4 "Clang.Lib: Found clangTooling: $(basename $CLANG_TOOLING_LIB) -> -lclangTooling"
				else
					CLANG_LIBS_FOUND="$CLANG_LIBS_FOUND $CLANG_TOOLING_LIB"
					debug_print 4 "Clang.Lib: Found clangTooling: $(basename $CLANG_TOOLING_LIB) -> using full path"
				fi
			fi
		fi

		# Clean up the library list
		CLANG_LIBS_FOUND=$(echo "$CLANG_LIBS_FOUND" | sed 's/^ *//;s/ *$//')

		if [ -n "$CLANG_LIBS_FOUND" ]; then
			debug_print 3 "Clang.Lib: Discovered clang libraries: $CLANG_LIBS_FOUND"
		else
			debug_print 2 "Clang.Lib: Required clang libraries not found in $LLVM_LIBDIR"
		fi
	fi

	# Create a temporary output file
	TMPO=$(mktemp "$TMPDIR/clang_lib.XXXXXX")

	# Build compilation command with discovered libraries
	ALL_LIBS="$LLVM_LIBS $CLANG_LIBS_FOUND"
	CMD="$HOST_CXX -o $TMPO $TMPDIR/clang_lib.cpp $LLVM_LDFLAGS $LLVM_CXXFLAGS $ALL_LIBS"

	debug_print 3 "Clang.Lib: Attempting link with discovered libs: $CLANG_LIBS_FOUND"
	debug_print 5 "Clang.Lib: Full command: $CMD"

	# Try compilation
	if [ "${CONFIGURE_DEBUG_LEVEL:-0}" -ge 5 ]; then
		$CMD
		COMPILE_EXIT=$?
	else
		$CMD >/dev/null 2>&1
		COMPILE_EXIT=$?
	fi

	if [ $COMPILE_EXIT -eq 0 ]; then
		debug_print 1 "Clang.Lib: Check PASSED with libraries: $CLANG_LIBS_FOUND"
		# Store discovered libraries in global variable for writing to config.mk
		# Include LLVM library flag from llvm-config --libs along with discovered clang libraries
		CLANG_LIBS_DISCOVERED="$LLVM_LIBS $CLANG_LIBS_FOUND"
		debug_print 3 "Clang.Lib: Stored CLANG_LIBS_DISCOVERED: $CLANG_LIBS_DISCOVERED"
		rm -f "$TMPDIR"/clang_lib.cpp "$TMPO"
		return 0
	fi

	# If compilation failed, exit with error
	debug_print 1 "Clang.Lib: Check FAILED (exit code: $COMPILE_EXIT)"
	debug_print 3 "Clang.Lib: Attempted libraries: $CLANG_LIBS_FOUND"
	debug_print 3 "Clang.Lib: Library directory: $LLVM_LIBDIR"
	if [ "${CONFIGURE_DEBUG_LEVEL:-0}" -ge 3 ]; then
		debug_print 3 "Clang.Lib: Available clang libraries in $LLVM_LIBDIR:"
		ls -1 "$LLVM_LIBDIR"/libclang* 2>/dev/null | sed 's/^/  /' >&2 || echo "  (none found)" >&2
	fi
	echo "Clang library missing or broken!" 1>&2
	echo "Failed command was: $CMD" 1>&2
	exit 1

	#unreachable shellcheck SC2317?
	rm -f "$TMPDIR"/clang_lib.cpp "$TMPDIR"/clang
}

check_python()
{
	cat >"$TMPDIR"/check_python.cpp <<EOF
#include <Python.h>

int main(int argc, char **argv)
{
	return (0);
}
EOF
		$HOST_CXX -o "$TMPDIR"/check_python "$TMPDIR"/check_python.cpp	\
			`$PKG_CONFIG --cflags --libs python3-embed`
	case $? in
		0)	;;
		*)	echo Python missing or broken\! 1>&2
			exit 1
			;;
	esac
	rm -f "$TMPDIR"/check_python.cpp "$TMPDIR"/check_python
}

check_scapy()
{
	python3 -c "import scapy.all; print('scapy OK')" >/dev/null 2>&1
	case $? in
		0)	echo "HAVE_SCAPY:=y" >> $CONFIG
			;;
		*)	echo "ERROR: scapy is required but not found!" 1>&2
			echo "Please install scapy: ( pip3 install scapy | apt install python3-scapy )" 1>&2
			exit 1
			;;
	esac
}

check_cross_compiler_environment()
{
	if [ ! -d "$DEF_CC_ENV_LOC" ]; then
		echo "$DEF_CC_ENV_LOC is not found!"
		# SC2242 shellcheck
		exit -1
	fi

	if [ ! -d "$SYSROOT_LOC" ]; then
		echo "$SYSROOT_LOC is not found!"
		# SC2242 shellcheck
		exit -1
	fi

	if [ ! -d "$CC_ENV_TOOLCHAIN" ]; then
		echo "$CC_ENV_TOOLCHAIN is not found!"
		# SC2242 shellcheck
		exit -1
	fi
}

quiet_config()
{
	cat <<EOF
# user can control verbosity similar to kernel builds (e.g., V=1)
ifeq ("\$(origin V)", "command line")
	VERBOSE = \$(V)
endif
ifndef VERBOSE
	VERBOSE = 0
endif
ifeq (\$(VERBOSE),1)
	Q =
else
	Q = @
endif

ifeq (\$(VERBOSE), 0)
	QUIET_EMBED    = @echo '    EMBED    '\$@;
	QUIET_CC       = @echo '    CC       '\$@;
	QUIET_CXX      = @echo '    CXX      '\$@;
	QUIET_AR       = @echo '    AR       '\$@;
	QUIET_ASM      = @echo '    ASM      '\$@;
	QUIET_XDP2     = @echo '    XDP2    '\$@;
	QUIET_LINK     = @echo '    LINK     '\$@;
	QUIET_INSTALL  = @echo '    INSTALL  '\$(TARGETS);
endif
EOF
}

usage_platforms()
{
	echo "Usage $0 [--platform { $1 } ] [ <platform_paramters> ]"
}

PLATFORMS=($(ls ../platforms))

PLATFORM="default"

if [ "$1" == "--platform" ]; then
	PLATFORM=$2
	PLATFORM_TEXT=$2
	shift 2
fi

for i in ${PLATFORMS[@]}; do
	if [ "$PLATFORM" == "$i" ]; then
		FOUND_PLAT="true"
	fi
done

if [ "$FOUND_PLAT" != "true" ]; then
	usage_platforms "${PLATFORMS[*]}"
	exit
fi

usage()
{
	echo -n "Usage: $0"
	if [ -n "$PLATFORM_TEXT" ]; then
		echo -n " --platform $PLATFORM_TEXT"
	fi

	echo " [--config-defines <defines>]"
	echo " [--ccarch <arch>]"
	echo " [--arch <arch>]"
	echo " [--compiler <compiler>]"
	echo " [--installdir <dir>]"
	echo " [--build-opt-parser]"
	echo " [--build-parser-json]"
	echo " [--no-build-compiler]"
	echo " [--pkg-config-path <path>]"
	echo " [--llvm-config <llvm-config>]"
	echo " [--debug-level <0-7>]"
	echo ""
	echo " ( also VERBOSE like kernel builds, see also quiet_config() )"

	platform_usage

	exit 1
}

source ../platforms/"$PLATFORM"/src/configure

init_platform

COMPILER="gcc"

# Show debug level if set
if [ -n "${CONFIGURE_DEBUG_LEVEL}" ]; then
	debug_print 1 "Debug Level: ${CONFIGURE_DEBUG_LEVEL}"
fi

	while [ -n "$1" ]; do
	case $1 in
		"--config-defines") CONFIG_DEFINES=$2; shift;;
		"--ccarch") TARGET_ARCH=$2; shift;;
		"--arch") ARCH=$2; shift;;
		"--compiler") COMPILER=$2; shift;;
		"--pkg-config-path") MY_PKG_CONFIG_PATH=$2; shift;;
		"--installdir") INSTALLDIR=$2; shift;;
		"--build-opt-parser") BUILD_OPT_PARSER="y";;
		"--build-parser-json") BUILD_PARSER_JSON="y";;
		"--no-build-compiler") NO_BUILD_COMPILER="y";;
		"--llvm-config") HOST_LLVM_CONFIG=$2; shift;;
		"--debug-level") CONFIGURE_DEBUG_LEVEL=$2; shift;;
		*) parse_platform_opts $1;;
	esac
	shift
done

if [ -n "$MY_PKG_CONFIG_PATH" ]; then
	if [ -n "$PKG_CONFIG_PATH" ]; then
		export PKG_CONFIG_PATH="$MY_PKG_CONFIG_PATH:$PKG_CONFIG_PATH"
	else
		export PKG_CONFIG_PATH="$MY_PKG_CONFIG_PATH"
	fi
fi

if [ "$NO_BUILD_COMPILER" == "y" ]; then
	if [ "$BUILD_OPT_PARSER" == "y" ]; then
		echo -n "No build compiler and optimized parser cannot be "
		echo "configured at the same time"
		# SC2242 shellcheck
		exit -1
	fi
	if [ "$BUILD_PARSER_JSON" == "y" ]; then
		echo -n "No build compiler an build parser .json cannot be "
		echo "configured at the same time"
		# SC2242 shellcheck
		exit -1
	fi
fi

echo "# Generated config based on" $INCLUDE >$CONFIG
echo "ifneq (\$(TOP_LEVEL_MAKE),y)" >> $CONFIG
quiet_config >> $CONFIG

if [ -n "$PKG_CONFIG_PATH" ]; then
	echo "PKG_CONFIG_PATH=$PKG_CONFIG_PATH" >> $CONFIG
	# PATH_ARG is empty because pkg-config automatically uses PKG_CONFIG_PATH from environment
	echo "PATH_ARG=\"\"" >> $CONFIG
else
	echo "PATH_ARG=\"\"" >> $CONFIG
fi

echo -n 'CFLAGS_PYTHON=`$(PKG_CONFIG) $(PATH_ARG)' >> $CONFIG
echo ' --cflags python3-embed`' >> $CONFIG
echo -n 'LDFLAGS_PYTHON=`$(PKG_CONFIG) $(PATH_ARG) --libs' >> $CONFIG
echo ' python3-embed`' >> $CONFIG
echo 'CAT=cat' >> $CONFIG

# Set up architecture variables
LLVM_CONFIG="llvm-config"
CC_GCC="gcc"
if [ "$COMPILER" == "clang" ]; then
	CC_GCC="clang"
fi
CC_CXX="g++"
CC_AR="ar"
CFLAGS_N=""
LIBS_N=""
LIBS_NL=""

if [ -z "$ARCH" ]; then
	# Architecture was not set from command line, try to determine

	if [ -n "$TARGET_ARCH" ]; then
		ARCH="$TARGET_ARCH"
	else
		ARCH="`uname -m`"
	fi
fi

set_platform_opts

: ${PKG_CONFIG:=pkg-config}
: ${AR:="$CC_AR"}
: ${HOST_CC:=gcc}
: ${HOST_CXX:=g++}
: ${HOST_CLANG:=clang}
: ${LDLIBS:="$LIBS_N $LIBS_NL"}

echo "CC_ISA_EXT_FLAGS := $CC_ISA_EXT_FLAGS" >> $CONFIG
echo "ASM_ISA_EXT_FLAGS := $ASM_ISA_EXT_FLAGS" >> $CONFIG
echo "C_MARCH_FLAGS := $C_MARCH_FLAGS" >>$CONFIG
echo "ASM_MARCH_FLAGS := $ASM_MARCH_FLAGS" >>$CONFIG
echo "HOST_CC := gcc" >> $CONFIG
echo "HOST_CXX := g++" >> $CONFIG
echo "HOST_CLANG := clang" >> $CONFIG
echo "CC_ELF := $CC_ELF" >> $CONFIG
echo "LDLIBS = $LDLIBS" >> $CONFIG
echo "LDLIBS += \$(LDLIBS_LOCAL) -ldl" >> $CONFIG
echo "LDLIBS_STATIC = $LIBS_N" >> $CONFIG
echo "LDLIBS_STATIC += \$(LDLIBS_LOCAL) -ldl" >> $CONFIG
echo "TEST_TARGET_STATIC = \$(TEST_TARGET:%=%_static)" >> $CONFIG
echo "OBJ = \$(TEST_TARGET:%=%.o)" >> $CONFIG
echo "STATIC_OBJ = \$(TEST_TARGET_STATIC:%=%.o)" >> $CONFIG
echo "TARGETS = \$(TEST_TARGET)" >> $CONFIG
echo "PKG_CONFIG := $PKG_CONFIG" >> $CONFIG
echo "TARGET_ARCH := $TARGET_ARCH" >> $CONFIG


if [ -z "$INSTALLDIR" ]; then
	# Default to install directory in the xdp2 repo root (one level up from src/)
	INSTALLDIR=$PWD/../install/$ARCH
fi

if [ -z "$INSTALLTARNAME" ]; then
	INSTALLTARNAME=install.tgz
fi

echo "XDP2_ARCH := $ARCH" >> $CONFIG
echo "XDP2_CFLAGS += -DARCH_$ARCH" >> $CONFIG

if [ -n "$TARGET_ARCH" ]; then
	echo "CFLAGS += -DTARGET_ARCH_$TARGET_ARCH" >> $CONFIG
fi

if [ "$SOFT_FLOAT_BUILD" == "yes" ]; then
    echo "XDP2_CFLAGS += -DFPGA_SOFT_FLAG" >> $CONFIG
fi

echo >> $CONFIG

echo "Platform is $PLATFORM"

unlink ../platform 2>/dev/null
ln -s platforms/"$PLATFORM" ../platform

echo "Architecture is $ARCH"

unlink ./include/arch 2>/dev/null

if [ -d ../platform/src/include/arch/arch_$ARCH ]; then
	ln -s ../../platform/src/include/arch/arch_$ARCH ./include/arch
	echo "Architecture includes are ./include/arch_$ARCH"
	CFLAGS_N+=" -fcommon"
else
	ln -s ../../platform/src/include/arch/arch_generic ./include/arch
	echo "Architecture includes for $ARCH not found, using generic"
fi

echo "Target Architecture is $TARGET_ARCH"

echo "COMPILER is $COMPILER"

: ${CC:="$CC_GCC $CFLAGS_N"}
: ${CXX:="$CC_CXX $CFLAGS_N"}
: ${LD:="$LD_GCC"}

# Auto-detect llvm-config if not explicitly set or verify the one provided
debug_print 1 "Tool Detection: Starting llvm-config detection"
FOUND_LLVM_CONFIG=""
if [ -n "$HOST_LLVM_CONFIG" ]; then
	# If HOST_LLVM_CONFIG is set, verify it works
	debug_print 2 "Tool Detection: HOST_LLVM_CONFIG is set to: $HOST_LLVM_CONFIG"
	LLVM_CONFIG_PATH=$(command -v "$HOST_LLVM_CONFIG" 2>/dev/null)
	if [ -n "$LLVM_CONFIG_PATH" ] && "$LLVM_CONFIG_PATH" --version >/dev/null 2>&1; then
		FOUND_LLVM_CONFIG="$LLVM_CONFIG_PATH"
		debug_print 1 "Tool Detection: Verified user-provided HOST_LLVM_CONFIG: $FOUND_LLVM_CONFIG"
		LLVM_VER=`$FOUND_LLVM_CONFIG --version 2>/dev/null`
		debug_print 2 "Tool Detection: Version: $LLVM_VER"
	else
		debug_print 2 "Tool Detection: User-provided HOST_LLVM_CONFIG not found or invalid, will auto-detect"
	fi
fi

if [ -z "$FOUND_LLVM_CONFIG" ]; then
	# Auto-detect: try unversioned first, then versioned variants (llvm-config-20, etc.)
	# Verify each tool works by testing --version
	debug_print 2 "Tool Detection: Auto-detecting llvm-config..."
	for llvm_config_tool in llvm-config llvm-config-20 llvm-config-19 llvm-config-18 llvm-config-17 llvm-config-16; do
		debug_print 3 "Tool Detection: Checking for $llvm_config_tool"
		LLVM_CONFIG_PATH=$(command -v "$llvm_config_tool" 2>/dev/null)
		if [ -n "$LLVM_CONFIG_PATH" ]; then
			debug_print 3 "Tool Detection: Found $llvm_config_tool at $LLVM_CONFIG_PATH"
			if "$LLVM_CONFIG_PATH" --version >/dev/null 2>&1; then
				FOUND_LLVM_CONFIG="$LLVM_CONFIG_PATH"
				LLVM_VER=`$FOUND_LLVM_CONFIG --version 2>/dev/null`
				debug_print 1 "Tool Detection: Selected $llvm_config_tool (version $LLVM_VER)"
				break
			else
				debug_print 3 "Tool Detection: $llvm_config_tool found but --version test failed"
			fi
		fi
	done
fi
echo "LLVM_VER:$LLVM_VER"

if [ -n "$FOUND_LLVM_CONFIG" ]; then
	HOST_LLVM_CONFIG="$FOUND_LLVM_CONFIG"
	debug_print 1 "Tool Detection: Using HOST_LLVM_CONFIG=$HOST_LLVM_CONFIG"
else
	# Fallback to default (will fail later if it doesn't exist)
	HOST_LLVM_CONFIG="/usr/bin/llvm-config"
	debug_print 2 "Tool Detection: No llvm-config found, falling back to default: $HOST_LLVM_CONFIG"
fi
echo "HOST_LLVM_CONFIG:$HOST_LLVM_CONFIG"

: ${LLVM_CONFIG:="$LLVM_CONFIG"}
echo "CC := $CC" >> $CONFIG
echo "LD := $LD" >> $CONFIG
echo "CXX := $CXX" >> $CONFIG
echo "HOST_LLVM_CONFIG := $HOST_LLVM_CONFIG" >> $CONFIG
echo "LLVM_CONFIG := $LLVM_CONFIG" >> $CONFIG
echo "LDFLAGS := $LDFLAGS" >> $CONFIG
echo "PYTHON := python3" >> $CONFIG

# If we didn't get an architecture from the command line set it base
# on the running host

debug_print 1 "Configuration: Platform=$PLATFORM, Architecture=$ARCH, Compiler=$COMPILER"

check_libpcap
check_boostsystem
check_boostwave
check_boostthread
check_boostfilesystem
check_clang_lib
check_python
check_scapy

if [ -n "$CLANG_LIBS_DISCOVERED" ]; then
	echo "CLANG_LIBS := $CLANG_LIBS_DISCOVERED" >> $CONFIG
	debug_print 1 "Configuration: Wrote CLANG_LIBS to config.mk: $CLANG_LIBS_DISCOVERED"
else
	debug_print 2 "Configuration: Warning: CLANG_LIBS_DISCOVERED is empty, Makefile will use default"
fi

echo "ifneq (\$(USE_HOST_TOOLS),y)" >> $CONFIG

echo "%.o: %.c" >> $CONFIG
echo '	$(QUIET_CC)$(CC) $(CFLAGS) $(XDP2_CFLAGS) $(EXTRA_CFLAGS) $(C_MARCH_FLAGS)\
					-c -o $@ $<' >> $CONFIG

echo "%_static.o: %.c" >> $CONFIG
echo '	$(QUIET_CC)$(CC) $(CFLAGS) $(XDP2_CFLAGS) $(EXTRA_CFLAGS) -DXDP2_NO_DYNAMIC $(C_MARCH_FLAGS)\
					-c -o $@ $<' >> $CONFIG

echo "%.o: %.cpp" >> $CONFIG
echo '	$(QUIET_CXX)$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(C_MARCH_FLAGS)\
						-c -o $@ $<' >> $CONFIG

echo "%.o: %.s" >> $CONFIG
echo '	$(QUIET_ASM)$(CC) $(ASM_MARCH_FLAGS)\
					-c -o $@ $<' >> $CONFIG

echo "else" >> $CONFIG

echo "%.o: %.c" >> $CONFIG
echo '	$(QUIET_CC)$(HOST_CC) $(CFLAGS) $(XDP2_CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $<' >> $CONFIG

echo "%.o: %.cpp" >> $CONFIG
echo '	$(QUIET_CXX)$(HOST_CXX) $(XDP2_CXXFLAGS) $(CXXFLAGS) $(EXTRA_CXXFLAGS)		\
						-c -o $@ $<' >> $CONFIG

echo "endif" >> $CONFIG

echo "%.ll: %.c" >> $CONFIG
echo '	$(QUIET_CC)$(HOST_CLANG) $(CFLAGS) $(XDP2_CFLAGS) $(EXTRA_CFLAGS) $(C_MARCH_FLAGS)\
					-S $< -emit-llvm' >> $CONFIG

echo >> $CONFIG
if [ -f ${HOST_LLVM_CONFIG} ]; then
XDP2_CLANG_VERSION=`${HOST_LLVM_CONFIG} --version`
else
XDP2_CLANG_VERSION=`/usr/bin/llvm-config --version`
fi

echo "XDP2_CLANG_VERSION=${XDP2_CLANG_VERSION}"
echo "XDP2_CLANG_VERSION=${XDP2_CLANG_VERSION}" >> $CONFIG
XDP2_CLANG_RESOURCE_PATH="`${HOST_LLVM_CONFIG} --libdir`/clang/`echo ${XDP2_CLANG_VERSION} | cut -d'.' -f1`"
XDP2_C_INCLUDE_PATH="`${HOST_LLVM_CONFIG} --libdir`/clang/`echo ${XDP2_CLANG_VERSION} | cut -d'.' -f1`/include"
echo "XDP2_C_INCLUDE_PATH=${XDP2_C_INCLUDE_PATH}"
echo "XDP2_C_INCLUDE_PATH=${XDP2_C_INCLUDE_PATH}" >> $CONFIG
echo "XDP2_CLANG_RESOURCE_PATH=${XDP2_CLANG_RESOURCE_PATH}"
echo "XDP2_CLANG_RESOURCE_PATH=${XDP2_CLANG_RESOURCE_PATH}" >> $CONFIG
echo >> $CONFIG

output_platform_config

# TBD
#OPTIMIZE_PARSER- may have to be enhanced with commandline options
#

echo >> $CONFIG
echo "endif # !TOP_LEVEL_MAKE" >> $CONFIG
echo >> $CONFIG
echo "INSTALLDIR ?= $INSTALLDIR" >> $CONFIG
echo "INSTALLTARNAME ?= $INSTALLTARNAME" >> $CONFIG
echo "BUILD_OPT_PARSER ?= $BUILD_OPT_PARSER" >> $CONFIG
echo "BUILD_PARSER_JSON ?= $BUILD_PARSER_JSON" >> $CONFIG
echo "NO_BUILD_COMPILER ?= $NO_BUILD_COMPILER" >> $CONFIG

echo "CONFIG_DEFINES := $CONFIG_DEFINES" >> $CONFIG
