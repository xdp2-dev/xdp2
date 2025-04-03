include ../../config.mk

TARGET = pmacro_gen

SRC = pmacro_gen.c

$(TARGET): pmacro_gen.c
	$(QUIET_HOST_CC)$(HOST_CC) $(SRC) -o $@

.PHONY: install
install: $(TARGET)
	$(QUIET_INSTALL)$(INSTALL) -m 0755 $^ $(INSTALLDIR)$(BINDIR)

.PHONY: clean
clean:
	@rm -f $(TARGET)
