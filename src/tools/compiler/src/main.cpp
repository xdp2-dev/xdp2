// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
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

#include <filesystem>
#include <iostream>
#include <numeric>
#include <string>

#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>

#include "xdp2gen/graph.h"
#include "xdp2gen/macro_defs.h"
#include "xdp2gen/python_generators.h"

namespace xdp2gen
{

template <typename G> struct MacroOnly :
    boost::wave::context_policies::default_preprocessing_hooks {
  typedef typename boost::graph_traits<G>::vertex_descriptor
              vertex_descriptor;
  typedef typename boost::graph_traits<G>::edge_descriptor
              edge_descriptor;

  std::vector<std::tuple<std::string, vertex_descriptor, bool, bool>> *roots;
  std::vector<table> *parser_tables;
  std::vector<tlv_node> *tlv_nodes;
  std::vector<flag_fields_node> *flag_fields_nodes;
  std::vector<table> *tlv_tables;
  std::vector<table> *flag_fields_tables;
  G *graph;

  MacroOnly(G &g, std::vector<table> &parser_tables,
      std::vector<table> &tlv_tables,
      std::vector<table> &flag_fields_tables,
      std::vector<tlv_node> &tlv_nodes,
      std::vector<flag_fields_node> &flag_fields_nodes,
      std::vector<std::tuple<std::string, vertex_descriptor, bool, bool>> &roots)
    : boost::wave::context_policies::default_preprocessing_hooks{},
      graph{ &g }, parser_tables (&parser_tables),
      tlv_tables (&tlv_tables), flag_fields_tables (&flag_fields_tables),
      tlv_nodes{ &tlv_nodes }, flag_fields_nodes{ &flag_fields_nodes },
      roots (&roots)
  {
  }

  // Ignores #include directives
  template <typename ContextType> bool
  found_include_directive(const ContextType &context,
        const std::string &filename, bool include_next)
  {
    return true;
  }

  // Output function like macro info
  template <typename ContextT, typename TokenT, typename ContainerT,
    typename IteratorT> bool
  expanding_function_like_macro(ContextT const &ctx,
    TokenT const &macro_name,
    std::vector<TokenT> const &parameters,
    ContainerT const &definition,
                            TokenT const &macrocall,
    std::vector<ContainerT> const &arguments,
    IteratorT const &seqstart,
    IteratorT const &seqend)
  {
    auto macro = macro_name.get_value();

    if (macro == "XDP2_DECL_PARSE_NODE" ||
        macro == "XDP2_DECL_TLVS_PARSE_NODE") {
      xdp2gen::handle_decl_node(*graph, arguments);
    } else if (macro == "XDP2_MAKE_PROTO_TABLE") {
      xdp2gen::handle_make_table(*graph, *parser_tables,
               arguments);
    } else if (macro == "XDP2_MAKE_TLV_TABLE") {
      xdp2gen::handle_make_table(*graph, *tlv_tables,
                arguments);
    } else if (macro == "XDP2_MAKE_FLAG_FIELDS_TABLE") {
      xdp2gen::handle_make_table(*graph, *flag_fields_tables,
                arguments);
    } else if (macro == "XDP2_MAKE_TLV_PARSE_NODE") {
      xdp2gen::handle_make_tlv_node(*tlv_nodes, arguments);
    } else if (macro == "XDP2_MAKE_FLAG_FIELD_PARSE_NODE") {
      xdp2gen::handle_make_flag_field_node(*flag_fields_nodes, arguments);
    } else if (macro == "XDP2_MAKE_LEAF_PARSE_NODE") {
      xdp2gen::handle_make_leaf_node(*graph, arguments);
    } else if (macro == "XDP2_MAKE_LEAF_TLVS_PARSE_NODE") {
      xdp2gen::handle_make_leaf_tlv_node(*graph, arguments);
    } else if (macro == "XDP2_MAKE_FLAG_FIELDS_PARSE_NODE") {
      xdp2gen::handle_make_flag_fields_node(*graph, arguments);
    } else if (macro == "XDP2_MAKE_FLAG_FIELDS_OVERLAY_PARSE_NODE") {
      xdp2gen::handle_make_flag_fields_overlay_node(*graph, arguments);
    } else if (macro == "XDP2_MAKE_LEAF_FLAG_FIELDS_PARSE_NODE") {
      xdp2gen::handle_make_leaf_flag_fields_node(*graph, arguments);
    } else if (macro_name.get_value() == "XDP2_MAKE_PARSE_NODE") {
      xdp2gen::handle_make_node(*graph, arguments);
    } else if (macro_name.get_value() == "XDP2_MAKE_OVERLAY_PARSE_NODE") {
      xdp2gen::handle_make_overlay_node(*graph, arguments);
    } else if (macro_name.get_value() == "XDP2_PARSER_ADD") {
      xdp2gen::handle_parser_add(*graph, *roots, arguments);
    } else if (macro_name.get_value() == "XDP2_PARSER_EXT") {
      xdp2gen::handle_parser_ext(*graph, *roots, arguments);
    } else if (macro_name.get_value() == "XDP2_PARSER") {
      xdp2gen::handle_parser(*graph, *roots, arguments);
    } else if (macro_name.get_value() == "XDP2_PARSER_XDP") {
      xdp2gen::handle_parser_xdp(*graph, *roots, arguments);
    } else if (macro_name.get_value() == "XDP2_MAKE_TLV_OVERLAY_PARSE_NODE") {
      xdp2gen::handle_make_tlv_overlay_node(*tlv_nodes, arguments);
	}
    return true;
  }

  template <typename TokenContainer> static std::vector<std::string>
  string_tokens (const TokenContainer &tokens)
  {
    auto strings = std::vector<std::string>{};

    for (const auto &token : tokens) {
      auto str = to_std_string(token.get_value());

      if (is_whitespace(str))
        continue;

      strings.push_back(str);
    }

    return strings;
  }

  template <typename ContainerT> void
  table_from_macro_args(vertex_descriptor source,
            std::vector<ContainerT> const &arguments)
  {
  }
};

template <typename Context> auto
add_xdp2_macros (Context &context)
{
  auto macros = std::vector<std::string>{
    "XDP2_DECL_PARSE_NODE(node)",
    "XDP2_DECL_TLVS_PARSE_NODE(node)",
    "XDP2_MAKE_PROTO_TABLE(table_name, ...)",
    "XDP2_MAKE_PARSER_TABLE(table_name, ...)", //TODO: add interpretation
    "XDP2_MAKE_TLV_TABLE(table_name, ...)",
    "XDP2_MAKE_FLAG_FIELDS_TABLE(table_name, ...)",
    "XDP2_MAKE_TLV_PARSE_NODE(node, proto_tlv_node, metadata, handler)",
    "XDP2_MAKE_FLAG_FIELD_PARSE_NODE(node, name, metadata)",
    "XDP2_MAKE_PARSE_NODE(node, name, metadata, pointer, table)",
    "XDP2_MAKE_OVERLAY_PARSE_NODE(node, name, metadata, handler, overlay_node)",
    "XDP2_MAKE_TLVS_PARSE_NODE(node, name, metadata, pointer, "
    "table)",
    "XDP2_MAKE_FLAG_FIELDS_PARSE_NODE(node, name, metadata, pointer, table, flag_fields_table)",
    "XDP2_MAKE_FLAG_FIELDS_OVERLAY_PARSE_NODE(node, name, metadata, pointer, overlay_node, flag_fields_table)",
    "XDP2_MAKE_LEAF_FLAG_FIELDS_PARSE_NODE(node, name, metadata, pointer, flag_fields_table, post_flag_handle)",
    "XDP2_MAKE_LEAF_PARSE_NODE(node, name, metadata, pointer)",
    "XDP2_MAKE_LEAF_TLVS_PARSE_NODE(node, name, metadata, "
    "pointer, table)",
    "XDP2_PARSER_ADD(name, description, node_addr)",
    "XDP2_PARSER_EXT(parser, description, node_addr)",
    "XDP2_PARSER(parser, description, node_addr)",
    "XDP2_PARSER_XDP(parser, description, node_addr)",
	"XDP2_MAKE_TLV_OVERLAY_PARSE_NODE(node_name, "
	"metadata_func, handler_func, "
	"overlay_table, overlay_type_func, "
	"unknown_overlay_ret, overlay_wildcard_node)",
  };

  for (const auto &macro : macros)
    context.add_macro_definition (macro, true);
}

template <typename G> void
parse_file(G &g, std::vector<std::tuple<std::string,
     typename boost::graph_traits<G>::vertex_descriptor, bool, bool>> &roots,
     std::string filename)
{
  // save current file position for exception handling
  using position_type = boost::wave::util::file_position_type;
  position_type current_position;

  try {
    using lex_iterator_type = boost::wave::cpplexer::
        lex_iterator<boost::wave::cpplexer::
        lex_token<> >;
    using input_policy = boost::wave::iteration_context_policies::
        load_file_to_string;
    using context_policy = MacroOnly<G>;

                using context_type = boost::wave::context<std::string::iterator,
      lex_iterator_type, input_policy, context_policy>;

    auto file = std::ifstream(filename);

                auto input = std::string(std::istreambuf_iterator<char>
          (file.rdbuf()),
          std::istreambuf_iterator<char>());

    std::vector<table> parser_tables;
    std::vector<tlv_node> tlv_nodes;
    std::vector<flag_fields_node> flag_fields_nodes;
    std::vector<table> tlv_tables;
    std::vector<table> flag_fields_tables;
    context_type context(input.begin(), input.end(),
             filename.c_str (),
             MacroOnly<G>{g, parser_tables, tlv_tables, flag_fields_tables,
               tlv_nodes, flag_fields_nodes, roots});

    add_xdp2_macros(context);
    for (const auto &it : context)
      current_position = it.get_position();

    std::cout << "proto tables size: " << parser_tables.size () <<
      " tlv tables size " << tlv_tables.size () <<
      " tlv nodes " << tlv_nodes.size () <<
      " flag fields tables size " << flag_fields_tables.size () <<
      " flag fields nodes " << flag_fields_nodes.size () << std::endl;

    xdp2gen::connect_vertices (g, parser_tables);
    xdp2gen::fill_tlv_overlay_to_tlv_node (tlv_nodes, tlv_tables);
    xdp2gen::fill_tlv_node_to_vertices (g, tlv_nodes, tlv_tables);
    xdp2gen::fill_flag_fields_node_to_vertices (g, flag_fields_nodes, flag_fields_tables);
  }

  // preprocessing error
  catch (boost::wave::cpp_exception const &e) {
                std::cerr << e.file_name () << "(" << e.line_no () <<
      ") preprocessing error: " << e.description () <<
      std::endl;
  }
  // use last recognized token to retrieve the error position
  catch (std::exception const &e) {
    std::cerr << current_position.get_file () << "(" <<
      current_position.get_line () << "): " <<
      "exception caught: " << e.what () << std::endl;
  }
  // use last recognized token to retrieve the error position
  catch (...) {
    std::cerr << current_position.get_file () << "(" <<
      current_position.get_line () << "): " <<
      "unexpected exception caught." << std::endl;
  }
}

} // namespace xdp2gen

