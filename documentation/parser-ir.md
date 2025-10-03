<img src="images/xdp2.png" alt="XDP2 logo"/>

The Parser Immeidate Representation (PIR)
=========================================

The *Parser Intermediate Representation*, or *PIR*, is a generic Intermediate
Representation (IR) for parsers. This IR represents parsers in a declarative
representation as opposed to an imperative representation. PIR maps the parser
data structures described in [parser description](parser.md).

Description
-----------

The Parser INtermeidtae Representation can be out into plain json. That makes
since it is a declarative representation.

We illustrate PIR by example: the json below describes a parser for Ethernet,
IPv4, TCP with options, and GRE with flag-fields.

```json
"parsers": [{
  "name": "my_parser",
  "root-node": "eth_node",
  "okay-target": "okay”
}],
"parse-nodes": [
{
  "name": "ipv4_node",
  "min-hdr-length": 20,
  "hdr-length": {
    "field-off": 0, "field-len": 1,
    "mask": "0xf", "multiplier" : 4
  },
  "next-proto": {
     "field-off": 9, "field-len": 1,
     "ents": [
         { "key": 6, "node": "tcp_node"},
         { "key": 47, "node": "gre_node"},
     ],
  },
},{
  "name": “tcp_node",
  "tlvs-parse-node": {
    "tlv-type":
       { "field-off": 0, "field-len": 1 },
    "tlv-length":
      { "field-off": 1, "field-len": 1 },
    "pad1": 1, "eol": 0,
    "table": "tcp_opt_table"
  },
  # Other fields in TCP parse node
},{
  "name": "gre_node",
  "encap": true,
  "hdr-length":
       { "flag-fields-length": true },
  "flag-fields-parse-node": {
     "flags-offset": 0, "flags-length": 2,
     "flags-reverse-order": true,
     "table": "gre_flags_table"
  },
  # Other fields in GRE parse node
},{
  "name": "tcp_opt_tstamp_node",
  "metadata": { "ents": [
      { "md-off": 4,"hdr-src-off": 2,
        "length": 4,"endian-swap": true
      }
  ]}
},{
  "name": "gre_key_id",
  "metadata": { "ents": [
      { "md-off": 8, "type": "hdr-off-len" }
  ]}
},
# ether_node and okay parse nodes
],
“proto-tables”: [
    #  tcp_opt_table and  gre_flags_table
]
```

The **parsers** property declares *my_parser* with **root-node** *eth_node*
(not shown) and **okay-target** indicates that the *okay* node (not shown) is
invoked when the parser completes. Parse nodes are defined in the
**parse-nodes** property. The protocol table for *eth_node* includes an entry
that maps **0x800** EtherType to *ipv4_node*.

In *ipv4_node*, **min-hdr-len** sets the minimum header length (20 bytes for
IPv4). **hdr-length** sets parameters for the IPv4 header length function.
**next-proto** gives the parameters for the next header type function, and the
ents sub-field of **next-proto** is an inlined protocol table that matches TCP
and GRE to *ipv4_node* and *gre_node*.

The **tlvs-parse-node** in *ipv4_node* provides the rules and attributes for
parsing TCP options. **tlv-type** and **tlv-length** provide the function
parameters to determine the type and length of a TCP option (the minimum TLV
length is inferred ). The starting offset of TLVs is taken to be the minimum
header length (20 bytes). *tcp_opt_table* maps option type **6** to
*tcp_opt_tstamp_node*, and that node records the timestamp value in the
metadata.

**flag-fields-parse-node** in  *gre_node* gives the rules for parsing GRE
flag-fields. For each possible GRE flag-filed there is an entry in
*gre_flags_table* that specifies the flag value, mask, and field size.
Flag value **0x4000**, the KeyId, is mapped to *gre_key_id* node.

Building json parser
====================

