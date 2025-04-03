/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2025 Tom Herbert
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

#ifdef __GNUC__
	#if __GNUC__ > 6
		#include <optional>
		namespace xdp2gen { using std::optional; }
	#else
		#include <experimental/optional>
		namespace xdp2gen { using std::experimental::optional; }
	#endif
#else
	#include <optional>
	namespace xdp2gen { using std::optional; }
#endif

#include <vector>

namespace xdp2gen
{

struct tlv_node {
	std::string name, string_name, metadata, handler, type, overlay_table,
		unknown_overlay_ret, wildcard_node, check_length;

	std::vector<tlv_node> tlv_nodes;

	friend inline std::ostream& operator<<(std::ostream& os, tlv_node v) {
		return os << "[tlv_node {name: " << v.name <<
		       " string_name: " << v.string_name << " metadata: " <<
		       v. metadata << " handler: " << v. handler << " type: " <<
		       v. type << " overlay_table: " << v. overlay_table <<
		       " unknown_overlay_ret: " << v.unknown_overlay_ret <<
		       " wildcard_node: " << v. wildcard_node <<
		       " check_length: " << v. check_length << "}]";
	}
};

struct flag_fields_node {
	std::string name, string_name, metadata, handler, index;
};

struct vertex_property {
	// TODO enable node instead of its name
	// ParseNode& node;
	std::string name, parser_node, metadata, handler, table, tlv_table,
	flag_fields_table, unknown_proto_ret, wildcard_proto_def;

	std::vector<tlv_node> tlv_nodes;
	std::vector<flag_fields_node> flag_fields_nodes;

