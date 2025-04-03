/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2020,2021 SiXDP2 Inc.
 *
 * Authors: Felipe Magno de Almeida <felipe@expertise.dev>
 *          João Paulo Taylor Ienczak Zanette <joao.tiz@expertise.dev>
 *          Lucas Cavalcante de Sousa <lucas@expertise.dev>
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

#ifndef XDP2_GRAPH_H
#define XDP2_GRAPH_H

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/directed_graph.hpp>
#include <boost/graph/graphviz.hpp>
#include <xdp2gen/llvm/proto_next_pattern.h>
#include <xdp2gen/llvm/metadata_pattern.h>
#include <xdp2gen/clang-ast/metadata_spec.h>
#include <xdp2gen/program-options/log_handler.h>
#include <functional>

#ifdef __GNUC__
#if __GNUC__ > 6
#include <optional>
namespace xdp2gen
{
using std::optional;
}
#else
#include <experimental/optional>
namespace xdp2gen
{
using std::experimental::optional;
}
#endif
#else
#include <optional>
namespace xdp2gen
{
using std::optional;
}
#endif

#include <vector>

namespace xdp2gen
{

struct vertex_property;

struct metadata_transfer {
    std::string name;
    std::variant<llvm::metadata_transfer, llvm::metadata_write_constant,
                 llvm::metadata_write_header_offset,
                 llvm::metadata_write_header_length,
                 llvm::metadata_value_transfer>
        transfer;
};

struct tlv_node {
    std::string name, string_name, metadata, handler, type, overlay_table,
        unknown_overlay_ret, wildcard_node, check_length;

    std::vector<xdp2gen::metadata_transfer> metadata_transfers;

    unsigned int key;

    vertex_property *parent_parse_node = nullptr;

    using tlv_node_ref = std::reference_wrapper<tlv_node>;
    using tlv_node_ref_const = std::reference_wrapper<tlv_node const>;

    struct tlv_node_hash {
        size_t operator()(tlv_node_ref const &tlvnode) const
        {
            std::hash<std::string> str_hash;
            std::hash<int> int_hash;

            return str_hash(tlvnode.get().name) +
                   str_hash(tlvnode.get().metadata) +
                   str_hash(tlvnode.get().handler) +
                   str_hash(tlvnode.get().overlay_table) +
                   str_hash(tlvnode.get().unknown_overlay_ret) +
                   str_hash(tlvnode.get().wildcard_node) +
                   str_hash(tlvnode.get().check_length) +
                   int_hash(tlvnode.get().key);
        }

        size_t operator()(tlv_node const &tlvnode) const
        {
            std::hash<std::string> str_hash;
            std::hash<int> int_hash;

            return str_hash(tlvnode.name) + str_hash(tlvnode.metadata) +
                   str_hash(tlvnode.handler) + str_hash(tlvnode.overlay_table) +
                   str_hash(tlvnode.unknown_overlay_ret) +
                   str_hash(tlvnode.wildcard_node) +
                   str_hash(tlvnode.check_length) + int_hash(tlvnode.key);
        }
    };

    friend inline bool operator==(tlv_node const &lhs, tlv_node const &rhs)
    {
        return lhs.name == rhs.name;
    }

    using unordered_tlv_node_ref_set =
        std::unordered_set<tlv_node_ref, tlv_node_hash>;
    using unordered_tlv_node_ref_const_set =
        std::unordered_set<tlv_node_ref_const, tlv_node_hash>;
    using tlv_node_ref_vec = std::vector<tlv_node_ref>;
    using tlv_node_ref_const_vec = std::vector<tlv_node_ref_const>;

    std::vector<tlv_node> tlv_nodes;

    tlv_node::tlv_node_ref_vec get_all_tlv_node_list()
    {
        if (tlv_nodes.empty())
            return {};

        tlv_node::tlv_node_ref_vec tlv_node_ref_list(this->tlv_nodes.begin(),
                                                     this->tlv_nodes.end());

        for (auto &tlv_node_item : tlv_nodes) {
            auto const &tmp_tlv_node_ref =
                tlv_node_item.get_all_tlv_node_list();
            tlv_node_ref_list.insert(tlv_node_ref_list.end(),
                                     tmp_tlv_node_ref.begin(),
                                     tmp_tlv_node_ref.end());
        }

        return tlv_node_ref_list;
    }

