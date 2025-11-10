SRCDIR = ../..

include $(SRCDIR)/config.mk

TOPTARGETS := all clean install

SUBDIRS = proto_defs parsers

$(TOPTARGETS) : $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)

INCDIR=$(INSTALLDIR)$(HDRDIR)/xdp2

TARGETS = compiler_helpers.h utility.h bswap.h bitmap_word.h
TARGETS += bitmap.h bitmap_helpers.h pmacro.h _pmacro_gen.h
TARGETS += vstruct.h switch.h locks.h timer.h cli.h pcap.h
TARGETS += packets_helpers.h table_common.h stable.h _stable.h
TARGETS += dtable.h _dtable.h checksum.h obj_allocator.h pvbuf.h
TARGETS += pvpkt.h config.h parser_types.h parser.h parser_metadata.h
TARGETS += flag_fields.h tlvs.h arrays.h proto_defs_define.h
TARGETS += proto_defs.h accelerator.h pkt_action.h bpf.h xdp_tmpl.h

PMACRO_GEN = $(SRCDIR)/tools/pmacro/pmacro_gen

PMACRO_TARG = _pmacro_gen.h

PMACRO_MAX_ARGS ?= 256
# Generate pmacro_gen.h from pmacro_gen tool
$(PMACRO_TARG):
	if  [ "$(VERBOSE)" = "0" ]; then \
		echo "    MACROGEN $@"; \
	fi
	$(PMACRO_GEN) $(PMACRO_MAX_ARGS) > $@

TABLES_TARGS = _stable.h _dtable.h

_%table.h: _%table_template.h _%table_skey_template.h
	if  [ "$(VERBOSE)" = "0" ]; then \
		echo "    TBL_INC  $@"; \
	fi
	sed -e 's/$$TYPE\$$/PLAIN/g;s/$$CLASS\$$//g;s/$$LTYPE\$$/plain/g' $< > $@
	sed -e 's/$$TYPE\$$/PLAIN/g;s/$$CLASS\$$/_NAME/g;s/$$LTYPE\$$/plain/g' $< >> $@
	sed -e 's/$$TYPE\$$/PLAIN/g;s/$$CLASS\$$/_CAST/g;s/$$LTYPE\$$/plain/g' $< >> $@
	sed -e 's/$$TYPE\$$/PLAIN/g;s/$$CLASS\$$/_NAME_CAST/g;s/$$LTYPE\$$/plain/g' $< >> $@
	sed -e 's/$$TYPE\$$/PLAIN/g;s/$$CLASS\$$/_ANAME/g;s/$$LTYPE\$$/plain/g' $< >> $@
	sed -e 's/$$TYPE\$$/PLAIN/g;s/$$CLASS\$$/_ANAME_CAST/g;s/$$LTYPE\$$/plain/g' $< >> $@
	sed -e 's/$$TYPE\$$/TERN/g;s/$$CLASS\$$//g;s/$$LTYPE\$$/tern/g' $< >> $@
	sed -e 's/$$TYPE\$$/TERN/g;s/$$CLASS\$$/_NAME/g;s/$$LTYPE\$$/tern/g' $< >> $@
	sed -e 's/$$TYPE\$$/TERN/g;s/$$CLASS\$$/_CAST/g;s/$$LTYPE\$$/tern/g' $< >> $@
	sed -e 's/$$TYPE\$$/TERN/g;s/$$CLASS\$$/_NAME_CAST/g;s/$$LTYPE\$$/tern/g' $< >> $@
	sed -e 's/$$TYPE\$$/TERN/g;s/$$CLASS\$$/_ANAME/g;s/$$LTYPE\$$/tern/g' $< >> $@
	sed -e 's/$$TYPE\$$/TERN/g;s/$$CLASS\$$/_ANAME_CAST/g;s/$$LTYPE\$$/tern/g' $< >> $@
	sed -e 's/$$TYPE\$$/LPM/g;s/$$CLASS\$$//g;s/$$LTYPE\$$/lpm/g' $< >> $@
	sed -e 's/$$TYPE\$$/LPM/g;s/$$CLASS\$$/_NAME/g;s/$$LTYPE\$$/lpm/g' $< >> $@
	sed -e 's/$$TYPE\$$/LPM/g;s/$$CLASS\$$/_CAST/g;s/$$LTYPE\$$/lpm/g' $< >> $@
	sed -e 's/$$TYPE\$$/LPM/g;s/$$CLASS\$$/_NAME_CAST/g;s/$$LTYPE\$$/lpm/g' $< >> $@
	sed -e 's/$$TYPE\$$/LPM/g;s/$$CLASS\$$/_ANAME/g;s/$$LTYPE\$$/lpm/g' $< >> $@
	sed -e 's/$$TYPE\$$/LPM/g;s/$$CLASS\$$/_ANAME_CAST/g;s/$$LTYPE\$$/lpm/g' $< >> $@
	sed -e 's/$$TYPE\$$/PLAIN/g;s/$$LTYPE\$$/plain/g' $(word 2, $^) >> $@
	sed -e 's/$$TYPE\$$/TERN/g;s/$$LTYPE\$$/tern/g' $(word 2, $^) >> $@
	sed -e 's/$$TYPE\$$/LPM/g;s/$$LTYPE\$$/lpm/g' $(word 2, $^) >> $@

all: $(PMACRO_TARG) $(TABLES_TARGS)

clean:
	@rm -f $(PMACRO_TARG) $(TABLES_TARGS)

install: $(TARGETS)
	@install -m 0755 -d $(INCDIR)
	$(QUIET_INSTALL)$(INSTALL) -m 0644 $(TARGETS) $(INCDIR)
