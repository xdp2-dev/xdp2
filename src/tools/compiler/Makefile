# Set USE_HOST_TOOLS to use HOST_CC for building compiler
USE_HOST_TOOLS := y
include ../../config.mk

TARGETS = gen-patterns-cpp xdp2-compiler

TEMPLATES_PATH = ../../templates/xdp2
TEMPLATES_LIST = $(shell sed -f $(TEMPLATES_PATH)/templates.sed $(TEMPLATES_PATH)/templates.list)
TEMPLATES_SRC = $(patsubst %,%.template.c,$(TEMPLATES_LIST))

%.cpp: %.template.c
	@echo "const char* $(<F:.template.c=)_template_str = R\"(" > $@
	$(QUIET_EMBED)$(CAT) $< >> $@
	@echo ")\";" >> $@

OBJS := src/main.o src/template.o
OBJS += $(patsubst %,$(TEMPLATES_PATH)/%.o,$(TEMPLATES_LIST))

CLANG_INFO ?= -DXDP2_CLANG_VERSION="$(XDP2_CLANG_VERSION)" -DXDP2_CLANG_RESOURCE_PATH="$(XDP2_CLANG_RESOURCE_PATH)"

LLVM_INCLUDE ?= -I`$(HOST_LLVM_CONFIG) --includedir`
LLVM_LIBS ?= -L`$(HOST_LLVM_CONFIG) --libdir`

CPPFRONT_INCLUDE = -I../../../thirdparty/cppfront/include

EXTRA_CXXFLAGS += '-g'

CXXFLAGS += -Iinclude -I../../../thirdparty/json/include -I../../include $(LLVM_INCLUDE) -std=c++20 $(CFLAGS_PYTHON)  $(CLANG_INFO) $(EXTRA_CXXFLAGS) -Wno-deprecated-enum-enum-conversion $(CPPFRONT_INCLUDE)
BOOST_LIBS ?= -lboost_wave -lboost_thread -lboost_filesystem -lboost_system -lboost_program_options
CLANG_LIBS ?= -lclang -lLLVM -lclang-cpp


LIBS ?= -lpthread -ldl -lutil

all: $(TARGETS)
CPPFRONT = ../../../thirdparty/cppfront/cppfront-compiler
PATTERNS_LLVM_CPP2 = include/xdp2gen/llvm/patterns.h2
PATTERNS_CLANG_AST_CPP2 = include/xdp2gen/ast-consumer/patterns.h2

xdp2-compiler: $(OBJS)
	$(HOST_CXX) $^ -o $@ $(LLVM_LIBS) $(BOOST_LIBS) $(CLANG_LIBS) $(LDFLAGS_PYTHON) $(LIBS)

xdp2-define-test: $(TESTOBJS) $(TESTFILES)
	$(HOST_CXX) $(TESTOBJS) -o $@ $(BOOST_LIBS) $(LDFLAGS_PYTHON)
	for i in $(TESTFILES); \
	do ./$@ $$i; done

gen-patterns-cpp:
	$(CPPFRONT) $(PATTERNS_LLVM_CPP2) && $(CPPFRONT) $(PATTERNS_CLANG_AST_CPP2)

all: $(TARGETS)

.PHONY: install
install: xdp2-compiler
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(OBJS) $(TARGETS) test/main.o

.PHONY: format
format:
	@find -iname *.cpp -o -iname *.h | xargs clang-format -i --verbose