    tlv_node::unordered_tlv_node_ref_set get_all_tlv_nodes_ref_set()
    {
        if (tlv_nodes.empty())
            return {};

        tlv_node::tlv_node_ref_vec const &tlv_node_ref_list =
            this->get_all_tlv_node_list();
        tlv_node::unordered_tlv_node_ref_set tlv_node_ref_set(
            tlv_node_ref_list.begin(), tlv_node_ref_list.end());

        return tlv_node_ref_set;
    }

    friend inline std::ostream &operator<<(std::ostream &os, tlv_node v)
    {
        return os << "[tlv_node {name: " << v.name
                  << " string_name: " << v.string_name
                  << " metadata: " << v.metadata << " handler: " << v.handler
                  << " type: " << v.type
                  << " overlay_table: " << v.overlay_table
                  << " unknown_overlay_ret: " << v.unknown_overlay_ret
                  << " wildcard_node: " << v.wildcard_node
                  << " check_length: " << v.check_length << "}]";
    }
};

struct flag_fields_node {
    std::string name, string_name, metadata, handler, index;

    int key, flag;

    size_t size;

    std::vector<xdp2gen::metadata_transfer> metadata_transfers;

    using flag_fields_ref = std::reference_wrapper<flag_fields_node>;
    using flag_fields_ref_vec = std::vector<flag_fields_ref>;
};

struct vertex_property {
    std::string name, parser_node, metadata, handler, post_handler, table,
        tlv_table, flag_fields_table, flag_fields_name, unknown_proto_ret,
        wildcard_proto_node, start_fields_offset, get_flags, tlv_wildcard_node;

    std::optional<std::size_t> handler_blockers, handler_watchers;

    clang_ast::metadata_record metadata_record;

    std::optional<bool> overlay;
    std::optional<bool> encap;

    std::optional<size_t> proto_min_len;
    std::optional<std::string> proto_decl_name, proto_name, proto_len,
        proto_next_proto;

    std::optional<int> unknown_ret;

    std::optional<xdp2gen::llvm::packet_buffer_offset_masked_multiplied>
        next_proto_data, len_data, flags_data;
    std::optional<xdp2gen::llvm::condition> cond_exprs;
    std::vector<metadata_transfer> metadata_transfers;

    std::optional<std::string> tlv_type;
    std::optional<xdp2gen::llvm::packet_buffer_load> tlv_type_val;
    std::optional<std::string> tlv_len;
    std::optional<xdp2gen::llvm::packet_buffer_offset_masked_multiplied>
        tlv_len_val;
    std::optional<std::string> tlv_start_offset;
    std::optional<xdp2gen::llvm::constant_value> tlv_start_offset_val;
    std::optional<__u8> pad1_val;
    std::optional<__u8> padn_val;
    std::optional<__u8> eol_val;
    std::optional<bool> pad1_enable;
    std::optional<bool> padn_enable;
    std::optional<bool> eol_enable;
    std::optional<__u16> max_loop;
    std::optional<__u16> max_non;
    std::optional<__u8> max_plen;
    std::optional<__u8> max_c_pad;
    std::optional<__u8> disp_limit_exceed;
    std::optional<__u8> exceed_loop_cnt_is_err;

    std::vector<tlv_node> tlv_nodes;

    std::optional<tlv_node> tlv_wildcard;

    tlv_node::tlv_node_ref_vec get_all_tlv_node_list()
    {
        if (tlv_nodes.empty())
            return {};

        tlv_node::tlv_node_ref_vec tlv_node_ref_list(this->tlv_nodes.begin(),
                                                     this->tlv_nodes.end());

        for (auto &tlv_node_item : tlv_nodes) {
            auto const &tmp_tlv_node_ref =
                tlv_node_item.get_all_tlv_node_list();
            tlv_node_ref_list.insert(tlv_node_ref_list.end(),
                                     tmp_tlv_node_ref.begin(),
                                     tmp_tlv_node_ref.end());
        }

        if (tlv_wildcard.has_value())
            tlv_node_ref_list.push_back(tlv_wildcard.value());

        return tlv_node_ref_list;
    }