int main (int argc, char *argv[])
{
  if (argc != 2 && argc != 3) {
    std::cout << "Usage: " << argv[0] << " <source> [OUTPUT]\n"
        << "\n"
           "Where if OUTPUT is provided:\n"
           "  - If OUTPUT extension is .c, "
           "generates C code\n"
           "  - If OUTPUT extension is .xdp, "
           "generates XDP BPF-C code\n"
           "  - If OUTPUT extension is .dot, "
           "generates graphviz dot file\n";
    return 1;
  }

  xdp2gen::graph_t graph;

  std::vector<xdp2gen::root_t> roots;
  std::string filename = argv[1];
  xdp2gen::parse_file(graph, roots, filename);

  {
    auto vs = vertices (graph);
    std::cout << "Finished parsing file. " <<
      std::distance (vs.first, vs.second) << " vertices\n";
  }

  if (!roots.empty()) {
    auto back_edges = xdp2gen::back_edges(graph, get<1>(roots[0]));

    for (auto &&edge : back_edges) {
      auto u = source(edge, graph);
      auto v = target(edge, graph);

      std::cout << "  [" << graph[u].name << ", " <<
        graph[v].name << "]\n";
    }

    std::cout << "Has cycle? -> " <<
        (back_edges.empty () ? "No" : "Yes") << "\n";

    if (argc == 3) {
      auto output = std::string{ argv[2] };

      if (output.substr(std::max(output.size() - 4,
                                 0ul)) == ".dot") {
        std::cout << "Generating dot file...\n";
        xdp2gen::dotify(graph, output,
             get<1>(roots[0]), back_edges);
      } else if (output.substr(std::max(output.size() - 7,
                               0ul)) == ".kmod.c") {
         try {
        auto file = std::ofstream { output };
        auto out = std::ostream_iterator<char>(file);

		auto res = xdp2gen::python::generate_root_parser_kmod_c(
              filename,
              output,
              graph,
              roots
            );
		if (res != 0) {
			std::cout << "failed python gen?" << std::endl;
			return res;
		}

        } catch (std::exception const& e) {
          std::cerr << "Failed to generate " << output << ": " << e.what() << "\n";
          return 1;
        }
      } else if (output.substr(std::max(output.size() - 2,
             0ul)) == ".c") {
        try {
            auto res = xdp2gen::python::generate_root_parser_c(
              filename,
              output,
              graph,
              roots
            );
            if (res != 0) {
				std::cout << "failed python gen?" << std::endl;
              return res;
            }
        } catch (std::exception const& e) {
          std::cerr << "Failed to generate " << output << ": " << e.what() << "\n";
          return 1;
        }
      } else if (output.substr(std::max(output.size() - 6,
                               0ul)) == ".xdp.h") {
        auto file = std::ofstream { output };
        auto out = std::ostream_iterator<char>(file);
        if (roots.size() > 1) {
           std::cout << "XDP only supports one root";
           return 1;
        }

		auto res = xdp2gen::python::generate_root_parser_xdp_c(
              filename,																
              output,
              graph,
              roots
            );
		if (res != 0) {
			std::cout << "failed python gen?" << std::endl;
			return res;
		}
      } else {
        std::cout << "Unknown file extension in "
               "filename " << output << ".\n";
        return 1;
      }
      std::cout << "Done\n";
    } else {
      std::cout << "Nothing to generate\n";
    }
  } else {
    std::cout << "No roots in this parser, use XDP2_PARSER_ADD, "
           "XDP2_PARSER[_EXT], or XDP2_PARSER_XDP" <<
           std::endl;
  }
}
