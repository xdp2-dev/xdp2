<!--(if 0)-->
// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2025 Tom Herbert
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
<!--(end)-->
#include <core.p4>
#include <v1model.p4>

<!--(for name in graph)-->
header @!name!@_t {
    <!--(if len(graph[name]['out_edges']) != 0)-->
    bit<@!graph[name]['next_proto_info']['bit_size']!@> next_proto;
    bit<@!graph[name]['proto_min_len']*8 - graph[name]['next_proto_info']['bit_offset'] - graph[name]['next_proto_info']['bit_size']!@> after_minimum_size;
    bit<@!graph[name]['next_proto_info']['bit_offset']!@> before_minimum_size;
    <!--(else)-->
    bit<@!graph[name]['proto_min_len']*8!@> minimum_size;
    <!--(end)-->
}
<!--(end)-->

struct headers {
<!--(for field in metadata_record["fields"])-->
<!--(end)-->
}

struct fwd_metadata_t {
}

struct metadata {
    fwd_metadata_t fwd_metadata;
}

error { UnknownProtocol }

<!--(macro generate_p4_parser)-->
parser ParserImpl(packet_in packet,
    out headers hdr,
    inout metadata meta,
    inout standard_metadata_t standard_metadata)
{
    state start {
        transition @!root_name!@;
    }
    state unknown_protocol {
        verify(false, error.UnknownProtocol);
        transition accept;
    }
    <!--(for name in graph)-->
    state @!name!@ {
        <!--(if len(graph[name]['out_edges']) != 0)-->
        // should actually use metadata information
        //packet.extract(hdr.@!name!@);
        transition select (hdr.@!name!@.next_proto) {
            <!--(for edge_target in graph[name]['out_edges'])-->
                    <!--(if 0)-->
            @!edge_target['macro_name_value']!@: @!edge_target['target']!@;
                    <!--(end)-->
            <!--(end)-->
            default: unknown_protocol;
        }
        <!--(else)-->
        // should actually use metadata information
        packet.extract(hdr.@!name!@);
        transition accept;
        <!--(end)-->
    }
    <!--(end)-->
}
<!--(end)-->


<!--(for root in roots)-->
@!generate_p4_parser(root_name=root['parser_name'])!@
<!--(end)-->
