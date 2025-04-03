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

#ifndef XDP2GEN_INCLUDE_XDP2GEN_MACRO_DEFS_H
#define XDP2GEN_INCLUDE_XDP2GEN_MACRO_DEFS_H

#include "xdp2gen/x3/token_grammar.h"

namespace xdp2gen
{

template <typename G> std::pair<typename
				boost::graph_traits<G>::vertex_descriptor, bool>
insert_node_by_name(G &graph, std::string name)
{
	auto pv = search_vertex_by_name(graph, name);

	if (!pv) {
		auto &&u = add_vertex(graph);
		graph[u] = { name };
		return { u, true };
	}

	return { *pv, false };
}

template <typename ContainerT> std::string
get_identifier_from_tokens(ContainerT const &c)
{
	for (auto &&t : c) {
		if (t == boost::wave::T_IDENTIFIER) {
			auto v = t.get_value();

			return std::string(v.begin (), v.end ());
		}
	}
	return {};
}

template <typename G, typename CV, typename ContainerT> void
handle_parser_add(G &graph, CV &cv, std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 3) {
		auto parser_name = get_identifier_from_tokens(arguments[0]);
		auto name = get_identifier_from_tokens(arguments[2]);
		auto pv = search_vertex_by_name(graph, name);

		if (pv)
			cv.push_back({parser_name, *pv, true, false});
		else
			std::cout << "Could not find root "
				"vertex of name " << name << std::endl;
	} else {
		// should error'out
		std::cerr << "XDP2_PARSER_ADD should have 3 parameter" <<
								std::endl;
	}
}

template <typename G, typename CV, typename ContainerT> void
handle_parser(G &graph, CV &cv, std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 3) {
		auto parser_name = get_identifier_from_tokens(arguments[0]);
		auto name = get_identifier_from_tokens(arguments[2]);
		auto pv = search_vertex_by_name(graph, name);

		if (pv)
			cv.push_back({parser_name, *pv, false, false});
		else
			std::cout << "Could not find root "
				"vertex of name " << name << std::endl;
	} else {
		// should error'out
		std::cerr << "XDP2_PARSER should have 3 parameter" <<
								std::endl;
	}
}

template <typename G, typename CV, typename ContainerT> void
handle_parser_ext(G &graph, CV &cv, std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 3) {
		auto parser_name = get_identifier_from_tokens(arguments[0]);
		auto name = get_identifier_from_tokens(arguments[2]);
		auto pv = search_vertex_by_name(graph, name);

		if (pv)
			cv.push_back({parser_name, *pv, false, true});
		else
			std::cout << "Could not find root "
				"vertex of name " << name << std::endl;
	} else {
		// should error'out
		std::cerr << "XDP2_PARSER should have 3 parameter" <<
								std::endl;
	}
}

template <typename G, typename CV, typename ContainerT> void
handle_parser_xdp(G &graph, CV &cv, std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 3) {
		auto parser_name = get_identifier_from_tokens(arguments[0]);
		auto name = get_identifier_from_tokens(arguments[2]);
		auto pv = search_vertex_by_name(graph, name);

		if (pv)
			cv.push_back({parser_name, *pv, false, false});
		else
			std::cout << "Could not find root "
				"vertex of name " << name << std::endl;
	} else {
		// should error'out
		std::cerr << "XDP2_PARSER_XDP should have 3 parameter" <<
								std::endl;
	}
}

template <typename G, typename ContainerT> void
handle_decl_node(G &graph, std::vector<ContainerT> const &arguments)
{
	if (!arguments.empty()) {
		auto name = get_identifier_from_tokens(arguments[0]);

		insert_node_by_name(graph, name);
	} else {
		// should error'out
		std::cerr << "XDP2_DECL_PARSE_NODE or "
			     "XDP2_DECL_TLVS_PARSE_NODE should have "
			     "1 parameter" << std::endl;
	}
}

template <typename G, typename ContainerT> void
handle_make_leaf_node(G &graph, std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 4) {
		auto name = get_identifier_from_tokens(arguments[0]);
		auto &&node = graph[insert_node_by_name(graph, name).first];

		node.parser_node = get_identifier_from_tokens(arguments[1]);
		node.metadata = get_identifier_from_tokens(arguments[2]);
		node.handler = get_identifier_from_tokens(arguments[3]);
	} else {
		// should error'out
		std::cerr << "XDP2_MAKE_LEAF_PARSE_NODE should have "
			     "4 parameter" << std::endl;
	}
}

