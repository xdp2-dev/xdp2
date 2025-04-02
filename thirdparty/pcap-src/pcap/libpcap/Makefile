
include ../config.mk

# Turn on transparent support for LFS
CFLAGS += -fPIC
NO_SPIKE_BUILD ?= n
#LOC_UTILOBJ = mdm.o thread_queues.o threads.o datapath.o disc.o threads_emul.o

#
#Prepare source and object lists for XDP2_RISCV64_SPIKE with riscv64-unknown-elf-gcc
#
OBJDIR_PCAP := pcap_objs
OBJDIR_PCAP_ELF := pcap_elf-objs

ALLOBJS_PCAP += pcap.o pcap-common.o
ALLOBJS_PCAP += savefile.o bpf_filter.o fmtutils.o
ALLOBJS_PCAP += gencode.o grammar.o nametoaddr.o
ALLOBJS_PCAP += optimize.o scanner.o sf-pcap.o
ALLOBJS_PCAP += sf-pcapng.o getaddrinfo.o etherent.o inet_ntop.o
TARGETS = libpcap_xdp2.a 

.PHONY: all
all: $(TARGETS)

libpcap_xdp2.a: $(ALLOBJS_PCAP)
	$(QUIET_AR)$(AR) rcs $@ $^

$(ALLOBJS_PCAP_UNKNOWN_LINUX): $(OBJDIR_PCAP)/%.o: %.c
	mkdir -p $(OBJDIR_PCAP)
	$(QUIET_CC)$(CC) $(CFLAGS) -I $(CURRDIR)/include-linux/uapi\
			-I $(CURRDIR)/include -D _GNU_SOURCE\
			-c -o $@ $<
.PHONY: install
install: 
	mkdir -p ../../../libpcap_xdp2/generic
	cp libpcap_xdp2.a ../../../libpcap_xdp2/generic
.PHONY: clean
clean:
	@rm -f  $(ADDLIB) $(TARGETS) 
	@rm -rf  $(OBJDIR_PCAP)