    tlv_node::unordered_tlv_node_ref_set get_all_tlv_nodes_ref_set()
    {
        if (tlv_nodes.empty())
            return {};

        tlv_node::tlv_node_ref_vec const &tlv_node_ref_list =
            this->get_all_tlv_node_list();
        tlv_node::unordered_tlv_node_ref_set tlv_node_ref_set(
            tlv_node_ref_list.begin(), tlv_node_ref_list.end());

        return tlv_node_ref_set;
    }

    std::vector<flag_fields_node> flag_fields_nodes;

    std::optional<size_t> start_fields_offset_value;

    //-1 for descending ordering, 0 for no order, 1 for ascending ordering
    int verify_flag_ordering()
    {
        int result = 0;

        if (!flag_fields_nodes.empty() && flag_fields_nodes.size() > 1) {
            if (flag_fields_nodes[0].flag > flag_fields_nodes[1].flag)
                result = -1;
            else
                result = 1;

            if (flag_fields_nodes.size() > 2) {
                for (std::size_t i = 2; i < flag_fields_nodes.size(); ++i) {
                    bool is_desc_order_broken = result == -1 &&
                                                (flag_fields_nodes[i - 1].flag <
                                                 flag_fields_nodes[i].flag);

                    bool is_asc_order_broken = result == 1 &&
                                               (flag_fields_nodes[i - 1].flag >
                                                flag_fields_nodes[i].flag);

                    if (is_desc_order_broken || is_asc_order_broken) {
                        result = 0;
                        break;
                    }
                }
            }
        }

        return result;
    }

    friend inline std::ostream &operator<<(std::ostream &os, vertex_property v)
    {
        return os << "[vertex {name: " << v.name
                  << " parser_node: " << v.parser_node
                  << " metadata: " << v.metadata << " handler: " << v.handler
                  << " table: " << v.table << " tlv_table: " << v.tlv_table
                  << " flag_fields_table: " << v.flag_fields_table
                  << " unknown_proto_ret: " << v.unknown_proto_ret
                  << " wildcard_proto_node: " << v.wildcard_proto_node << "}]";
    }
};

struct edge_property {
    std::string macro_name;
    std::string parser_node;
    bool back = false;
    unsigned int macro_name_value;
};

template <typename Container, typename Value>
bool contains(Container const &container, Value const &value)
{
    return std::find(container.begin(), container.end(), value) !=
           container.end();
}

template <typename Graph>
struct cycle_detector : public boost::bfs_visitor<> {
    typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex;
    typedef typename boost::graph_traits<Graph>::edge_descriptor edge;

    std::unordered_map<vertex, std::unordered_set<vertex>> sources;
    std::vector<edge> &back_edges;

    cycle_detector(std::vector<edge> &back_edges)
        : boost::bfs_visitor<>{}
        , back_edges{ back_edges }
    {
    }

    template <typename T>
    static inline T &remove_const(T const &o)
    {
        return const_cast<T &>(o);
    }

    void examine_edge(edge e, Graph const &graph)
    {
        if (contains(sources[e.m_source], e.m_target)) {
            back_edges.push_back(e);
            remove_const(graph[e]).back = true;
        }

        sources[e.m_target].insert(e.m_source);
        const auto &src_back_edges = sources[e.m_source];
        sources[e.m_target].insert(src_back_edges.begin(),
                                   src_back_edges.end());
    }
};

/*
 * Sets each vertice's depth level in the graph. Leaf nodes are set to be at
 * depth -1 so we don't need any overhead into updating their level on and on
 * based on maximum level found as the algorithm traverses in the tree.
 */
template <typename Graph>
struct vertice_leveler : public boost::bfs_visitor<> {
    typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex;
    typedef typename boost::graph_traits<Graph>::edge_descriptor edge;

    std::unordered_map<vertex, int> &levels;
    ssize_t &max_level;