template <typename G, typename ContainerT> void
handle_make_leaf_tlv_node(G &graph, std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 5) {
		auto name = get_identifier_from_tokens(arguments[0]);
		auto &&node = graph[insert_node_by_name(graph, name).first];

		node.parser_node = get_identifier_from_tokens(arguments[1]);
		node.metadata = get_identifier_from_tokens(arguments[2]);
		node.handler = get_identifier_from_tokens(arguments[3]);
		node.tlv_table = get_identifier_from_tokens(arguments[4]);
	} else {
		// should error'out
		std::cerr << "XDP2_MAKE_LEAF_TLVS_PARSE_NODE should "
			     "have 5 parameter" << std::endl;
	}
}

template <typename NC, typename ContainerT> void
handle_make_tlv_overlay_node(NC &nodes,
			     std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 7) {
		auto name = get_identifier_from_tokens(arguments[0]);

		typename NC::value_type node{ name,
					      get_identifier_from_tokens(
								arguments[1]),
					      get_identifier_from_tokens(
								arguments[2]),
					      get_identifier_from_tokens(
								arguments[3]) };

		node.overlay_table = get_identifier_from_tokens(arguments[3]);
		node.unknown_overlay_ret =
				get_identifier_from_tokens(arguments[5]);
		node.wildcard_node = get_identifier_from_tokens(arguments[6]);

		std::cout << "tlv overlay " << node << std::endl;
		nodes.push_back(node);
	} else {
		// should error'out
		std::cerr << "XDP2_MAKE_TLV_OVERLAY_PARSE_NODE should "
			     "have 8 parameter" << std::endl;
	}
}

template <typename G, typename ContainerT> void
handle_make_flag_fields_node(G &graph, std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 6) {
		auto name = get_identifier_from_tokens(arguments[0]);
		auto &&node = graph[insert_node_by_name(graph, name).first];

		node.parser_node = get_identifier_from_tokens(arguments[1]);
		node.metadata = get_identifier_from_tokens(arguments[2]);
		node.handler = get_identifier_from_tokens(arguments[3]);
		node.table = get_identifier_from_tokens(arguments[4]);
		node.flag_fields_table = get_identifier_from_tokens(arguments[5]);
	} else {
		// should error'out
		std::cerr << "XDP2_MAKE_FLAG_FIELDS_PARSE_NODE should "
			     "have 6 parameter" << std::endl;
	}
}

template <typename G, typename ContainerT> void
handle_make_flag_fields_overlay_node(G &graph,
				std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 6) {
		auto name = get_identifier_from_tokens(arguments[0]);
		auto &&node = graph[insert_node_by_name(graph, name).first];

		node.parser_node = get_identifier_from_tokens(arguments[1]);
		node.metadata = get_identifier_from_tokens(arguments[2]);
		node.handler = get_identifier_from_tokens(arguments[3]);
		node.wildcard_proto_def =
				get_identifier_from_tokens(arguments[4]);
		node.flag_fields_table =
				get_identifier_from_tokens(arguments[5]);
	} else {
		// should error'out
		std::cerr << "XDP2_MAKE_FLAG_FIELDS_OVERLAY_PARSE_NODE should "
			     "have 6 parameter" << std::endl;
	}
}

template <typename G, typename ContainerT> void
handle_make_leaf_flag_fields_node(G &graph, std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 6) {
		auto name = get_identifier_from_tokens(arguments[0]);
		auto &&node = graph[insert_node_by_name(graph, name).first];

		node.parser_node = get_identifier_from_tokens(arguments[1]);
		node.metadata = get_identifier_from_tokens(arguments[2]);
		node.handler = get_identifier_from_tokens(arguments[3]);
		node.flag_fields_table =
				get_identifier_from_tokens(arguments[4]);
	} else {
		// should error'out
		std::cerr << "XDP2_MAKE_LEAF_FLAG_FIELDS_PARSE_NODE should "
			     "have 6 parameter" << std::endl;
	}
}

template <typename G, typename ContainerT> void
handle_make_node(G &graph, std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 5) {
		auto name = get_identifier_from_tokens(arguments[0]);
		auto &&node = graph[insert_node_by_name(graph, name).first];

		node.parser_node = get_identifier_from_tokens(arguments[1]);
		node.metadata = get_identifier_from_tokens(arguments[2]);
		node.handler = get_identifier_from_tokens(arguments[3]);
		node.table = get_identifier_from_tokens(arguments[4]);
	} else {
		// should error'out
		std::cerr << "XDP2_MAKE_PARSE_NODE should have 5 "
			     "parameter" << std::endl;
	}
}

