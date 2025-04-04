<img src="images/Hop.png" alt="Hop the Panda Logo" align="right"/>

The XDP Target
================

The XDP2 compiler is able to generate code that runs on XDP.
The generated code uses the XDP2 parser framework as its building blocks,
while still complying with the eBPF verifier.

Example XDP programs in XDP2 are located in [samples/xdp2](../samples/xdp).
There are four sample programs:

| Name | Directory | README |
|---|---|---|
| flow_tracker_simple | [samples/xdp/flow_tracker_simple](../samples/xdp/flow_tracker_simple) | [README.md](../samples/xdp/flow_tracker_simple/README.md)
| flow_tracker_tmpl | [samples/xdp/flow_tracker_tmpl](../samples/xdp/flow_tracker_tmpl) | [README.md](../samples/xdp/flow_tracker_tmpl/README.md)
| flow_tracker_tlvs | [samples/xdp/flow_tracker_tlvs](../samples/xdp/flow_tracker_tlvs) | [README.md](../samples/xdp/flow_tracker_tlvs/README.md)
| flow_tracker_combo | [samples/xdp/flow_tracker_combo](../samples/xdp/flow_tracker_combo) | [README.md](../samples/xdp/flow_tracker_combo/README.md)

In this description  we'll use
[samples/xdp/flow_tracker_simple](../samples/xdp/flow_tracker_simple)
as a reference.

Architecture
------------

The general architecture of an XDP in XDP2 program is to split the program
into two parts: the frontend XDP handler program, and a program to do continued
parsing in a tail-call.

<img src="images/xdp.png" alt="XDP parser architecture"/>

*xdp_prog* is the frontend XDP program called each packet. *parser_prog*
is called as a tail-call by *xdp_prog* or itself.

The code in this program parsers up to 8 nodes, handing over the
control to 'parser program' in case it didn't finish parsing or if it
encountered a node with TLVs.

Each dotted line represents a BPF tail-call, which is how we hand control
to 'parser program'.

A node that by the specification has TLVs (eg. TCP), but doesn't declare
them in the parser graph node, will not trigger this behaviour.
Nodes that come after a node with TLVs are always processed in 'parser program'.

In total, the current implementation, can parse up to 40 nodes.
Due to the restrictions imposed by the eBPF verifier, it might not be always
possible to parse up to 8 nodes in 'xdp entry'.

Building the application
========================

In this section we build a simple flow tracker and explain how to integrate
XDP2 in your XDP application.

The flowtracker application described here can be built using the sources
[samples/xdp/flow_tracker_simple](../samples/xdp/flow_tracker_simple).


XDP2 Parser
-----------

Lets define a 5-tuple parser (from
[samples/xdp/flow_tracker_simple/parser.c](../samples/xdp/flow_tracker_simple/parser.c)).
The parser will extract IPv4 source and destination, protocol and TCP ports.

```C
/* Parser for Ethernet, IPv4 and IPv6, and TCP and UDP. Do canned metadata
 * extraction for each node
 */

#include "common.h"

/* Meta data functions for parser nodes. Use the canned templates
 * for common metadata
 */

XDP2_METADATA_TEMP_ether(ether_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv4(ipv4_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ipv6(ipv6_metadata, xdp2_metadata_all)
XDP2_METADATA_TEMP_ports(ports_metadata, xdp2_metadata_all)

/* Parse nodes. Parse nodes are composed of the common XDP2 Parser protocol
 * nodes, metadata functions defined above, and protocol tables defined
 * below
 */

XDP2_MAKE_PARSE_NODE(ether_node, xdp2_parse_ether, ether_table,
                     (.ops.extract_metadata = ether_metadata));
XDP2_MAKE_PARSE_NODE(ip_check_node, xdp2_parse_ip, ip_check_table, ());
XDP2_MAKE_PARSE_NODE(ipv4_node, xdp2_parse_ipv4, ipv4_table,
                     (.ops.extract_metadata = ipv4_metadata));
XDP2_MAKE_LEAF_PARSE_NODE(ports_node, xdp2_parse_ports,
                          (.ops.extract_metadata = ports_metadata));

/* Protocol tables */

XDP2_MAKE_PROTO_TABLE(ether_table,
                      ( __cpu_to_be16(ETH_P_IP), ip_check_node )
);

XDP2_MAKE_PROTO_TABLE(ip_check_table,
                      ( 4, ipv4_node )
);

XDP2_MAKE_PROTO_TABLE(ipv4_table,
                      ( IPPROTO_TCP, ports_node ),
                      ( IPPROTO_UDP, ports_node )
);

XDP2_PARSER(xdp2_parser_simple_tuple, "XDP2 parser for 5 tuple TCP/UDP",
            ether_node,
            (.max_frames = 1,
             .metameta_size = 0,
             .frame_size = sizeof(struct xdp2_metadata_all)
            )
);
```

