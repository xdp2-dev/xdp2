# Force no static build

SRCDIR = ../..

include $(SRCDIR)/config.mk

TARGET = parse_dump

NONOPT_PARSER_OBJ = parser.o

OPT_PARSER_OBJ = parser.p.o
OPT_PARSER_SRC = parser.p.c

OBJ = main.o print_meta.o tables.o
LDLIBS = $(SRCDIR)/lib/xdp2/libxdp2.a $(SRCDIR)/lib/cli/libcli.a
LDLIBS += -L$(SRCDIR)/../thirdparty/libpcap
LDLIBS += $(SRCDIR)/lib/siphash/libsiphash.a
LDLIBS +=-lpcap
# -lbsd

$(NONOPT_TARGET): $(OBJ) $(NONOPT_PARSER_OBJ)
	$(QUIET_LINK)$(CC) $^  $(LDLIBS) -o $@

ifeq ($(BUILD_OPT_PARSER),y)
$(TARGET): $(OBJ) $(OPT_PARSER_OBJ)
	$(QUIET_LINK)$(CC) $^  $(LDLIBS) -o $@
else
$(TARGET): $(OBJ) $(NONOPT_PARSER_OBJ)
	$(QUIET_LINK)$(CC) $^  $(LDLIBS) -o $@
endif

XDP2_COMPILER = $(SRCDIR)/tools/compiler/xdp2-compiler

$(OPT_PARSER_SRC): parser.c parser.o
	$(XDP2_COMPILER) -I$(SRCDIR)/include -o $@ -i $<

.PHONY: install
install: $(TARGET)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGET) $(OBJ) $(NONOPT_PARSER_OBJ) $(OPT_PARSER_OBJ) $(OPT_PARSER_SRC)