    vertice_leveler(std::unordered_map<vertex, int> &levels,
                    std::ptrdiff_t &max_level)
        : boost::bfs_visitor<>{}
        , levels{ levels }
        , max_level{ max_level }
    {
    }

    void examine_edge(edge e, Graph const &graph)
    {
        if (levels.find(e.m_target) != levels.end())
            return;

        if (levels.find(e.m_source) == levels.end())
            levels[e.m_source] = 0;

        if (out_degree(e.m_target, graph) == 0) {
            levels[e.m_target] = -1;
            return;
        }

        auto next_level = levels[e.m_source] + 1;
        levels[e.m_target] = next_level;
        if (next_level > max_level)
            max_level = next_level;
    }
};

template <typename G>
typename boost::graph_traits<G>::vertex_descriptor
find_vertex_by_name(G const &graph, const std::string &vertex_name)
{
    for (auto &&v : graph.vertex_set()) {
        auto name = graph[v].name;

        if (name == vertex_name)
            return v;
    }
    plog::log(std::cerr)
        << "searched for " << vertex_name << " but not found" << std::endl;
    throw std::runtime_error("Vertex not found");
}

template <typename G>
optional<typename boost::graph_traits<G>::vertex_descriptor>
search_vertex_by_name(G const &graph, const std::string &vertex_name)
{
    for (auto &&v : graph.vertex_set()) {
        auto name = graph[v].name;
        if (name == vertex_name)
            return v;
    }
    return {};
}

template <typename G>
std::vector<typename boost::graph_traits<G>::edge_descriptor>
back_edges(G &graph,
           typename boost::graph_traits<G>::vertex_descriptor root_vertex)
{
    typedef typename boost::graph_traits<G>::edge_descriptor edge;
    auto back_edges = std::vector<edge>{};

    auto root = boost::root_vertex(root_vertex);

    boost::breadth_first_search(graph, root_vertex,
                                root.visitor(cycle_detector<G>{ back_edges }));

    return back_edges;
};

template <typename G>
std::vector<std::vector<typename boost::graph_traits<G>::vertex_descriptor>>
vertice_levels(G const &graph,
               typename boost::graph_traits<G>::vertex_descriptor root_vertex)
{
    typedef typename boost::graph_traits<G>::vertex_descriptor vertex_t;
    auto vertice_levels = std::unordered_map<vertex_t, int>{};
    auto root = boost::root_vertex(root_vertex);
    vertice_levels[root_vertex] = 0;
    auto max_level = ssize_t{ 0 };

    boost::breadth_first_search(
        graph, root_vertex,
        root.visitor(vertice_leveler<G>{ vertice_levels, max_level }));

    auto levels = std::vector<std::vector<vertex_t>>(max_level + 2);

    for (auto &&pair : vertice_levels) {
        auto vertex = pair.first;
        auto level = pair.second;

        if (level == -1)
            levels.at(max_level + 1).push_back(vertex);
        else
            levels.at(level).push_back(vertex);
    }

    return levels;
}

template <typename G>
void dotify(G const &graph, std::string filename,
            typename boost::graph_traits<G>::vertex_descriptor root_vertex,
            std::vector<typename boost::graph_traits<G>::edge_descriptor> const
                &back_edges)
{
    typedef typename boost::graph_traits<G>::vertex_descriptor vertex;
    typedef typename boost::graph_traits<G>::edge_descriptor edge;

    struct vertex_writer {
        G const &graph;

        void operator()(std::ostream &out, vertex const &u) const
        {
            out << "[label=\"" << graph[u].name << "\"]";
        }
    };

    struct edge_writer {
        G const &graph;
        std::vector<edge> const &back_edges;

        void operator()(std::ostream &out, edge const &e) const
        {
            if (source(e, graph) == target(e, graph) ||
                std::find(back_edges.begin(), back_edges.end(), e) !=
                    back_edges.end())
                out << "[color=red]";
        }
    };

    struct graph_writer {
        G const &graph;
        std::vector<std::vector<vertex>> const &levels;

        void operator()(std::ostream &out) const
        {
            std::size_t i = 0;
            auto rank = "same";

            for (const auto &level : levels) {
                if (i == levels.size() - 1)
                    rank = "max";

                out << "{rank = " << rank << "; ";
                for (const auto &vertex : level)
                    out << vertex << " ";
                out << "}";
                ++i;
            }
        }
    };

    auto levels = xdp2gen::vertice_levels(graph, root_vertex);
    auto file = std::ofstream{ filename };

    write_graphviz(file, graph, vertex_writer{ graph },
                   edge_writer{ graph, back_edges },
                   graph_writer{ graph, levels });
}

template <typename G>
std::pair<typename boost::graph_traits<G>::vertex_descriptor, bool>
insert_node_by_name(G &graph, std::string name)
{
  plog::log(std::cout) << "insert_node_by_name " << name << std::endl;

    auto pv = search_vertex_by_name(graph, name);

    if (!pv) {
        auto &&u = add_vertex(graph);
        graph[u] = { name };
        return { u, true };
    }

    return { *pv, false };
}

struct table {
    std::string name;

