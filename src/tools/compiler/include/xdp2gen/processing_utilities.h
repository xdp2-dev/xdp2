/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2020,2021 SiXDP2 Inc.
 *
 * Authors: Pedro Daniel <pedro.daniel@expertisesolutions.com.br>
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

#ifndef XDP2GEN_INCLUDE_XDP2GEN_UTILITIES_H
#define XDP2GEN_INCLUDE_XDP2GEN_UTILITIES_H

// Standard
// - Io manip
#include <sstream>
#include <iostream>
#include <filesystem>
// - Data manip
#include <ranges> // C++20 only
#include <numeric>
#include <optional>
// - Data types
#include <string>

// XDP2
#include "xdp2gen/graph.h"
#include "xdp2gen/python_generators.h"
#include "xdp2gen/llvm/basic_block_iteration.h"

#include "xdp2gen/ast-consumer/proto-nodes.h"
#include "xdp2gen/ast-consumer/proto-tables.h"
#include "xdp2gen/ast-consumer/flag-fields.h"
#include <xdp2gen/program-options/log_handler.h>

namespace xdp2gen
{

void transfer_data_from_proto_node_data_to_vertex(
    xdp2_proto_node_extract_data const &proto_node_data,
    xdp2gen::vertex_property &vertex)
{
    // Inserts data in graph
    vertex.proto_decl_name = proto_node_data.decl_name;
    vertex.proto_name = proto_node_data.name;
    vertex.proto_min_len = proto_node_data.min_len;
    vertex.proto_len = proto_node_data.len;
    vertex.proto_next_proto = proto_node_data.next_proto;
    vertex.tlv_type = proto_node_data.tlv_type;
    vertex.tlv_len = proto_node_data.tlv_len;
    vertex.tlv_start_offset = proto_node_data.tlv_start_offset;
    vertex.pad1_enable = proto_node_data.pad1_enable;
    vertex.padn_enable = proto_node_data.padn_enable;
    vertex.eol_enable = proto_node_data.eol_enable;
    vertex.pad1_val = proto_node_data.pad1_val;
    vertex.padn_val = proto_node_data.padn_val;
    vertex.eol_val = proto_node_data.eol_val;
    vertex.overlay = proto_node_data.overlay;
    vertex.encap = proto_node_data.encap;

    if (proto_node_data.flag_fields_name.has_value()) {
        vertex.flag_fields_name = proto_node_data.flag_fields_name.value();
    }

    if (proto_node_data.start_fields_offset.has_value()) {
        vertex.start_fields_offset =
            proto_node_data.start_fields_offset.value();
    }
    if (proto_node_data.get_flags.has_value()) {
        vertex.get_flags = proto_node_data.get_flags.value();
    }
}

std::vector<xdp2_proto_table_extract_data>::const_iterator find_table_by_name(
    std::string &table_name,
    std::vector<xdp2_proto_table_extract_data> const &consumed_proto_table_data)
{
    // Tries to find correct node to get data
    auto match_xdp2_table = std::find_if(
        consumed_proto_table_data.begin(), consumed_proto_table_data.end(),
        [&table_name](const xdp2_proto_table_extract_data &table) -> bool {
            plog::log(std::cout)
                << "    - Compared with: " << table.decl_name << std::endl;
            return (table.decl_name == table_name);
        });

    return match_xdp2_table;
}

std::vector<xdp2_flag_fields_extract_data>::const_iterator
find_flag_fields_by_name(std::string &name,
                         std::vector<xdp2_flag_fields_extract_data> const
                             &consumed_flag_fields_data)
{
    // Tries to find correct node to get data
    auto match_xdp2_table = std::find_if(
        consumed_flag_fields_data.begin(), consumed_flag_fields_data.end(),
        [&name](const xdp2_flag_fields_extract_data &table) -> bool {
            plog::log(std::cout)
                << "    - Compared with: " << table.name << std::endl;
            return (table.name == name);
        });

    return match_xdp2_table;
}

template <typename G>
void connect_vertices(G &g, auto const &src,
                      std::vector<xdp2_proto_table_extract_data> const
                          &consumed_proto_table_data)
{
    if (!g[src].table.empty()) {
        auto table_it =
            find_table_by_name(g[src].table, consumed_proto_table_data);

        if (table_it != consumed_proto_table_data.end()) {
            for (auto &&entry : table_it->entries) {
                auto node_name = entry.second.substr(0, entry.second.find('.'));

                if (auto pv = xdp2gen::search_vertex_by_name(g, node_name)) {
                    auto dst = *pv;
                    auto edge = add_edge(src, dst, g);

                    auto to_hex = [](unsigned int mask) -> std::string {
                        std::ostringstream ss;
                        ss << "0x" << std::hex << mask;
                        return ss.str();
                    };
                    g[edge.first] = { to_hex(entry.first), entry.second, false,
                                      entry.first };
                } else {
                    plog::log(std::cerr) << "Not found destination "
                                            "edge: "
                                         << node_name << std::endl;
                }
            }
        } else {
            plog::log(std::cerr)
                << "Not found table " << g[src].table << std::endl;
        }
    }
}

void process_proto_node_tables(xdp2gen::graph_t &graph, auto const &vd,
                               std::vector<xdp2_proto_table_extract_data> const
                                   &consumed_proto_table_data)
{
    auto &&vertex = graph[vd];

    std::string proto_table_search_expr = vertex.table;

    plog::log(std::cout)
        << "  - Proto Table Target Search Expr: " << proto_table_search_expr
        << std::endl;

    auto match_xdp2_table = xdp2gen::find_table_by_name(
        proto_table_search_expr, consumed_proto_table_data);

    // If there is a match
    if (match_xdp2_table != consumed_proto_table_data.end()) {
        plog::log(std::cout)
            << "  - FOUND CORRESPONDENT PROTO TABLE" << std::endl;

        for (auto out_edge :
             boost::make_iterator_range(boost::out_edges(vd, graph))) {
            auto &&out_edge_obj = graph[out_edge];
            std::string entry_target_search_expr = out_edge_obj.parser_node;
            plog::log(std::cout) << "    - Entry Target Search Expr: "
                                 << entry_target_search_expr << std::endl;

            auto match_table_entry = std::find_if(
                (*match_xdp2_table).entries.begin(),
                (*match_xdp2_table).entries.end(),
                [&entry_target_search_expr](
                    std::pair<unsigned int, std::string> entry) -> bool {
                    plog::log(std::cout)
                        << "      - Compared with: " << entry.second
                        << std::endl;
                    return (entry.second == entry_target_search_expr);
                });

            if (match_table_entry != (*match_xdp2_table).entries.end()) {
                plog::log(std::cout)
                    << "      - FOUND CORRESPONDENT ENTRY" << std::endl;
                plog::log(std::cout)
                    << "        - key: " << (*match_table_entry).first
                    << std::endl;
                plog::log(std::cout)
                    << "        - macro_name: " << out_edge_obj.macro_name
                    << std::endl;
                out_edge_obj.macro_name_value = (*match_table_entry).first;
            } else {
                plog::log(std::cout)
                    << "      - NOT ABLEFOUND CORRESPONDENT ENTRY" << std::endl;
            }
        }
    } else {
        plog::log(std::cout)
            << "  - NOT ABLE TO FOUND CORRESPONDENT PROTO TABLE" << std::endl;
    }
}

void handle_tlv_nodes_tables(
    std::vector<xdp2gen::tlv_node> &tlv_nodes,
    std::vector<xdp2_proto_table_extract_data>::const_iterator
        match_xdp2_tlv_table,
    std::vector<xdp2_proto_table_extract_data> const &consumed_proto_table_data)
{
    if (match_xdp2_tlv_table != consumed_proto_table_data.end()) {
        for (auto &tlv_node_item : tlv_nodes) {
            if (!tlv_node_item.tlv_nodes.empty()) {
                auto match_xdp2_tlv_table = find_table_by_name(
                    tlv_node_item.overlay_table, consumed_proto_table_data);
                handle_tlv_nodes_tables(tlv_node_item.tlv_nodes,
                                        match_xdp2_tlv_table,
                                        consumed_proto_table_data);
            }

            std::string tlv_node_name = tlv_node_item.name;

            auto match_table_entry = std::find_if(
                (*match_xdp2_tlv_table).entries.begin(),
                (*match_xdp2_tlv_table).entries.end(),
                [&tlv_node_name](
                    std::pair<unsigned int, std::string> entry) -> bool {
                    plog::log(std::cout)
                        << "      - Compared with: " << entry.second
                        << std::endl;
                    return (entry.second == tlv_node_name);
                });

            if (match_table_entry != (*match_xdp2_tlv_table).entries.end()) {
                plog::log(std::cout)
                    << "      - FOUND CORRESPONDENT ENTRY" << std::endl;
                plog::log(std::cout)
                    << "        - key: " << (*match_table_entry).first
                    << std::endl;
                tlv_node_item.key = (*match_table_entry).first;
            } else {
                plog::log(std::cout)
                    << "      - NOT ABLEFOUND CORRESPONDENT ENTRY" << std::endl;
            }
        }
    }
}

void process_tlv_nodes_tables(xdp2gen::vertex_property &vertex,
                              std::vector<xdp2_proto_table_extract_data> const
                                  &consumed_proto_table_data)
{
    plog::log(std::cout)
        << "  - Proto TLV Table Target Search Expr: " << vertex.tlv_table;

    auto match_xdp2_tlv_table =
        find_table_by_name(vertex.tlv_table, consumed_proto_table_data);

    handle_tlv_nodes_tables(vertex.tlv_nodes, match_xdp2_tlv_table,
                            consumed_proto_table_data);
}

void process_flag_fields_nodes_tables(
    xdp2gen::vertex_property &vertex,
    std::vector<xdp2_proto_table_extract_data> const &consumed_proto_table_data,
    std::vector<xdp2_flag_fields_extract_data> const &consumed_flag_fields_data)
{
    plog::log(std::cout) << "  - Proto Flag Field Table Target Search Expr: "
                         << vertex.flag_fields_table << std::endl;

    if (!vertex.flag_fields_table.empty()) {
        auto table_it = find_table_by_name(vertex.flag_fields_table,
                                           consumed_proto_table_data);

        if (table_it != consumed_proto_table_data.end()) {
            for (int i = 0; i < table_it->entries.size(); ++i) {
                auto const &entry = table_it->entries[i];

                auto node_name = entry.second.substr(0, entry.second.find('.'));

                vertex.flag_fields_nodes[i].key = table_it->entries[i].first;
            }

        } else {
            plog::log(std::cerr)
                << "Not found table " << vertex.flag_fields_table << std::endl;
        }
    }

    auto match_xdp2_flag_fields = find_flag_fields_by_name(
        vertex.flag_fields_name, consumed_flag_fields_data);

    if (match_xdp2_flag_fields != consumed_flag_fields_data.end()) {
        if ((*match_xdp2_flag_fields).fields.size() ==
            vertex.flag_fields_nodes.size()) {
            for (int i = 0; i < vertex.flag_fields_nodes.size(); ++i) {
                int key = vertex.flag_fields_nodes[i].key;

                if (key < (*match_xdp2_flag_fields).fields.size()) {
                    vertex.flag_fields_nodes[i].size =
                        (*match_xdp2_flag_fields).fields[key].size;
                    vertex.flag_fields_nodes[i].flag =
                        (*match_xdp2_flag_fields).fields[key].flag;
                }
            }
        }
    }
}

}

#endif /* SRC_TOOLS_COMPILER_INCLUDE_XDP2GEN_UTILITIES_H_ */