This creates a parser named *xdp2_parser_simple_tuple*. The root is parse
node is *ether_node*. Additionally, there are parse nodes *ip_check_node*,
*ipv4_node*, and *ports_node*. The *ether_table* has one entry for IPv4, and
the *ipv4_table* has entries for UDP and TCP both of which map to *ports_node*.

We use the "canned" metadata functions and metadata structure define in
[include/xdp2/parser_metadata.h](../src/include/xdp2/parser_metadata.h).
The *ether_node*, *ipv4_node*, and *ports_node* parse nodes have an
associated metadata function.

Generating the Parser
---------------------

The compiler will generate eBPF-C code when targeting XDP (replace
*xdp2/install* with the path for the appropriate directory). Note that
normally 'make' would be used (see the
[README](../samples/xdp/flow_tracker_simple/README.md)).

```
$ ~/xdp2/install/bin/xdp2-compiler -I~/xdp2/install/include -i parser.c -o parser.xdp.h
```


The file `parser.xdp.h` contains the generated eBPF-C code.  The entry point
function is `xdp2_parser_simple_tuple_xdp2_parse_ether_node` in our case.

Flow tracker
------------

Let's create a toy flow tracker which store flows in a hash map.

From
[samples/xdp/flow_tracker_simple/flow_tracker.h](../samples/xdp/flow_tracker_simple/flow_tracker.h)
we define include definitions:
```C
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

#include "xdp2/bpf.h"
#include "xdp2/parser_metadata.h"

struct flowtuple {
        __be32 saddr;
        __be32 daddr;
        __be16 sport;
        __be16 dport;
        __u8 protocol;
};

struct bpf_elf_map SEC("maps") flowtracker = {
        .type = BPF_MAP_TYPE_HASH,
        .size_key = sizeof(struct flowtuple),
        .size_value = sizeof(__u64),
        .max_elem = 32,
        .pinning = PIN_GLOBAL_NS,
};

/* Lookup a flow in the flow tracker map and increment counter on a hit */
static __always_inline void flow_track(struct xdp2_metadata_all *frame)
{
        struct flowtuple ft = {};
        __u64 new_counter = 1;
        __u64 *counter;

        /* is packet TCP? */
        if (frame->ip_proto != 6)
                return;

        ft.saddr = frame->addrs.v4.saddr;
        ft.daddr = frame->addrs.v4.daddr;
        ft.sport = frame->src_port;
        ft.dport = frame->dst_port;
        ft.protocol = frame->ip_proto;

        /* Looking for packets from source port 22 */
        if (ft.sport != htons(22))
                return;

        counter = bpf_map_lookup_elem(&flowtracker, &ft);
        if (counter) {
                __sync_fetch_and_add(counter, 1);
        } else {
                /* New flow entry */
                bpf_map_update_elem(&flowtracker, &ft, &new_counter,
                                    BPF_ANY);
        }
}
```
*struct flow_tracker* is a user defined metadata structure.  The *flowtracker*
hash map is defined to hold thirty-two flow entries. *flow_track* is the
function called to record the flow in the hash map.