    struct entry {
        std::string left, right;
    };

    std::vector<entry> entries;

    friend inline std::ostream &operator<<(std::ostream &os, table &t)
    {
        os << "============================="
           << "\n"
           << t.name << "\n";

        for (auto &item : t.entries)
            os << "{" << item.left << ", " << item.right << "}"
               << "\n";

        os << "=============================" << std::endl;

        return os;
    }
};

template <typename G>
void connect_vertices(G &g, std::vector<xdp2gen::table> parser_tables)
{
    auto vs = vertices(g);

    for (auto &&src : boost::make_iterator_range(vs.first, vs.second)) {
        if (!g[src].table.empty()) {
            auto table_it = std::find_if(parser_tables.begin(),
                                         parser_tables.end(), [&](auto &&tt) {
                                             return g[src].table == tt.name;
                                         });
            if (table_it != parser_tables.end()) {
                for (auto &&entry : table_it->entries) {
                    auto node_name =
                        entry.right.substr(0, entry.right.find('.'));

                    if (auto pv =
                            xdp2gen::search_vertex_by_name(g, node_name)) {
                        auto dst = *pv;
                        auto edge = add_edge(src, dst, g);
                        g[edge.first] = { entry.left, entry.right };
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
}

template <typename G>
void fill_tlv_node_to_vertices(G &g, std::vector<tlv_node> &tlv_nodes,
                               std::vector<table> &tlv_tables)
{
    auto vs = vertices(g);

    for (auto &&v : boost::make_iterator_range(vs.first, vs.second)) {
        if (!g[v].tlv_table.empty()) {
            auto table_it = std::find_if(tlv_tables.begin(), tlv_tables.end(),
                                         [&](auto &&tt) {
                                             return g[v].tlv_table == tt.name;
                                         });
            if (table_it != tlv_tables.end()) {
                for (auto &&entry : table_it->entries) {
                    auto node_name = entry.right;
                    auto node_it = std::find_if(tlv_nodes.begin(),
                                                tlv_nodes.end(), [&](auto &&n) {
                                                    return n.name == node_name;
                                                });
                    if (node_it != tlv_nodes.end()) {
                        g[v].tlv_nodes.push_back(*node_it);
                        g[v].tlv_nodes.back().type = entry.left;
                    } else {
                        plog::log(std::cerr) << "node TLV not "
                                                "found"
                                             << std::endl;
                    }
                }
            } else {
                plog::log(std::cerr)
                    << "Not found TLV table " << g[v].tlv_table << std::endl;
            }
        }
    }
}

void fill_tlv_overlay_to_tlv_node(std::vector<tlv_node> &tlv_nodes,
                                  std::vector<table> tlv_tables)
{
    for (auto &&node : tlv_nodes) {
        if (!node.overlay_table.empty()) {
            auto table_it = std::find_if(
                tlv_tables.begin(), tlv_tables.end(),
                [&](auto &&tt) { return node.overlay_table == tt.name; });
            if (table_it != tlv_tables.end()) {
                for (auto &&entry : table_it->entries) {
                    auto node_name = entry.right;
                    auto node_it = std::find_if(tlv_nodes.begin(),
                                                tlv_nodes.end(), [&](auto &&n) {
                                                    return n.name == node_name;
                                                });
                    if (node_it != tlv_nodes.end()) {
                        plog::log(std::cout) << "Found TLV for overlay table "
                                             << node.overlay_table << std::endl;
                        node.tlv_nodes.push_back(*node_it);
                        node.tlv_nodes.back().type = entry.left;
                    } else {
                        plog::log(std::cerr) << "node TLV not "
                                                "found"
                                             << std::endl;
                    }
                }
            } else {
                plog::log(std::cerr) << "Not found overlay TLV table "
                                     << node.overlay_table << std::endl;
            }
        }
    }
}

template <typename G>
void fill_flag_fields_node_to_vertices(G &g,
                                       std::vector<flag_fields_node> nodes,
                                       std::vector<table> tables)
{
    auto vs = vertices(g);

    for (auto &&v : boost::make_iterator_range(vs.first, vs.second)) {
        if (g[v].flag_fields_table.empty())
            continue;

        auto table_it =
            std::find_if(tables.begin(), tables.end(), [&](auto &&tt) {
                return g[v].flag_fields_table == tt.name;
            });

        if (table_it != tables.end()) {
            for (auto &&entry : table_it->entries) {
                auto node_name = entry.right;
                auto node_it =
                    std::find_if(nodes.begin(), nodes.end(),
                                 [&](auto &&n) { return n.name == node_name; });

                if (node_it != nodes.end()) {
                    g[v].flag_fields_nodes.push_back(*node_it);
                    g[v].flag_fields_nodes.back().index = entry.left;
                } else if (node_name == "XDP2_FLAG_NODE_NULL") {
                    g[v].flag_fields_nodes.push_back(
                        { "XDP2_FLAG_NODE_NULL", "" });
                    g[v].flag_fields_nodes.back().index = entry.left;
                } else {
                    plog::log(std::cerr) << "node flag fields not "
                                            "found "
                                         << node_name << std::endl;
                }
            }
        } else {
            plog::log(std::cerr) << "Not found flag fields table "
                                 << g[v].flag_fields_table << std::endl;
        }
    }
}

template <typename G>
void fill_tlv_wildcard_nodes(G &g, std::vector<tlv_node> &tlv_nodes)
{
    auto vs = vertices(g);

    for (auto &&v : boost::make_iterator_range(vs.first, vs.second)) {
        if (!g[v].tlv_wildcard_node.empty()) {
            for (auto &item : tlv_nodes) {
                if (g[v].tlv_wildcard_node == item.name) {
                    g[v].tlv_wildcard = item;
                }
            }
        }
    }
}

using graph_t =
    boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                          xdp2gen::vertex_property, xdp2gen::edge_property,
                          boost::no_property, boost::vecS>;
using vertex_descriptor_t = boost::graph_traits<graph_t>::vertex_descriptor;

template <typename G>
struct parser {
    std::string parser_name;
    boost::graph_traits<G>::vertex_descriptor root;
    bool dummy;
    bool ext;

    boost::graph_traits<G>::vertex_descriptor okay_target;
    bool okay_target_set = false;
    boost::graph_traits<G>::vertex_descriptor fail_target;
    bool fail_target_set = false;
    boost::graph_traits<G>::vertex_descriptor encap_target;
    bool encap_target_set = false;

    int max_nodes = 255;
    int max_encaps = 4;
    int max_frames = 4;
    int metameta_size = 64;
    int frame_size = 256;
    int num_counters = 255;
    int num_keys = 255;

    friend inline std::ostream &operator<<(std::ostream &os, parser &p)
    {
        os << "PARSER"
           << "\n";

        os << "paser_name: " << p.parser_name << "\n";

        os << "root: \n" << p.root << "\n";

        os << "dummy: " << (p.dummy ? "true" : "false") << "\n";

        os << "ext: " << (p.ext ? "true" : "false") << "\n";

        os << "-----------------------------" << std::endl;

        return os;
    }
};

} // namespace xdp2gen

#endif
