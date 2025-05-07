XDP2DIR ?= $(realpath ..)
INSTALLDIR ?= /usr/lib/modules/$(shell uname -r)/extra
TARGETS := cls_xdp2.ko

.PHONY: all
all: $(TARGETS)

cls_xdp2.ko:
	$(MAKE) -C $(KDIR) M=$(shell pwd) XDP2DIR=$(XDP2DIR)

.PHONY: clean
clean:
	$(MAKE) -C $(KDIR) M=$(shell pwd) $@

.PHONY: modules_install
modules_install: all
	@install -m 0755 -d $(INSTALLDIR)
	install -m 0644 $(TARGETS) $(INSTALLDIR)
	depmod
