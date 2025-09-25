# Force no static build

SRCDIR = ../..

include $(SRCDIR)/config.mk

TARGET = parse_dump

NONOPT_PARSER_OBJ = parser.o

OPT_PARSER_OBJ = parser.p.o
OPT_PARSER_SRC = parser.p.c

PARSER_JSON_OBJ = parser.ll parser.json

OBJ = main.o print_meta.o tables.o
LDLIBS = $(SRCDIR)/lib/xdp2/libxdp2.a $(SRCDIR)/lib/cli/libcli.a
LDLIBS += -L$(SRCDIR)/../thirdparty/libpcap
LDLIBS += $(SRCDIR)/lib/siphash/libsiphash.a
LDLIBS +=-lpcap
# -lbsd

$(NONOPT_TARGET): $(OBJ) $(NONOPT_PARSER_OBJ)
	$(QUIET_LINK)$(CC) $^  $(LDLIBS) -o $@

ifeq ($(BUILD_PARSER_JSON),y)
PARSER_JSON = parser.json
endif

.PHONE: all
all: $(TARGET) $(PARSER_JSON)

ifeq ($(BUILD_OPT_PARSER),y)
$(TARGET): $(OBJ) $(OPT_PARSER_OBJ)
	$(QUIET_LINK)$(CC) $^  $(LDLIBS) -o $@
else
$(TARGET): $(OBJ) $(NONOPT_PARSER_OBJ)
	$(QUIET_LINK)$(CC) $^  $(LDLIBS) -o $@
endif

XDP2_COMPILER = $(SRCDIR)/tools/compiler/xdp2-compiler

$(OPT_PARSER_SRC): parser.c parser.o parser.json
	$(XDP2_COMPILER) -I$(SRCDIR)/include -o $@ -i $<

parser.json: parser.ll
	$(XDP2_COMPILER) -I$(SRCDIR)/include -o $@ -l $< -i ${<:%.ll=%.c}

.PHONY: install
install: $(TARGET)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGET) $(OBJ) $(NONOPT_PARSER_OBJ) $(OPT_PARSER_OBJ) $(OPT_PARSER_SRC) $(PARSER_JSON_OBJ)