The main XDP programs are defined in 
[samples/xdp/flow_tracker_simple/flow_tracker.xdp.c](../samples/xdp/flow_tracker_simple/flow_tracker.xdp.c):

```C
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

#include "common.h"
#include "flow_tracker.h"

#include "parser.xdp.h"

#define PROG_MAP_ID 0xcafe

struct bpf_elf_map SEC("maps") ctx_map = {
        .type = BPF_MAP_TYPE_PERCPU_ARRAY,
        .size_key = sizeof(__u32),
        .size_value = sizeof(struct flow_tracker_ctx),
        .max_elem = 2,
        .pinning = PIN_GLOBAL_NS,
};

struct bpf_elf_map SEC("maps") parsers = {
        .type = BPF_MAP_TYPE_PROG_ARRAY,
        .size_key = sizeof(__u32),
        .size_value = sizeof(__u32),
        .max_elem = 1,
        .pinning = PIN_GLOBAL_NS,
        .id = PROG_MAP_ID,
};

static __always_inline struct flow_tracker_ctx *xdp2_get_ctx(void)
{
        /* clang-10 has a bug if key == 0,
         * it generates bogus bytecodes.
         */
        __u32 key = 1;

        return bpf_map_lookup_elem(&ctx_map, &key);
}

* Entry point for the XDP program */
SEC("prog")
int xdp_prog(struct xdp_md *ctx)
{
        struct flow_tracker_ctx *parser_ctx = xdp2_get_ctx();
        void *data_end = (void *)(long)ctx->data_end;
        const void *data = (void *)(long)ctx->data;
        const void *original = data;
        int rc = XDP2_OKAY;

        if (!parser_ctx)
                return XDP_ABORTED;

        parser_ctx->ctx.frame_num = 0;
        parser_ctx->ctx.next = CODE_IGNORE;
        parser_ctx->ctx.metadata = parser_ctx->frame;
        parser_ctx->ctx.parser = xdp2_parser_simple_tuple;

        /* Invoke XDP2 parser */
        rc = XDP2_PARSE_XDP(xdp2_parser_simple_tuple, &parser_ctx->ctx,
                            &data, data_end, false, 0);

        if (rc != XDP2_OKAY && rc != XDP2_STOP_OKAY)
                return XDP_PASS;

        if (parser_ctx->ctx.next != CODE_IGNORE) {
                /* Parser is not complete, need to continue in a tailcall */
                parser_ctx->ctx.offset = data - original;
                bpf_xdp_adjust_head(ctx, parser_ctx->ctx.offset);
                bpf_tail_call(ctx, &parsers, 0);
        }

        /* Call processing user function here */
        flow_track(parser_ctx->frame);

        return XDP_PASS;
}

* Tail call program. Continue parsing in a tail call */
SEC("0xcafe/0")
int parser_prog(struct xdp_md *ctx)
{
        struct flow_tracker_ctx *parser_ctx = xdp2_get_ctx();
        void *data_end = (void *)(long)ctx->data_end;
        const void *data = (void *)(long)ctx->data;
        const void *original = data;
        int rc = XDP2_OKAY;

        if (!parser_ctx)
                return XDP_ABORTED;

        /* XXXTH we need to set ctx.metadata here to satisfy the verifier.
         * Not sure why we need to do this. Needs to be debugged
         */
        parser_ctx->ctx.metadata = parser_ctx->frame;

        /* Invoke XDP2 parser */
        rc = XDP2_PARSE_XDP(xdp2_parser_simple_tuple, &parser_ctx->ctx,
                            &data, data_end, true, 0);

        if (rc != XDP2_OKAY && rc != XDP2_STOP_OKAY) {
                bpf_xdp_adjust_head(ctx, -parser_ctx->ctx.offset);
                return XDP_PASS;
        }
        if (parser_ctx->ctx.next != CODE_IGNORE) {
                /* Parser is not complete, need to continue in another
                 * tailcall
                 */
                parser_ctx->ctx.offset += data - original;
                bpf_xdp_adjust_head(ctx, data - original);
                bpf_tail_call(ctx, &parsers, 0);
        }

        /* Call processing user function here */
        flow_track(parser_ctx->frame);

        bpf_xdp_adjust_head(ctx, -parser_ctx->ctx.offset);

        return XDP_PASS;
}

char __license[] SEC("license") = "GPL";
```