template <typename G, typename ContainerT> void
handle_make_overlay_node(G &graph, std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 5) {
		auto name = get_identifier_from_tokens(arguments[0]);
		auto &&node = graph[insert_node_by_name(graph, name).first];

		node.parser_node = get_identifier_from_tokens(arguments[1]);
		node.metadata = get_identifier_from_tokens(arguments[2]);
		node.handler = get_identifier_from_tokens(arguments[3]);
		node.wildcard_proto_def = get_identifier_from_tokens(arguments[4]);
	} else {
		// should error'out
		std::cerr << "XDP2_MAKE_OVERLAY_PARSE_NODE should have 5 "
			     "parameter" << std::endl;
	}
}

template <typename NC, typename ContainerT> void
handle_make_tlv_node(NC &nodes, std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 4) {
		auto name = get_identifier_from_tokens(arguments[0]);

		typename NC::value_type node{ name,
					      get_identifier_from_tokens(
								arguments[1]),
					      get_identifier_from_tokens(
								arguments[2]),
					      get_identifier_from_tokens(
								arguments[3]) };
		nodes.push_back(node);
	} else {
		// should error'out
		std::cerr << "XDP2_MAKE_PARSE_TLV_NODE should have 4 "
			     "parameter" << std::endl;
	}
}

template <typename NC, typename ContainerT> void
handle_make_flag_field_node(NC &nodes, std::vector<ContainerT> const &arguments)
{
	if (arguments.size() == 3) {
		auto name = get_identifier_from_tokens(arguments[0]);

		typename NC::value_type node{ name, name,
					      get_identifier_from_tokens(
								arguments[1]),
					      get_identifier_from_tokens(
								arguments[2])
		};
		nodes.push_back(node);
	} else {
		// should error'out
		std::cerr << "XDP2_MAKE_FLAG_FIELD_PARSER_NODE should have 3 "
			     "parameters" << std::endl;
	}
}

namespace table_parser
{

namespace x3 = boost::spirit::x3;
using xdp2gen::x3::operator"" _t;
using xdp2gen::x3::identifier;
using xdp2gen::x3::token;

auto const table_left_parser = '{'_t >> +token;
auto const table_right_parser = '&'_t >> +(token - '}'_t) >> '}'_t;

} // namespace table_parser

template <typename G, typename Tables, typename ContainerT> void
handle_make_table(G &graph, Tables &tables,
		  std::vector<ContainerT> const &arguments)
{
	auto name = get_identifier_from_tokens(arguments[0]);
	auto iterator = std::next(arguments.begin());
	namespace x3 = boost::spirit::x3;
	auto last = arguments.end();

	typename Tables::value_type t{ name };

	while (iterator != last) {
		std::vector<std::string> left;

		if (x3::phrase_parse(iterator->begin(),iterator->end(),
				     table_parser::table_left_parser,
				     xdp2gen::x3::spaces, left)) {
			++iterator;
			if (iterator != last) {
				std::vector<std::string> right;
				auto current_parsing = iterator->begin();

				if (x3::phrase_parse(current_parsing,
						     iterator->end(),
						     table_parser::
							table_right_parser,
						     xdp2gen::x3::spaces,
							right) &&
				    x3::parse(current_parsing, iterator->end(),
					      *xdp2gen::x3::spaces) &&
				    current_parsing == iterator->end()) {
					++iterator;
					std::string left_string;
					for (auto &&l : left)
						left_string.insert(
							left_string.end(),
							l.begin(), l.end ());
					std::string right_string;
					for (auto &&r : right)
						right_string.insert(
							right_string.end(),
							r.begin(), r.end ());
					typename Tables::value_type::entry
						e{ left_string, right_string };
					t.entries.push_back(e);
				} else {
					std::cerr << "actually failed the "
						     "second time ";
					for (auto &&t : *iterator)
						std::cerr << t.get_value ();

					std::cerr << "\n";
					std::cerr << "stopped at ";
					for (auto &&t :
					     boost::make_iterator_range(
							current_parsing,
							iterator->end())) {
						std::cerr << t.get_value();
					}
					std::cerr << "\n";
					std::cerr << "right content ";
					for (auto &&t : right)
						std::cerr << t;

					std::cerr << "\n";
					break;
				}
			}
		} else {
			auto first = iterator->begin();

			if (first != iterator->end() &&
			    (x3::parse(first, iterator->end(),
				       *xdp2gen::x3::spaces) == false ||
			    first != iterator->end()))
				std::cerr << "parsed fail" << std::endl;
			break;
		}
	}

	tables.push_back(t);
}

} // namespace xdp2gen

#endif