	friend inline std::ostream& operator<<(std::ostream& os,
					       vertex_property v) {
		return os << "[vertex {name: " << v.name << " parser_node: " <<
		       v.parser_node << " metadata: " << v.metadata <<
		       " handler: " << v. handler << " table: " << v. table <<
		       " tlv_table: " << v. tlv_table <<
		       "flag_fields_table: " << v. flag_fields_table <<
		       " unknown_proto_ret: " << v.unknown_proto_ret <<
		       " wildcard_proto_def: " << v. wildcard_proto_def <<
		       "}]";
	}
};

struct edge_property {
	std::string macro_name;
	std::string parser_node;
	bool back = false;
};

template <typename Container, typename Value> bool
contains(Container const &container, Value const &value)
{
	return std::find(container.begin(), container.end(),
			 value) != container.end ();
}

template <typename Graph> struct cycle_detector : public boost::bfs_visitor<> {
	typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex;
	typedef typename boost::graph_traits<Graph>::edge_descriptor edge;

	std::unordered_map<vertex, std::unordered_set<vertex> > sources;
	std::vector<edge> &back_edges;

	cycle_detector(std::vector<edge> &back_edges) : boost::bfs_visitor<>{},
		       back_edges{ back_edges } {}

	template <typename T>static inline T &
	remove_const(T const &o)
	{
		return const_cast<T &> (o);
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
template <typename Graph> struct vertice_leveler : public boost::bfs_visitor<> {
	typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex;
	typedef typename boost::graph_traits<Graph>::edge_descriptor edge;

	std::unordered_map<vertex, int> &levels;
	ssize_t &max_level;

	vertice_leveler(std::unordered_map<vertex, int> &levels,
			std::ptrdiff_t &max_level) :
				boost::bfs_visitor<>{},
			levels{ levels }, max_level{ max_level } { }

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

template <typename G> typename boost::graph_traits<G>::vertex_descriptor
find_vertex_by_name(G const &graph, const std::string &vertex_name)
{
	for (auto &&v : graph.vertex_set()) {
		auto name = graph[v].name;

		if (name == vertex_name)
                        return v;
	}
	std::cerr << "searched for " << vertex_name << " but not found" <<
		std::endl;
	throw std::runtime_error("Vertex not found");
}

template <typename G> optional<typename
				boost::graph_traits<G>::vertex_descriptor>
search_vertex_by_name(G const &graph, const std::string &vertex_name)
{
	for (auto &&v : graph.vertex_set()) {
		auto name = graph[v].name;
		if (name == vertex_name)
			return v;
	}
	return {};
}

template <typename G> std::vector<typename
				boost::graph_traits<G>::edge_descriptor>
back_edges(G &graph,
	   typename boost::graph_traits<G>::vertex_descriptor root_vertex)
{
	typedef typename boost::graph_traits<G>::edge_descriptor edge;
	auto back_edges = std::vector<edge>{};

	auto root = boost::root_vertex(root_vertex);

	boost::breadth_first_search(graph, root_vertex,
				    root.visitor(cycle_detector<G>
							{ back_edges }));

	return back_edges;
};

template <typename G> std::vector<std::vector<typename
				boost::graph_traits<G>::vertex_descriptor>>
	vertice_levels(G const &graph, typename
		       boost::graph_traits<G>::vertex_descriptor root_vertex)
{
	typedef typename boost::graph_traits<G>::vertex_descriptor vertex_t;
	auto vertice_levels = std::unordered_map<vertex_t, int>{};
	auto root = boost::root_vertex(root_vertex);
	vertice_levels[root_vertex] = 0;
	auto max_level = ssize_t{ 0 };

	boost::breadth_first_search(graph, root_vertex,
				    root.visitor(vertice_leveler<G>{
					vertice_levels, max_level
				    }));

	auto levels = std::vector<std::vector<vertex_t>> (max_level + 2);

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

template <typename G> void
dotify(G const &graph, std::string filename,
       typename boost::graph_traits<G>::vertex_descriptor root_vertex,
       std::vector<typename boost::graph_traits<G>::edge_descriptor>
							const &back_edges)
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
			    std::find(back_edges.begin(), back_edges.end(),
			    e) != back_edges.end())
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

struct table {
	std::string name;

	struct entry {
		std::string left, right;
	};

	std::vector<entry> entries;
};

template <typename G> void
connect_vertices(G &g, std::vector<xdp2gen::table> parser_tables)
{
	auto vs = vertices(g);

	for (auto &&src : boost::make_iterator_range(vs.first, vs.second)) {
		if (!g[src].table.empty()) {
			auto table_it = std::find_if(parser_tables.begin(),
					parser_tables.end(), [&] (auto &&tt) {
					return g[src].table == tt.name; });
			if (table_it != parser_tables.end ()) {
				for (auto &&entry : table_it->entries) {
					auto node_name = entry.right.substr(
						0, entry.right.find ('.'));

					if (auto pv =
					     xdp2gen::search_vertex_by_name(
							g, node_name)) {
						auto dst = *pv;
						auto edge = add_edge(src, dst,
								     g);
						g[edge.first] = { entry.left,
								  entry.right };
					} else {
						std::cerr <<
							"Not found destination "
							"edge: " <<
							node_name << std::endl;
					}
				}
			} else {
				std::cerr << "Not found table " <<
					g[src].table << std::endl;
			}
		}
	}
}

template <typename G> void
fill_tlv_node_to_vertices(G &g, std::vector<tlv_node> tlv_nodes,
			  std::vector<table> tlv_tables)
{
	auto vs = vertices(g);

	for (auto &&v : boost::make_iterator_range(vs.first, vs.second)) {
		if (!g[v].tlv_table.empty()) {
			auto table_it = std::find_if(tlv_tables.begin(),
						     tlv_tables.end(),
						     [&] (auto &&tt) {
							return g[v].tlv_table ==
								tt.name; });
			if (table_it != tlv_tables.end()) {
				for (auto &&entry : table_it->entries) {
					auto node_name = entry.right;
					auto node_it = std::find_if(
							tlv_nodes.begin(),
							tlv_nodes.end(),
							[&] (auto &&n) {
						return n.name == node_name; });
					if (node_it != tlv_nodes.end()) {
						g[v].tlv_nodes.push_back(
								*node_it);
						g[v].tlv_nodes.back().type =
								entry.left;
					} else {
						std::cerr << "node TLV not "
							     "found" <<
							std::endl;
					}
				}
			} else {
				std::cerr << "Not found TLV table " <<
						g[v].tlv_table << std::endl;
			}
		}
	}
}

void
fill_tlv_overlay_to_tlv_node(std::vector<tlv_node>& tlv_nodes,
			  std::vector<table> tlv_tables)
{
	for (auto&& node : tlv_nodes) {
		if (!node.overlay_table.empty()) {
			auto table_it = std::find_if(tlv_tables.begin(),
					tlv_tables.end(), [&] (auto &&tt) {
					return node.overlay_table ==
								tt.name; });
			if (table_it != tlv_tables.end()) {
				for (auto &&entry : table_it->entries) {
					auto node_name = entry.right;
					auto node_it = std::find_if(
							tlv_nodes.begin (),
							tlv_nodes.end (),
							[&] (auto &&n) {
						return n.name == node_name; });
					if (node_it != tlv_nodes.end()) {
						std::cout << "Found TLV for overlay table " << node.overlay_table << std::endl;
						node.tlv_nodes.push_back(
								*node_it);
						node.tlv_nodes.back().type =
								entry.left;
					} else {
						std::cerr << "node TLV not "
							     "found" <<
							std::endl;
					}
				}
			} else {
				std::cerr << "Not found overlay TLV table " <<
						node.overlay_table << std::endl;
			}
		}
	}
}

template <typename G> void
fill_flag_fields_node_to_vertices(G &g, std::vector<flag_fields_node> nodes,
			  std::vector<table> tables)
{
	auto vs = vertices(g);

	for (auto &&v : boost::make_iterator_range(vs.first, vs.second)) {
		if (g[v].flag_fields_table.empty())
			continue;

		auto table_it = std::find_if(tables.begin(), tables.end(),
					     [&] (auto &&tt) {
				return g[v].flag_fields_table == tt.name; });

		if (table_it != tables.end()) {
			for (auto &&entry : table_it->entries) {
				auto node_name = entry.right;
				auto node_it = std::find_if(nodes.begin(),
							    nodes.end(),
							    [&] (auto &&n) {
					return n.name == node_name; });

				if (node_it != nodes.end()) {
					g[v].flag_fields_nodes.push_back(
							*node_it);
					g[v].flag_fields_nodes.back().index =
							entry.left;
				} else if (node_name ==
					   "XDP2_FLAG_NODE_NULL") {
					g[v].flag_fields_nodes.push_back(
						{"XDP2_FLAG_NODE_NULL", ""});
					g[v].flag_fields_nodes.back().index =
							entry.left;
				} else {
					std::cerr << "node flag fields not "
						"found " << node_name <<
						std::endl;
				}
			}
		} else {
			std::cerr << "Not found flag fields table " <<
					g[v].flag_fields_table << std::endl;
		}
	}
}

using graph_t = boost::adjacency_list<boost::vecS, boost::vecS,
	boost::directedS, xdp2gen::vertex_property,
	xdp2gen::edge_property, boost::no_property, boost::vecS>;
using vertex_descriptor_t = boost::graph_traits<graph_t>::vertex_descriptor;
using root_t = std::tuple<std::string, vertex_descriptor_t, bool, bool>;

} // namespace xdp2gen

#endif