We can see the two eBPF programs *xdp_prog* and *parser_prog*. They are very
similar, the major difference being that *xdp_prog* needs initialization.

The first thing each program does is to get a flow_tracker context from an
eBPF map. The flow_tracker context holds the metadata buffer and other
parameters for parsing. In *xdp_prog* we initialize the context structure.

The **XDP_PARSE_XDP** macro is invoked to parse the packet. Arguments are
the address of the parser structure, a pointer to the parser context in the
flow_tracker context, a pointer-pointer to the data (the pointer will
advance by the length of headers processed), an end-of-data pointer, and
boolean indicating whether the macro is being invoked in a tail-call.

If **XDP_PARSE_XDP** returns success then *parser_ctx->ctx.next* is checked.
If the value is **CODE_IGNORE** then that indicates that parsing is complete.
If it's another value then that is a marker for the next protocol to process;
a tail-call is made to *parser_prog* to continue parsing starting from the
node indicated in *parser_ctx->ctx.next*. In the tail-call program,
**XDP_PARSE_XDP** is invoked and the tail-call argument is set to true.

Note that a more robust implementation should handle the case where our map is
full.

Running the XDP program
=======================

The output of compilation is **flow_tracker.xdp.o**. This can be loaded
as an XDP program. For example:

```
$ sudo ip link set dev <device> xdp obj flow_tracker.xdp.o
```

Where `<device>` is your network device (example `eno1` etc).

Note that `include/` points to XDP2's `include/` directory.

There are several ways to load a BPF program into XDP.
Using the `ip` program is just one of those ways.

After loading the program, we can then verify the implementation with `bpftool`.

```
$ sudo bpftool map -f
2: prog_array  name hid_jmp_table  flags 0x0
        key 4B  value 4B  max_entries 1024  memlock 8576B
        owner_prog_type tracing  owner jited
3: hash  flags 0x0
        key 9B  value 1B  max_entries 500  memlock 46944B
        pinned /sys/fs/bpf/snap/snap_snap-store_ubuntu-software
6: percpu_array  name ctx_map  flags 0x0
        key 4B  value 232B  max_entries 2  memlock 7824B
        pinned /sys/fs/bpf/tc/globals/ctx_map
7: prog_array  name parsers  flags 0x0
        key 4B  value 4B  max_entries 1  memlock 392B
        owner_prog_type xdp  owner jited
        pinned /sys/fs/bpf/tc/globals/parsers
8: hash  name flowtracker  flags 0x0
        key 16B  value 8B  max_entries 32  memlock 5568B
        pinned /sys/fs/bpf/tc/globals/flowtracker
14: array  name flow_tra.data  flags 0x400
        key 4B  value 8B  max_entries 1  memlock 8192B
        btf_id 217
15: array  name flow_tra.rodata  flags 0x480
        key 4B  value 752B  max_entries 1  memlock 8192B
        btf_id 217  frozen
```

And the we can dump the map after generating some traffic (packets with
TCP source port 22):
```
$ sudo bpftool map dump id 7
key: 7f 00 00 01 7f 00 00 01  00 16 9f c2 06 00 00 00  value: 08 00 00 00 00 00 00 00
Found 1 element
```

The key is the memory dump of our `struct flowtuple` tuple and the
value is the memory dump of our `__u64` counter.

Unloading
---------

The flow tracket program can be unloaded by:

```
sudo ip link set dev <device> xdp off
```

The BPF maps can be removed by:

```
sudo rm -rfv /sys/fs/bpf/tc/globals
```
