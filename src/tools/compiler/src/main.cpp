// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2022 SiXDP2 Inc.
 *
 * Authors: Felipe Magno de Almeida <felipe@expertise.dev>
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
#include <functional>

#include <arpa/inet.h>

// Boost
#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>

// XDP2
#include "xdp2gen/graph.h"
#include "xdp2gen/python_generators.h"
#include "xdp2gen/processing_utilities.h"
#include "xdp2gen/llvm/llvm_graph.h"
#include "xdp2gen/pattern_match.h"
#include "xdp2gen/llvm/metadata_pattern.h"
#include "xdp2gen/llvm/proto_next_pattern.h"
#include "xdp2gen/ast-consumer/graph_consumer.h"
#include "xdp2gen/ast-consumer/proto-nodes.h"
#include "xdp2gen/ast-consumer/proto-tables.h"
#include "xdp2gen/ast-consumer/metadata-type.h"
#include "xdp2gen/ast-consumer/flag-fields.h"
#include "xdp2gen/json/metadata.h"
#include "xdp2gen/program-options/compiler_options.h"
#include "xdp2gen/program-options/log_handler.h"
#include "xdp2gen/llvm/patterns.h"
#include <xdp2/parser_types.h> //somehow we can make this path better

// Clang
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CommonOptionsParser.h>

// LLVM
#include <llvm/ADT/Twine.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/MemoryBuffer.h>

#include <nlohmann/json.hpp>

//
// Macros
//

#define XDP2_STRINGIFY_A(X) #X
#define XDP2_STRINGIFY(X) XDP2_STRINGIFY_A(X)

//
// Globals
//

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory XDP2ToolsCompilerCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static llvm::cl::extrahelp
    CommonHelp(clang::tooling::CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
// static llvm::cl::extrahelp MoreHelp("\nMore help text...\n");

//
// Forward declarations
//

int extract_struct_constants(
    std::string cfile, std::string llvm_file, std::vector<const char *> args,
    xdp2gen::graph_t &graph,
    std::vector<xdp2gen::parser<xdp2gen::graph_t>> &roots,
    xdp2gen::clang_ast::metadata_record &record);

std::string xdp2_code_to_string(int code)
{
    switch (code) {
    case XDP2_OKAY:
    case XDP2_RET_OKAY:
    case XDP2_OKAY_USE_WILD:
    case XDP2_OKAY_USE_ALT_WILD:
        return { "continue" };
    case XDP2_STOP_OKAY:
        return { "stop_okay" };
    case XDP2_STOP_NODE_OKAY:
        return { "stop_node" };
    case XDP2_STOP_SUB_NODE_OKAY:
        return { "stop_sub" };
    case XDP2_STOP_FAIL:
    case XDP2_STOP_LENGTH:
    case XDP2_STOP_UNKNOWN_PROTO:
    case XDP2_STOP_ENCAP_DEPTH:
    case XDP2_STOP_UNKNOWN_TLV:
    case XDP2_STOP_TLV_LENGTH:
    case XDP2_STOP_BAD_FLAG:
    case XDP2_STOP_FAIL_CMP:
    case XDP2_STOP_LOOP_CNT:
    case XDP2_STOP_TLV_PADDING:
    case XDP2_STOP_OPTION_LIMIT:
    case XDP2_STOP_MAX_NODES:
    case XDP2_STOP_COMPARE:
    case XDP2_STOP_CNTR1:
    case XDP2_STOP_CNTR2:
    case XDP2_STOP_CNTR3:
    case XDP2_STOP_CNTR4:
    case XDP2_STOP_CNTR5:
    case XDP2_STOP_THREADS_FAIL:
        return { "stop_fail" };
        ;
    }

    return { "" };
}

llvm::Expected<clang::tooling::CommonOptionsParser>
create_options_parser(std::string cfile, std::vector<const char *> args)
{
    // args.push_back(cfile.c_str());
    int argc = args.size();

    plog::log(std::cout) << "args size " << argc << std::endl;

    plog::log(std::cout) << "Compiler args" << std::endl;
    for (auto &&item : args)
        plog::log(std::cout) << std::string(item) << "\n";
    plog::log(std::cout) << std::endl;

    llvm::Expected<clang::tooling::CommonOptionsParser> OptionsParser =
        clang::tooling::CommonOptionsParser::create(argc, &args[0],
                                                    XDP2ToolsCompilerCategory);

    plog::log(std::cout) << "OptionsParser->getSourcePathList()" << std::endl;
    if (auto err = OptionsParser.takeError()) {
        llvm::errs() << "OptionsParser Error " << err;
        std::exit(1);
    }
    for (auto &&item : OptionsParser->getSourcePathList())
        plog::log(std::cout) << std::string(item) << "\n";
    plog::log(std::cout) << std::endl;

    plog::log(std::cout)
        << "OptionsParser->getCompilations().getAllFiles()" << std::endl;
    for (auto &&item : OptionsParser->getCompilations().getAllFiles())
        plog::log(std::cout) << std::string(item) << "\n";
    plog::log(std::cout) << std::endl;

    return OptionsParser;
};

clang::tooling::ClangTool create_clang_tool(
                                            llvm::Expected<clang::tooling::CommonOptionsParser> &OptionsParser,
                                            std::optional<std::string> resource_path)
{
    std::string version = XDP2_STRINGIFY(XDP2_CLANG_VERSION);
    version = version.substr(0, version.find("git"));
    plog::log(std::cout)
        << "/usr/lib/clang/" << version << "/include" << std::endl;
    if (getenv("XDP2_C_INCLUDE_PATH"))
        setenv("C_INCLUDE_PATH", getenv("XDP2_C_INCLUDE_PATH"), 1);

    plog::log(std::cout) << "OptionsParser->getSourcePathList()" << std::endl;
    for (auto &&item : OptionsParser->getSourcePathList())
        plog::log(std::cout) << std::string(item) << "\n";
    plog::log(std::cout) << std::endl;

    plog::log(std::cout)
        << "OptionsParser->getCompilations().getAllFiles()" << std::endl;
    for (auto &&item : OptionsParser->getCompilations().getAllFiles())
        plog::log(std::cout) << std::string(item) << "\n";
    plog::log(std::cout) << std::endl;

    clang::tooling::ClangTool Tool(
        OptionsParser->getCompilations(), OptionsParser->getSourcePathList(),
        std::make_shared<clang::PCHContainerOperations>());
    if (resource_path) {
      plog::log(std::cout) << "Resource dir : " << *resource_path << std::endl;
      Tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster(
         {"-resource-dir", *resource_path},
         clang::tooling::ArgumentInsertPosition::BEGIN));
    }
#ifdef XDP2_CLANG_RESOURCE_PATH
    else {
      Tool.appendArgumentsAdjuster(clang::tooling::getInsertArgumentAdjuster(
         {"-resource-dir", XDP2_STRINGIFY(XDP2_CLANG_RESOURCE_PATH)},
         clang::tooling::ArgumentInsertPosition::BEGIN));
    }
#endif

    return Tool;
};

void validate_json_metadata_ents_type(const nlohmann::ordered_json &ents)
{
    for (auto elm : ents) {
        if (elm.contains("type") && elm.contains("length")) {
            auto type = elm["type"].get<std::string>();
            auto length = elm["length"].get<std::size_t>();
            if (type == "hdr_length" && length != 2) {
                plog::warning(std::cerr)
                    << "<Warning> - hdr_length type should have a size of 2 bytes"
                    << std::endl;
                plog::warning(std::cerr) << elm << std::endl;
            } else if (type == "offset" && length != 2) {
                plog::warning(std::cerr)
                    << "<Warning> - offset type should have a size of 2 bytes"
                    << std::endl;
                plog::warning(std::cerr) << elm << std::endl;
            }
        }
    }
}

template <typename G>
void parse_file(G &g, std::vector<xdp2gen::parser<G>> &roots,
                clang::tooling::ClangTool &Tool)
{
    // Extract basic graph information
    graph_info graph_consumed_data{ &g, &roots };

    clang::IgnoringDiagConsumer diagConsumer;
    Tool.setDiagnosticConsumer(&diagConsumer);

    int action1 =
        Tool.run(extract_graph_constants_factory(graph_consumed_data).get());

    plog::log(std::cout) << "========= TLVS NODES =========" << std::endl;
    for (auto item : graph_consumed_data.graph->vertex_set())
        if (!(*graph_consumed_data.graph)[item].tlv_table.empty())
            plog::log(std::cout)
                << (*graph_consumed_data.graph)[item] << std::endl;
    plog::log(std::cout) << "=========================" << std::endl;

    plog::log(std::cout) << "========= TLVS =========" << std::endl;
    for (auto &item : graph_consumed_data.tlv_nodes)
        plog::log(std::cout) << item << std::endl;
    plog::log(std::cout) << "=========================" << std::endl;

    plog::log(std::cout) << "========= TLVS TABLES =========" << std::endl;
    for (auto &item : graph_consumed_data.tlv_tables)
        plog::log(std::cout) << item << std::endl;
    plog::log(std::cout) << "=========================" << std::endl;

    plog::log(std::cout)
        << "FINAL GRAPH SIZE - " << g.vertex_set().size() << std::endl;

    plog::log(std::cout) << "TLV_NODES SIZE - "
                         << graph_consumed_data.tlv_nodes.size() << std::endl;

    plog::log(std::cout)
        << "FLAG_FIELD_NODES SIZE - "
        << graph_consumed_data.flag_fields_nodes.size() << std::endl;

    xdp2gen::fill_tlv_wildcard_nodes(g, graph_consumed_data.tlv_nodes);
    xdp2gen::fill_tlv_overlay_to_tlv_node(graph_consumed_data.tlv_nodes,
                                           graph_consumed_data.tlv_tables);
    xdp2gen::fill_tlv_node_to_vertices(g, graph_consumed_data.tlv_nodes,
                                        graph_consumed_data.tlv_tables);
    xdp2gen::fill_flag_fields_node_to_vertices(
        g, graph_consumed_data.flag_fields_nodes,
        graph_consumed_data.flag_fields_tables);

    plog::log(std::cout) << "========= ROOTS =========" << std::endl;
    for (auto &item : roots)
        plog::log(std::cout) << item << std::endl;
    plog::log(std::cout) << "=========================" << std::endl;

    // exit(1);
}

//
// Main
//

int main(int argc, char *argv[])
{
    xdp2gen::xdp2_input_flag_handler xdp2gen_input_handler(argc, argv);

    bool is_compilation_verbose =
        xdp2gen_input_handler.verify_flag_existence("verbose");

    plog::set_display_log(is_compilation_verbose);

    bool are_warnings_disabled =
        xdp2gen_input_handler.verify_flag_existence("disable-warnings");

    plog::set_display_warning(!are_warnings_disabled);

    if (!xdp2gen_input_handler.is_in_well_formed_state()) {
        std::cout << xdp2gen_input_handler;

        return 1;
    }

    xdp2gen::graph_t graph;
    std::vector<xdp2gen::parser<xdp2gen::graph_t>> roots;

    auto filename =
        xdp2gen_input_handler.get_flag_value<std::string>("input").value();

    auto include_paths =
        xdp2gen_input_handler.get_flag_value<std::vector<std::string>>(
            "include");

    std::vector<const char *> compiler_args;

    compiler_args.push_back(argv[0]);

    std::string temp("--");

    std::vector<std::string> include_args;

    std::stringstream ss;

    if (include_paths.has_value()) {
        ss << "--extra-arg=-I";

        for (auto &item : include_paths.value()) {
            plog::log(std::cout) << "item " << item << std::endl;
            ss << "" << item << " ";
        }

        std::size_t len = ss.str().length();

        char *cstr = new char[len + 1];
        std::strcpy(cstr, ss.str().c_str());

        if (cstr[len - 1] == ' ')
            cstr[len - 1] = '\0';

        compiler_args.push_back(cstr);
    }

    compiler_args.push_back(filename.c_str());
    compiler_args.push_back(temp.c_str());

    plog::log(std::cout) << "Compiler args" << std::endl;
    for (auto &&item : compiler_args)
      plog::log(std::cout) << std::string(item) << "\n";
    plog::log(std::cout) << std::endl;

    std::optional<std::string> resource_path;
    if (xdp2gen_input_handler.verify_flag_existence("resource-path"))
      resource_path = xdp2gen_input_handler.get_flag_value<std::string>("resource-path").value();

    auto OptionsParser = create_options_parser(filename, compiler_args);
    if (OptionsParser) {
        auto Tool = create_clang_tool(OptionsParser, resource_path);
        parse_file(graph, roots, Tool);
    }

    xdp2gen::clang_ast::metadata_record record;

    std::string llvm_filename;

    xdp2gen_input_handler.get_flag_value("ll", llvm_filename);
    if (extract_struct_constants(filename, llvm_filename, compiler_args, graph,
                                 roots, record) != 0)
        return -1;

    {
        auto [first, last] = vertices(graph);

        plog::log(std::cout)
            << "Finished parsing file. " << std::distance(first, last)
            << " vertices" << std::endl;
    }

    if (!roots.empty()) {
        auto back_edges = xdp2gen::back_edges(graph, roots[0].root);

        for (auto &&edge : back_edges) {
            auto u = source(edge, graph);
            auto v = target(edge, graph);

            plog::log(std::cout) << "  [" << graph[u].name << ", "
                                 << graph[v].name << "]" << std::endl;
        }

        plog::log(std::cout)
            << "Has cycle? -> " << (back_edges.empty() ? "No" : "Yes")
            << std::endl;

        if (xdp2gen_input_handler.is_in_well_formed_state()) {
            std::string output;
            xdp2gen_input_handler.get_flag_value("output", output);
            std::string output_basename;

            plog::log(std::cout) << "output name " << output << std::endl;

            if (output.substr(std::max(output.size() - 4, 0ul)) == ".dot") {
                output_basename =
                    output.substr(std::max(output.size() - 4, 0ul));
                plog::log(std::cout) << "Generating dot file..." << std::endl;
                xdp2gen::dotify(graph, output, roots[0].root, back_edges);
            } else if (output.substr(std::max(output.size() - 2, 0ul)) ==
                       ".c") {
                output_basename =
                    output.substr(std::max(output.size() - 2, 0ul));
                try {
                    auto res = xdp2gen::python::generate_root_parser_c(
                        filename, output, graph, roots, record);

                    if (res != 0) {
                        plog::log(std::cout)
                            << "failed python gen?" << std::endl;
                        return res;
                    }
                } catch (const std::exception &e) {
                    plog::log(std::cerr) << "Failed to generate " << output
                                         << ": " << e.what() << std::endl;
                    return 1;
                }
            } else if (output.substr(std::max(output.size() - 6, 0ul)) ==
                       ".xdp.h") {
                output_basename =
                    output.substr(std::max(output.size() - 6, 0ul));
                auto file = std::ofstream{ output };
                auto out = std::ostream_iterator<char>(file);

                if (roots.size() > 1) {
                    plog::log(std::cout) << "XDP only supports one root";
                    return 1;
                }

                auto res = xdp2gen::python::generate_root_parser_xdp_c(
                    filename, output, graph, roots, record);

                if (res != 0) {
                    plog::log(std::cout) << "failed python gen?" << std::endl;
                    return res;
                }
            } else if (output.substr(std::max(output.size() - 3, 0ul)) ==
                       ".p4") {
                output_basename =
                    output.substr(std::max(output.size() - 6, 0ul));
                auto file = std::ofstream{ output };
                auto out = std::ostream_iterator<char>(file);

                if (roots.size() > 1) {
                    plog::log(std::cout) << "P4 only supports one root";
                    return 1;
                }

                auto res = xdp2gen::python::generate_root_parser_p4(
                    filename, output, graph, roots, record);

                if (res != 0) {
                    plog::log(std::cout) << "failed python gen?" << std::endl;
                    return res;
                }
            } else if (output.substr(std::max(output.size() - 5, 0ul)) ==
                       ".json") {
                plog::log(std::cout) << "Generating json" << std::endl;
                // Creates the json
                nlohmann::ordered_json parser_json;

                nlohmann::ordered_json parser_json_array =
                    nlohmann::json::array();

                for (auto &root : roots) {
                    nlohmann::ordered_json parser_json_item{

                        { "file_name", filename },
                        { "name", root.parser_name },
                        { "root-node", graph[root.root].name },
                        { "metameta-size", root.metameta_size },
                        { "max-nodes", root.max_nodes },
                        { "max-encaps", root.max_encaps },
                        { "max-frames", root.max_frames },
                        { "frame-size", root.frame_size }

                    };

                    if (root.okay_target_set)
                        parser_json_item["okay-target"] =
                            graph[root.okay_target].name;

                    if (root.fail_target_set)
                        parser_json_item["fail-target"] =
                            graph[root.fail_target].name;

                    if (root.encap_target_set)
                        parser_json_item["encap-target"] =
                            graph[root.encap_target].name;

                    parser_json_array.push_back(parser_json_item);
                }

                parser_json["parsers"] = parser_json_array;

                if (!record.fields.empty()) {
                    nlohmann::ordered_json metadata = nlohmann::json::array();

                    metadata.push_back(xdp2gen::json::to_json(record));

                    parser_json.push_back({ "metadata", metadata });
                }

                nlohmann::ordered_json parse_nodes_root =
                    nlohmann::json::array();

                nlohmann::ordered_json tlv_nodes_root = nlohmann::json::array();

                xdp2gen::tlv_node::tlv_node_ref_vec tlv_nodes_list;

                xdp2gen::tlv_node::unordered_tlv_node_ref_set tlv_nodes_set;

                xdp2gen::flag_fields_node::flag_fields_ref_vec flag_fields_list;

                std::unordered_set<
                    xdp2gen::llvm::package_metadata_counter,
                    xdp2gen::llvm::package_metadata_counter::pmc_hash,
                    xdp2gen::llvm::package_metadata_counter::pmc_hash>
                    counter_collection;

                bool has_tlvs = false;

                auto is_mask_full = [](unsigned int mask,
                                       unsigned int length_in_bits) -> bool {
                    bool result = true;

                    for (size_t i = 0; i < length_in_bits; ++i) {
                        if (!(mask & 0x1)) {
                            result = false;
                            break;
                        }

                        mask = (mask >> 1);
                    }

                    return result;
                };

                auto reverse_bytes = [](void *num, size_t byte_size) {
                    std::reverse(static_cast<char *>(num),
                                 static_cast<char *>(num) + byte_size);
                };

                auto correct_name = [](std::string &name) -> std::string {
                    auto pos = name.find(".");

                    return name.substr(0, pos);
                };

                nlohmann::ordered_json len_data_object;
                nlohmann::ordered_json metadata_object;
                nlohmann::ordered_json next_proto_object;
                nlohmann::ordered_json cond_exprs;
                nlohmann::ordered_json tlv_parse_node;
                nlohmann::ordered_json flag_fields_parse_node;
                nlohmann::ordered_json counter_actions =
                    nlohmann::json::array();

                /// lets generate the json too
                for (auto vd : boost::make_iterator_range(vertices(graph))) {
                    auto &&node = graph[vd];

                    auto to_hex_mask = [](unsigned int mask) -> std::string {
                        std::ostringstream ss;
                        ss << "0x" << std::hex << mask;
                        return ss.str();
                    };

                    auto to_hex_mask_trunc =
                        [](unsigned int mask,
                           unsigned int mask_size) -> std::string {
                        unsigned int mask_mask = 0;

                        if (mask_size > 0) {
                            unsigned int num_bits = mask_size * CHAR_BIT;

                            for (std::size_t i = 0; i < num_bits; ++i)
                                mask_mask = (mask_mask << 1) + 1;
                        }

                        unsigned int print_mask = (mask & mask_mask);

                        std::ostringstream ss;
                        ss << "0x" << std::hex << print_mask;
                        return ss.str();
                    };

                    auto to_lower_case =
                        [](std::string const &str) -> std::string {
                        std::string result = str;

                        std::transform(result.begin(), result.end(),
                                       result.begin(), ::tolower);

                        return result;
                    };

                    // Converts proto node mask to hex string

                    // Body of parse_node
                    auto parse_node_object =
                        nlohmann::ordered_json{ { "name", node.name } };

                    if (!node.handler.empty() &&
                        to_lower_case(node.handler) != "null") {
                        parse_node_object["handler"]["name"] = node.handler;
                        if (node.handler_watchers.has_value())
                            parse_node_object["handler"]["watchers"] =
                                to_hex_mask(node.handler_watchers.value());
                        if (node.handler_blockers.has_value())
                            parse_node_object["handler"]["blockers"] =
                                to_hex_mask(node.handler_blockers.value());
                    }
                    // If there is a valid min_len, inserts the value in parse_node
                    if (node.proto_min_len.has_value())
                        parse_node_object["min-hdr-length"] =
                            node.proto_min_len.value();

                    if (node.overlay.has_value())
                        parse_node_object["overlay"] = node.overlay.value();

                    if (node.encap.has_value())
                        parse_node_object["encap"] = node.encap.value();

                    if (node.next_proto_data && !node.table.empty()) {
                        next_proto_object["field-off"] =
                            node.next_proto_data->bit_offset / CHAR_BIT;
                        next_proto_object["field-len"] =
                            node.next_proto_data->bit_size / CHAR_BIT;

                        if (!is_mask_full(node.next_proto_data->bit_mask,
                                          node.next_proto_data->bit_size)) {
                            next_proto_object["mask"] =
                                to_hex_mask(node.next_proto_data->bit_mask);
                        }
                        if (node.next_proto_data->right_shift != 0) {
                            next_proto_object["right-shift"] =
                                node.next_proto_data->right_shift;
                        }

                        if (!node.table.empty())
                            next_proto_object["table"] = node.table;

                        if (!node.wildcard_proto_node.empty()) {
                            next_proto_object["wildcard-node"] =
                                node.wildcard_proto_node;
                            next_proto_object["default"] = "wild";
                        }
                        if (node.unknown_ret.has_value()) {
                            next_proto_object["default"] =
                                xdp2_code_to_string(node.unknown_ret.value());
                        }
                        if (node.next_proto_data->endian_swap)
                            next_proto_object["endian-swap"] =
                                node.next_proto_data->endian_swap;
                    }

                    if (node.len_data) {
                        len_data_object["field-off"] =
                            node.len_data->bit_offset / CHAR_BIT;
                        len_data_object["field-len"] =
                            node.len_data->bit_size / CHAR_BIT;

                        if (!is_mask_full(node.len_data->bit_mask,
                                          node.len_data->bit_size)) {
                            len_data_object["mask"] =
                                to_hex_mask(node.len_data->bit_mask);
                        }
                        if (node.len_data->right_shift != 0) {
                            len_data_object["right-shift"] =
                                node.len_data->right_shift;
                        }

                        if (node.len_data->multiplier > 1)
                            len_data_object["multiplier"] =
                                node.len_data->multiplier;
                    }

                    if (!node.flag_fields_nodes.empty()) {
                        len_data_object["flag-fields-length"] = true;
                    }

                    if (node.cond_exprs) {
                        auto lhs =
                            std::get<xdp2gen::llvm::
                                         packet_buffer_offset_masked_multiplied>(
                                node.cond_exprs->lhs);
                        auto rhs = std::get<xdp2gen::llvm::constant_value>(
                            node.cond_exprs->rhs);

                        nlohmann::ordered_json ents = nlohmann::json::array();

                        nlohmann::ordered_json cond_obj;

                        cond_obj["type"] =
                            xdp2gen::llvm::comparison_op_to_string(
                                node.cond_exprs->comparison_op);
                        cond_obj["field-off"] = lhs.bit_offset / CHAR_BIT;
                        cond_obj["field-len"] = lhs.bit_size / CHAR_BIT;

                        if (!is_mask_full(lhs.bit_mask, lhs.bit_size)) {
                            cond_obj["mask"] = to_hex_mask(lhs.bit_mask);
                        }

                        std::size_t inverted_val = rhs.value;

                        if (lhs.bit_size <= 8)
                            ;
                        else if (lhs.bit_size <= 16)
                            inverted_val = htons(inverted_val);
                        else if (lhs.bit_size <= 32)
                            inverted_val = htonl(inverted_val);

                        cond_obj["value"] = inverted_val;

                        ents.push_back(cond_obj);
                        cond_exprs["default-fail"] = xdp2_code_to_string(
                            node.cond_exprs->default_fail.value);
                        cond_exprs["ents"] = ents;
                    }

                    if (node.tlv_type.has_value()) {
                        auto tlv_type_length =
                            node.tlv_type_val.has_value() ?
                                node.tlv_type_val.value().bit_size / CHAR_BIT :
                                1;
                        auto tlv_len_length =
                            node.tlv_len_val.has_value() ?
                                node.tlv_len_val.value().bit_size / CHAR_BIT :
                                1;

                        auto has_pad1 = (node.pad1_enable.has_value() ?
                                             node.pad1_enable.value() :
                                             false);
                        auto has_padn = (node.padn_enable.has_value() ?
                                             node.padn_enable.value() :
                                             false);
                        auto has_eol = (node.eol_enable.has_value() ?
                                            node.eol_enable.value() :
                                            false);

                        auto pad1_val = node.pad1_val.has_value() ?
                                            node.pad1_val.value() :
                                            0;
                        auto padn_val = node.padn_val.has_value() ?
                                            node.padn_val.value() :
                                            0;
                        auto eol_val =
                            node.eol_val.has_value() ? node.eol_val.value() : 0;

                        auto start_offset_val =
                            (node.tlv_start_offset_val.has_value() ?
                                 node.tlv_start_offset_val.value().value :
                                 20);

                        if (has_pad1)
                            tlv_parse_node["pad1"] = pad1_val;

                        if (has_padn)
                            tlv_parse_node["padn"] = padn_val;

                        if (has_eol)
                            tlv_parse_node["eol"] = eol_val;

                        if (node.max_plen.has_value())
                            tlv_parse_node["max-padding-length"] =
                                node.max_plen.value();

                        if (node.max_c_pad.has_value())
                            tlv_parse_node["max-consecutive-padding"] =
                                node.max_c_pad.value();

                        if (node.exceed_loop_cnt_is_err.has_value())
                            tlv_parse_node["loop-count-exceeded-is-err"] =
                                (node.exceed_loop_cnt_is_err.value() != 0 ?
                                     true :
                                     false);

                        if (node.disp_limit_exceed.has_value() &&
                            node.disp_limit_exceed.value() != XDP2_OKAY &&
                            node.exceed_loop_cnt_is_err.value() != 0)
                            tlv_parse_node["disp-limit-exceeded"] =
                                xdp2_code_to_string(
                                    node.disp_limit_exceed.value());

                        if (node.max_non.has_value())
                            tlv_parse_node["max-non-padding"] =
                                node.max_non.value();

                        if (node.max_loop.has_value())
                            tlv_parse_node["max-tlvs"] = node.max_loop.value();

                        tlv_parse_node["tlv-type"]["field-off"] = 0;
                        tlv_parse_node["tlv-type"]["field-len"] =
                            tlv_type_length;

                        tlv_parse_node["tlv-length"]["field-off"] =
                            node.tlv_len_val.has_value() ?
                                node.tlv_len_val.value().bit_offset / CHAR_BIT :
                                1;
                        tlv_parse_node["tlv-length"]["field-len"] =
                            tlv_len_length;

                        tlv_parse_node["start-offset"] = start_offset_val;

                        if (!node.tlv_wildcard_node.empty()) {
                            tlv_parse_node["wildcard-node"] =
                                node.tlv_wildcard_node;
                            tlv_parse_node["default"] = "wild";
                        }

                        if (!node.tlv_nodes.empty()) {
                            has_tlvs = true;

                            auto const &local_tlv_nodes_list =
                                node.get_all_tlv_node_list();

                            tlv_nodes_set.insert(local_tlv_nodes_list.begin(),
                                                 local_tlv_nodes_list.end());

                            nlohmann::json array = nlohmann::json::array();

                            for (auto &&tlv_node : node.tlv_nodes)
                                array.push_back(nlohmann::json::object(
                                    { { "key", tlv_node.key },
                                      { "node", tlv_node.name } }));

                            tlv_parse_node["ents"] = array;
                        }
                    }

                    if (!node.flag_fields_nodes.empty() && node.flags_data) {
                        flag_fields_list.insert(flag_fields_list.end(),
                                                node.flag_fields_nodes.begin(),
                                                node.flag_fields_nodes.end());

                        flag_fields_parse_node["flags-offset"] =
                            node.flags_data.value().bit_offset / CHAR_BIT;

                        if (!is_mask_full(node.flags_data.value().bit_mask,
                                          node.flags_data.value().bit_size)) {
                            flag_fields_parse_node["flags-mask"] =
                                to_hex_mask(node.flags_data.value().bit_mask);
                        }

                        flag_fields_parse_node["flags-length"] =
                            node.flags_data.value().bit_size / CHAR_BIT;

                        nlohmann::ordered_json ents = nlohmann::json::array();

                        int flag_fields_ordering = node.verify_flag_ordering();

                        if (flag_fields_ordering == 0) {
                            plog::log(std::cout)
                                << "node " << node.name
                                << " has flag fields in disorder" << std::endl;
                            return 1;
                        }

                        flag_fields_parse_node["flags-reverse-order"] =
                            flag_fields_ordering == -1 ? true : false;

                        for (auto &&flag_field : node.flag_fields_nodes) {
                            if (!flag_field.name.empty() &&
                                flag_field.name != "XDP2_FLAG_NODE_NULL") {
                                nlohmann::ordered_json flag_info;

                                ents.push_back(flag_info);

                                if (!flag_field.metadata_transfers.empty()) {
                                    nlohmann::ordered_json metadata_ents =
                                        nlohmann::json::array();

                                    for (auto &transfer_item :
                                         flag_field.metadata_transfers) {
                                        if (std::holds_alternative<
                                                xdp2gen::llvm::metadata_transfer>(
                                                transfer_item.transfer)) {
                                            auto &item =
                                                std::get<xdp2gen::llvm::
                                                             metadata_transfer>(
                                                    transfer_item.transfer);

                                            nlohmann::ordered_json tmp_json_obj =
                                                nlohmann::json::object();

                                            if (!transfer_item.name.empty())
                                                tmp_json_obj["name"] =
                                                    transfer_item.name;

                                            tmp_json_obj["type"] = "extract";
                                            tmp_json_obj["md-off"] =
                                                item.dst_bit_offset / CHAR_BIT;
                                            tmp_json_obj["is-frame"] =
                                                item.is_frame;
                                            tmp_json_obj["hdr-src-off"] =
                                                item.src_bit_offset / CHAR_BIT;
                                            tmp_json_obj["length"] =
                                                item.bit_size / CHAR_BIT;

                                            if (item.endian_swap.has_value() &&
                                                item.endian_swap.value())
                                                tmp_json_obj["endian-swap"] =
                                                    item.endian_swap.value();

                                            metadata_ents.push_back(
                                                tmp_json_obj);
                                        }
                                    }

                                    ents.back()["metadata"]["ents"] =
                                        metadata_ents;
                                }

                                plog::log(std::cout)
                                    << "flag_field.name " << flag_field.name
                                    << std::endl;

                                ents.back()["name"] = flag_field.name;

                                if (!flag_field.handler.empty())
                                    ents.back()["handler"] = flag_field.handler;

                                ents.back()["flag"] =
                                    to_hex_mask(flag_field.flag);
                                ents.back()["field-len"] =
                                    flag_field.size / CHAR_BIT;
                            }
                        }

                        flag_fields_parse_node["ents"] = ents;

                        plog::log(std::cout)
                            << "flag_fields_parse_node size "
                            << flag_fields_parse_node.size() << std::endl;
                    }

                    if (!node.metadata_transfers.empty()) {
                        std::unordered_set<
                            xdp2gen::llvm::package_metadata_counter,
                            xdp2gen::llvm::package_metadata_counter::pmc_hash,
                            xdp2gen::llvm::package_metadata_counter::pmc_hash>
                            local_counter_collection;

                        auto add_to_counter_collection =
                            [&counter_collection, &local_counter_collection](
                                xdp2gen::llvm::package_metadata_counter
                                    &pckg_metadata_counter,
                                nlohmann::ordered_json &counter_actions) {
                                size_t prev_set_size =
                                    counter_collection.size();

                                auto found_counter = counter_collection.find(
                                    pckg_metadata_counter);

                                nlohmann::ordered_json ent_action;
                                nlohmann::json ent_action_metadata =
                                    nlohmann::json::object();
                                nlohmann::json array = nlohmann::json::array();

                                if (found_counter == counter_collection.end()) {
                                    if (pckg_metadata_counter.name.empty()) {
                                        std::stringstream ss;
                                        ss << "counter_"
                                           << counter_collection.size() + 1;
                                        pckg_metadata_counter.name = ss.str();
                                    }

                                    ent_action["name"] =
                                        pckg_metadata_counter.name;
                                    ent_action["type"] =
                                        pckg_metadata_counter.type_to_string();

                                    ent_action_metadata["type"] = "counter";
                                    ent_action_metadata["md-off"] =
                                        pckg_metadata_counter.offset;
                                    ent_action_metadata["counter"] =
                                        pckg_metadata_counter.name;

                                    array.push_back(ent_action_metadata);

                                    ent_action["metadata"]["ents"] = array;

                                    counter_collection.insert(
                                        pckg_metadata_counter);
                                    local_counter_collection.insert(
                                        pckg_metadata_counter);
                                    counter_actions.push_back(ent_action);

                                } else {
                                    local_counter_collection.insert(
                                        *found_counter);

                                    if (local_counter_collection.size() !=
                                        counter_actions.size()) {
                                        ent_action["name"] =
                                            found_counter->name;
                                        ent_action["type"] =
                                            found_counter->type_to_string();

                                        ent_action_metadata["type"] = "counter";
                                        ent_action_metadata["md-off"] =
                                            found_counter->offset;
                                        ent_action_metadata["counter"] =
                                            found_counter->name;

                                        array.push_back(ent_action_metadata);

                                        ent_action["metadata"]["ents"] = array;

                                        counter_actions.push_back(ent_action);
                                    }
                                }
                            };

                        nlohmann::ordered_json array = nlohmann::json::array();

                        auto add_mask_right_shift =
                            [&is_mask_full, &to_hex_mask_trunc](
                                std::optional<xdp2gen::llvm::mask_shift> const
                                    &ms,
                                nlohmann::ordered_json &json_obj,
                                std::size_t field_len) {
                                if (ms.has_value()) {
                                    if (ms->mask_val.bit_size != 0) {
                                        if (!is_mask_full(
                                                ms->mask_val.value,
                                                ms->mask_val.bit_size)) {
                                            json_obj["mask"] = to_hex_mask_trunc(
                                                ms->mask_val.value, field_len);
                                        }
                                    }

                                    if (ms->shift_val.right != 0) {
                                        json_obj["right-shift"] =
                                            ms->shift_val.right;
                                    }
                                }
                            };

                        for (auto &&m : node.metadata_transfers) {
                            if (auto p = std::get_if<
                                    xdp2gen::llvm::metadata_transfer>(
                                    &m.transfer)) {
                                if (p->pckg_metadata_counter.has_value())
                                    add_to_counter_collection(
                                        p->pckg_metadata_counter.value(),
                                        counter_actions);

                                nlohmann::ordered_json tmp_json_obj =
                                    nlohmann::json::object();

                                if (!m.name.empty())
                                    tmp_json_obj["name"] = m.name;

                                tmp_json_obj["type"] = "extract";

                                tmp_json_obj["md-off"] =
                                    p->dst_bit_offset / CHAR_BIT;

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["struct-off"] =
                                        p->pckg_metadata_counter.value()
                                            .local_to_counter_offset /
                                        CHAR_BIT;

                                tmp_json_obj["is-frame"] = p->is_frame;
                                tmp_json_obj["hdr-src-off"] =
                                    p->src_bit_offset / CHAR_BIT;

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["length"] =
                                        p->pckg_metadata_counter.value()
                                            .field_size /
                                        CHAR_BIT;
                                else
                                    tmp_json_obj["length"] =
                                        p->bit_size / CHAR_BIT;

                                add_mask_right_shift(p->mask_shift_val,
                                                     tmp_json_obj,
                                                     p->bit_size / CHAR_BIT);

                                if (p->endian_swap.has_value() &&
                                    p->endian_swap.value())
                                    tmp_json_obj["endian-swap"] =
                                        p->endian_swap.value();

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["index"] =
                                        counter_collection
                                            .find(
                                                p->pckg_metadata_counter.value())
                                            ->name;

                                array.push_back(tmp_json_obj);
                            }

                            else if (auto p = std::get_if<
                                         xdp2gen::llvm::metadata_write_constant>(
                                         &m.transfer)) {
                                if (p->pckg_metadata_counter.has_value())
                                    add_to_counter_collection(
                                        p->pckg_metadata_counter.value(),
                                        counter_actions);

                                nlohmann::ordered_json tmp_json_obj =
                                    nlohmann::json::object();

                                if (!m.name.empty())
                                    tmp_json_obj["name"] = m.name;

                                tmp_json_obj["type"] = "constant";

                                tmp_json_obj["md-off"] =
                                    p->dst_bit_offset / CHAR_BIT;

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["struct-off"] =
                                        p->pckg_metadata_counter.value()
                                            .local_to_counter_offset /
                                        CHAR_BIT;

                                std::size_t inverted_val = p->value;

                                if (p->bit_size <= 8)
                                    ;
                                else if (p->bit_size <= 16)
                                    inverted_val = htons(p->value);
                                else if (p->bit_size <= 32)
                                    inverted_val = htonl(p->value);

                                tmp_json_obj["is-frame"] = p->is_frame;

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["length"] =
                                        p->pckg_metadata_counter.value()
                                            .field_size /
                                        CHAR_BIT;
                                else
                                    tmp_json_obj["length"] =
                                        p->bit_size / CHAR_BIT;

                                tmp_json_obj["value"] = inverted_val;

                                add_mask_right_shift(p->mask_shift_val,
                                                     tmp_json_obj,
                                                     p->bit_size / CHAR_BIT);

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["index"] =
                                        counter_collection
                                            .find(
                                                p->pckg_metadata_counter.value())
                                            ->name;

                                array.push_back(tmp_json_obj);

                            } else if (auto p = std::get_if<
                                           xdp2gen::llvm::
                                               metadata_write_header_offset>(
                                           &m.transfer)) {
                                if (p->pckg_metadata_counter.has_value())
                                    add_to_counter_collection(
                                        p->pckg_metadata_counter.value(),
                                        counter_actions);

                                nlohmann::ordered_json tmp_json_obj =
                                    nlohmann::json::object();

                                if (!m.name.empty())
                                    tmp_json_obj["name"] = m.name;

                                tmp_json_obj["type"] = "offset";

                                tmp_json_obj["md-off"] =
                                    p->dst_bit_offset / CHAR_BIT;

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["struct-off"] =
                                        p->pckg_metadata_counter.value()
                                            .local_to_counter_offset /
                                        CHAR_BIT;

                                tmp_json_obj["is-frame"] = p->is_frame;
                                tmp_json_obj["hdr-src-off"] =
                                    p->src_bit_offset / CHAR_BIT;

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["length"] =
                                        p->pckg_metadata_counter.value()
                                            .field_size /
                                        CHAR_BIT;
                                else
                                    tmp_json_obj["length"] =
                                        p->bit_size / CHAR_BIT;

                                add_mask_right_shift(p->mask_shift_val,
                                                     tmp_json_obj,
                                                     p->bit_size / CHAR_BIT);

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["index"] =
                                        counter_collection
                                            .find(
                                                p->pckg_metadata_counter.value())
                                            ->name;

                                array.push_back(tmp_json_obj);

                            } else if (auto p = std::get_if<
                                           xdp2gen::llvm::
                                               metadata_write_header_length>(
                                           &m.transfer)) {
                                if (p->pckg_metadata_counter.has_value())
                                    add_to_counter_collection(
                                        p->pckg_metadata_counter.value(),
                                        counter_actions);

                                nlohmann::ordered_json tmp_json_obj =
                                    nlohmann::json::object();

                                if (!m.name.empty())
                                    tmp_json_obj["name"] = m.name;

                                tmp_json_obj["type"] = "hdr_length";

                                tmp_json_obj["md-off"] =
                                    p->dst_bit_offset / CHAR_BIT;

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["struct-off"] =
                                        p->pckg_metadata_counter.value()
                                            .local_to_counter_offset /
                                        CHAR_BIT;

                                tmp_json_obj["is-frame"] = p->is_frame;
                                tmp_json_obj["hdr-src-off"] =
                                    p->src_bit_offset / CHAR_BIT;

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["length"] =
                                        p->pckg_metadata_counter.value()
                                            .field_size /
                                        CHAR_BIT;
                                else
                                    tmp_json_obj["length"] =
                                        p->bit_size / CHAR_BIT;

                                add_mask_right_shift(p->mask_shift_val,
                                                     tmp_json_obj,
                                                     p->bit_size / CHAR_BIT);

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["index"] =
                                        counter_collection
                                            .find(
                                                p->pckg_metadata_counter.value())
                                            ->name;

                                array.push_back(tmp_json_obj);
                            } else if (auto p = std::get_if<
                                           xdp2gen::llvm::
                                               metadata_value_transfer>(
                                           &m.transfer)) {
                                if (p->pckg_metadata_counter.has_value())
                                    add_to_counter_collection(
                                        p->pckg_metadata_counter.value(),
                                        counter_actions);

                                nlohmann::ordered_json tmp_json_obj =
                                    nlohmann::json::object();

                                if (!m.name.empty())
                                    tmp_json_obj["name"] = m.name;

                                tmp_json_obj["type"] = p->type;

                                tmp_json_obj["md-off"] =
                                    p->dst_bit_offset / CHAR_BIT;

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["struct-off"] =
                                        p->pckg_metadata_counter.value()
                                            .local_to_counter_offset /
                                        CHAR_BIT;

                                // tmp_json_obj["hdr-src-off"] = p->src_bit_offset / CHAR_BIT;
                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["length"] =
                                        p->pckg_metadata_counter.value()
                                            .field_size /
                                        CHAR_BIT;
                                else
                                    tmp_json_obj["length"] =
                                        p->bit_size / CHAR_BIT;

                                add_mask_right_shift(p->mask_shift_val,
                                                     tmp_json_obj,
                                                     p->bit_size / CHAR_BIT);

                                if (p->pckg_metadata_counter.has_value())
                                    tmp_json_obj["index"] =
                                        counter_collection
                                            .find(
                                                p->pckg_metadata_counter.value())
                                            ->name;

                                array.push_back(tmp_json_obj);
                            }
                        }

                        validate_json_metadata_ents_type(array);
                        metadata_object["ents"] = array;
                    }

                    if (!len_data_object.empty()) {
                        parse_node_object["hdr-length"] = len_data_object;
                        len_data_object.clear();
                    }
                    if (!metadata_object.empty()) {
                        parse_node_object["metadata"] = metadata_object;
                        metadata_object.clear();
                    }
                    if (!next_proto_object.empty()) {
                        parse_node_object["next-proto"] = next_proto_object;
                        next_proto_object.clear();
                    } else if (node.table.empty() &&
                               !node.wildcard_proto_node.empty()) {
                        parse_node_object["next-node"] =
                            node.wildcard_proto_node;
                    }
                    if (!cond_exprs.empty()) {
                        parse_node_object["cond-exprs"] = cond_exprs;
                        cond_exprs.clear();
                    }
                    if (!tlv_parse_node.empty()) {
                        parse_node_object["tlvs-parse-node"] = tlv_parse_node;
                        tlv_parse_node.clear();
                    }
                    if (!flag_fields_parse_node.empty()) {
                        parse_node_object["flag-fields-parse-node"] =
                            flag_fields_parse_node;
                        flag_fields_parse_node.clear();
                    }
                    if (!counter_actions.empty()) {
                        parse_node_object["counter-actions"] = counter_actions;
                        counter_actions.clear();
                    }

                    parse_nodes_root.push_back(parse_node_object);
                }

                parser_json.push_back({ "parse-nodes", parse_nodes_root });

                nlohmann::ordered_json proto_tables_root =
                    nlohmann::json::array();

                std::set<std::string> existent_proto_tables;
                for (auto vd : boost::make_iterator_range(vertices(graph))) {
                    auto &&node = graph[vd];

                    auto &&node_out_edges = out_edges(vd, graph);

                    if ((node_out_edges.first == node_out_edges.second) ||
                        (existent_proto_tables.count(node.table)))
                        continue;

                    existent_proto_tables.insert(node.table);

                    // Body of parse_table
                    auto proto_tables_object = nlohmann::ordered_json{
                        { "name", node.table },
                        { "ents", nlohmann::json::array() }
                    };

                    typename boost::graph_traits<
                        xdp2gen::graph_t>::out_edge_iterator ei,
                        ei_end;
                    for (boost::tie(ei, ei_end) = out_edges(vd, graph);
                         ei != ei_end; ++ei) {
                        auto &&out_edge_obj = graph[*ei];

                        auto &&target_node = graph[boost::target(*ei, graph)];

                        std::size_t key_value = out_edge_obj.macro_name_value;

                        if (node.next_proto_data) {
                          if (node.next_proto_data->bit_size <= 8)
                            ;
                          else if (node.next_proto_data->bit_size <= 16)
                            key_value = htons(key_value);
                          else if (node.next_proto_data->bit_size <= 32)
                            key_value = htonl(key_value);
                        }
                        // Converts proto node mask to hex string
                        auto to_hex_mask = [&key_value]() -> std::string {
                            std::ostringstream ss;
                            ss << "0x" << std::hex << key_value;
                            return ss.str();
                        };

                        nlohmann::ordered_json ent = nlohmann::ordered_json{

                            { "name", correct_name(target_node.name) },
                            { "key", to_hex_mask() },
                            { "node", correct_name(out_edge_obj.parser_node) }

                        };

                        proto_tables_object["ents"].push_back(ent);
                    }

                    proto_tables_root.push_back(proto_tables_object);
                }

                // Inserts in the json the parse_tables field
                parser_json.push_back({ "proto-tables", proto_tables_root });

                if (has_tlvs) {
                    for (auto const &tlv_node_item : tlv_nodes_set) {
                        auto tlv_node_object = nlohmann::ordered_json{
                            { "name", tlv_node_item.get().name }
                        };

                        if (!tlv_node_item.get().handler.empty())
                            tlv_node_object["handler"] =
                                tlv_node_item.get().handler;

                        if (!tlv_node_item.get().tlv_nodes.empty()) {
                            nlohmann::json ents = nlohmann::json::array();

                            for (auto &&item : tlv_node_item.get().tlv_nodes)
                                ents.push_back(nlohmann::json::object(
                                    { { "key", item.key },
                                      { "node", item.name } }));

                            if (!tlv_node_item.get().wildcard_node.empty()) {
                                tlv_node_object["overlay-node"]["wildcard-node"] =
                                    tlv_node_item.get().wildcard_node;
                                tlv_node_object["overlay-node"]["default"] =
                                    "wild";
                            }
                            auto node = tlv_node_item.get().parent_parse_node;
                            tlv_node_object["overlay-node"]["field-off"] =
                                node->tlv_len_val.has_value() ?
                                    node->tlv_len_val.value().bit_offset /
                                        CHAR_BIT :
                                    1;
                            tlv_node_object["overlay-node"]["field-len"] =
                                node->tlv_len_val.has_value() ?
                                    node->tlv_len_val.value().bit_size /
                                        CHAR_BIT :
                                    1;
                            tlv_node_object["overlay-node"]["ents"] = ents;
                        }

                        if (!tlv_node_item.get().metadata_transfers.empty()) {
                            nlohmann::ordered_json ents =
                                nlohmann::json::array();

                            for (auto &&transfer_item :
                                 tlv_node_item.get().metadata_transfers) {
                                if (std::holds_alternative<
                                        xdp2gen::llvm::metadata_transfer>(
                                        transfer_item.transfer)) {
                                    auto &item = std::get<
                                        xdp2gen::llvm::metadata_transfer>(
                                        transfer_item.transfer);

                                    nlohmann::ordered_json tmp_json_obj =
                                        nlohmann::json::object();

                                    if (!transfer_item.name.empty())
                                        tmp_json_obj["name"] =
                                            transfer_item.name;

                                    bool is_timestamp_related_field =
                                        ((((item.dst_bit_offset / CHAR_BIT) ==
                                               12 &&
                                           (item.src_bit_offset / CHAR_BIT) ==
                                               2) ||
                                          ((item.dst_bit_offset / CHAR_BIT) ==
                                               16 &&
                                           (item.src_bit_offset / CHAR_BIT) ==
                                               6)) &&
                                         (item.bit_size / CHAR_BIT) == 4);

                                    if (!item.is_frame &&
                                        is_timestamp_related_field)
                                        tmp_json_obj["type"] = "timestamp";

                                    else
                                        tmp_json_obj["type"] = "extract";

                                    tmp_json_obj["md-off"] =
                                        item.dst_bit_offset / CHAR_BIT;
                                    tmp_json_obj["is-frame"] = item.is_frame;
                                    tmp_json_obj["hdr-src-off"] =
                                        item.src_bit_offset / CHAR_BIT;
                                    tmp_json_obj["length"] =
                                        item.bit_size / CHAR_BIT;

                                    if (item.endian_swap.has_value() &&
                                        item.endian_swap.value())
                                        tmp_json_obj["endian-swap"] =
                                            item.endian_swap.value();

                                    ents.push_back(tmp_json_obj);
                                } else if (std::holds_alternative<
                                               xdp2gen::llvm::
                                                   metadata_write_constant>(
                                               transfer_item.transfer)) {
                                    auto &item = std::get<
                                        xdp2gen::llvm::metadata_write_constant>(
                                        transfer_item.transfer);

                                    nlohmann::ordered_json tmp_json_obj =
                                        nlohmann::json::object();

                                    if (!transfer_item.name.empty())
                                        tmp_json_obj["name"] =
                                            transfer_item.name;

                                    tmp_json_obj["type"] = "constant";
                                    tmp_json_obj["md-off"] =
                                        item.dst_bit_offset / CHAR_BIT;
                                    tmp_json_obj["is-frame"] = item.is_frame;
                                    tmp_json_obj["length"] =
                                        item.bit_size / CHAR_BIT;
                                    tmp_json_obj["value"] = item.value;

                                    ents.push_back(tmp_json_obj);
                                }
                            }

                            tlv_node_object["metadata"]["ents"] = ents;
                        }

                        tlv_nodes_root.push_back(tlv_node_object);
                    }

                    parser_json.push_back({ "tlv-nodes", tlv_nodes_root });
                }

                if (!counter_collection.empty()) {
                    nlohmann::ordered_json counters = nlohmann::json::array();

                    for (auto &item : counter_collection) {
                        nlohmann::ordered_json ent;

                        ent["name"] = item.name;
                        ent["element-size"] = item.elem_size / CHAR_BIT;
                        ent["num-elements"] = item.num_elems;

                        counters.push_back(ent);
                    }

                    parser_json.push_back({ "counters", counters });
                }

                std::ofstream json_output(output.c_str());
                json_output << parser_json.dump(4) << std::endl;
                plog::log(std::cout) << "Done" << std::endl;
            } else {
                plog::log(std::cout) << "Unknown file extension in filename "
                                     << output << std::endl;
                return 1;
            }
        } else {
            plog::log(std::cout) << "Nothing to generate" << std::endl;
        }
    } else {
        plog::log(std::cout)
            << "No roots in this parser, use XDP2_PARSER_ADD, "
               "XDP2_PARSER[_EXT], or XDP2_PARSER_XDP"
            << std::endl;
    }
}

//
// Definitions
//

int extract_struct_constants(
    std::string cfile, std::string llvm_file, std::vector<const char *> args,
    xdp2gen::graph_t &graph,
    std::vector<xdp2gen::parser<xdp2gen::graph_t>> &roots,
    xdp2gen::clang_ast::metadata_record &metadata_record)
{
    int argc = args.size();

    plog::log(std::cout) << "Compiler args" << std::endl;
    for (auto &&item : args)
        plog::log(std::cout) << std::string(item) << "\n";
    plog::log(std::cout) << std::endl;

    llvm::Expected<clang::tooling::CommonOptionsParser> OptionsParser =
        clang::tooling::CommonOptionsParser::create(argc, &args[0],
                                                    XDP2ToolsCompilerCategory);

    if (OptionsParser) {
        clang::tooling::ClangTool Tool(
            OptionsParser->getCompilations(),
            OptionsParser->getSourcePathList(),
            std::make_shared<clang::PCHContainerOperations>());

        clang::IgnoringDiagConsumer diagConsumer;
        Tool.setDiagnosticConsumer(&diagConsumer);
        // Extracted proto node data
        std::vector<xdp2_proto_node_extract_data> consumed_proto_nodes_data;

        // Extracted proto tables data
        std::vector<xdp2_proto_table_extract_data> consumed_proto_table_data;

        // Extracted flag fields information
        std::vector<xdp2_flag_fields_extract_data> consumed_flag_fields_data;

        plog::log(std::cout)
            << "=============================================" << std::endl;
        plog::log(std::cout)
            << "*** RUNNING PROTO NODE ACTION ***" << std::endl;
        int action1 = Tool.run(extract_proto_node_struct_constants_factory(
                                   consumed_proto_nodes_data)
                                   .get());

        plog::log(std::cout)
            << "=============================================" << std::endl;
        plog::log(std::cout)
            << "*** RUNNING PROTO TABLE ACTION ***" << std::endl;
        int action2 = Tool.run(extract_proto_table_struct_constants_factory(
                                   consumed_proto_table_data)
                                   .get());

        plog::log(std::cout)
            << "=============================================" << std::endl;
        plog::log(std::cout)
            << "*** RUNNING FLAG FIELDS ACTION ***" << std::endl;
        int action3 = Tool.run(extract_flag_fields_struct_constants_factory(
                                   consumed_flag_fields_data)
                                   .get());

        // DEBUG
        plog::log(std::cout)
            << "**** COLECTED DATA FROM PROTO NODE CONSUMER ****" << std::endl;
        for (xdp2_proto_node_extract_data data : consumed_proto_nodes_data) {
            plog::log(std::cout) << data << std::endl;
        }
        // !DEBUG

        // DEBUG
        plog::log(std::cout)
            << "**** COLECTED DATA FROM PROTO TABLE CONSUMER ****" << std::endl;
        for (xdp2_proto_table_extract_data data : consumed_proto_table_data)
            plog::log(std::cout) << data << std::endl;
        // !DEBUG

        // DEBUG
        plog::log(std::cout)
            << "**** COLECTED DATA FROM FLAG FIELDS CONSUMER ****" << std::endl;
        for (xdp2_flag_fields_extract_data data : consumed_flag_fields_data)
            plog::log(std::cout) << data << std::endl;
        // !DEBUG

        plog::log(std::cout)
            << "**** ITERATING IN GRAPH FOR DATA INSERTION ****" << std::endl;
        for (auto vd : boost::make_iterator_range(vertices(graph))) {
            auto &&vertex = graph[vd];

            plog::log(std::cout)
                << "Vertex descriptor: " << vd << std::endl
                << "  - Vertex name: " << vertex.name << std::endl
                << "  - Vertex parser_node: " << vertex.parser_node
                << std::endl;

            // Expr to search for xdp2 parser proto node
            std::string search_expr_proto_node = vertex.parser_node;

            plog::log(std::cout) << "  - Proto Node Target Search Expr: "
                                 << search_expr_proto_node << std::endl;

            // Tries to find correct node to get data
            auto match_xdp2_node = std::find_if(
                consumed_proto_nodes_data.begin(),
                consumed_proto_nodes_data.end(),
                [&search_expr_proto_node](
                    const xdp2_proto_node_extract_data &node) -> bool {
                    plog::log(std::cout)
                        << "    - Compared " << search_expr_proto_node
                        << " with: " << node.decl_name << std::endl;
                    return (node.decl_name == search_expr_proto_node);
                });

            // If there is a match
            if (match_xdp2_node != consumed_proto_nodes_data.end()) {
                plog::log(std::cout)
                    << "  - FOUND CORRESPONDENT PROTO NODE" << std::endl;

                xdp2gen::transfer_data_from_proto_node_data_to_vertex(
                    (*match_xdp2_node), vertex);

                plog::log(std::cout) << (*match_xdp2_node) << std::endl;

                plog::log(std::cout)
                    << "    - Printing tlv nodes: " << std::endl;

                if (!vertex.tlv_nodes.empty()) {
                    for (auto &&tlv_node : vertex.tlv_nodes) {
                        plog::log(std::cout)
                            << "      - tlv_node name: " << tlv_node.name
                            << std::endl;
                    }
                }

            } else {
                plog::log(std::cout)
                    << "  - NOT ABLE TO FOUND CORRESPONDENT PROTO NODE"
                    << std::endl;
            }

            xdp2gen::connect_vertices(graph, vd, consumed_proto_table_data);

            xdp2gen::process_tlv_nodes_tables(vertex,
                                               consumed_proto_table_data);

            xdp2gen::process_flag_fields_nodes_tables(
                vertex, consumed_proto_table_data, consumed_flag_fields_data);
        }

        llvm::SMDiagnostic Err;
        llvm::LLVMContext Context;

        if (!llvm_file.empty()) {
            std::unique_ptr<llvm::Module> Mod(
                llvm::parseIRFile(llvm_file.c_str(), Err, Context));

            if (!Mod) {
                Err.print(args[0], llvm::errs());
                return 1;
            }

            // Extraction of offsets and size data
            plog::log(std::cout)
                << "Extracting offsets and size data for next_protos"
                << std::endl;
            plog::log(std::cout)
                << "Graph size: " << num_vertices(graph) << std::endl;
            for (auto vd : boost::make_iterator_range(vertices(graph))) {
                plog::log(std::cout)
                    << "  - protocol name: " << graph[vd].name << std::endl
                    << "  - protocol parser_node: " << graph[vd].parser_node
                    << "\n"
                    << "  - protocol has tlvs: "
                    << (graph[vd].tlv_nodes.size() > 0 ? "True" : "False")
                    << "  - protocol has next_proto: "
                    << (graph[vd].proto_next_proto.has_value() ? "True" : "False")
                    << std::endl;
                if (graph[vd].proto_next_proto.has_value()) {
                    plog::log(std::cout)
                        << "  - SEARCHING FOR OFFSETS AND SIZE INFOS"
                        << std::endl;

                    auto proto_next =
                        Mod->getFunction(graph[vd].proto_next_proto.value());
                    if (proto_next) {
                        plog::log(std::cout)
                            << "Proto_next function "
                            << graph[vd].proto_next_proto.value() << std::endl;
                        try {
                            for (auto &&block :
                                 std::ranges::views::reverse(*proto_next)) {
                                xdp2gen::llvm::llvm_graph lg{ &block };
                                auto matches =
                                    xdp2gen::llvm::proto_next_patterns.match_all<
                                        xdp2gen::llvm::
                                            packet_buffer_offset_masked_multiplied,
                                        xdp2gen::llvm::condition>(lg,
                                                                   { 0, 1 });
                                for (auto mvar : matches) {
                                    if (auto const *m = std::get_if<0>(&mvar)) {
                                        graph[vd].next_proto_data = *m;
                                    } else if (auto const *m =
                                                   std::get_if<1>(&mvar)) {
                                        graph[vd].cond_exprs = *m;
                                    }
                                }
                            }
                        } catch (std::runtime_error const &e) {
                            plog::log(std::cout)
                                << "error trying to match next_proto "
                                << e.what() << std::endl;
                        }
                    } else {
                        plog::log(std::cout)
                            << "Function next_proto not found: "
                            << *graph[vd].proto_next_proto << std::endl;
                        abort();
                    }

                    ///
                }

                auto metadata = Mod->getFunction(graph[vd].metadata);
                plog::log(std::cout)
                    << "trying metadata " << graph[vd].metadata << std::endl;
                plog::log(std::cerr)
                    << "trying metadata " << graph[vd].metadata << std::endl;
                if (metadata) {
                    plog::log(std::cout) << "found function" << std::endl;

                    auto record_map = xdp2gen::ast_consumer::metadata_records(
                        Tool, graph[vd].metadata, graph[vd].metadata_record,
                        metadata_record);

                    plog::log(std::cout)
                        << "metadata record " << graph[vd].metadata_record
                        << std::endl;

                    plog::log(std::cout) << "Trying to check for IR in "
                                         << graph[vd].metadata << std::endl;

                    plog::log(std::cout) << "working out metadata" << std::endl;
                    for (auto &&block :
                         std::ranges::views::reverse(*metadata)) {
                        xdp2gen::llvm::llvm_graph lg{ &block };
                        auto matches =
                            xdp2gen::llvm::metadata_patterns.match_all<
                                xdp2gen::llvm::metadata_transfer,
                                xdp2gen::llvm::metadata_write_constant,
                                xdp2gen::llvm::metadata_value_transfer,
                                xdp2gen::llvm::metadata_write_header_offset,
                                xdp2gen::llvm::metadata_write_header_length>(
                                lg, { 0, 0, 0, 0, 0 });
                        auto get_counter_index_name =
                            [&record_map](
                                xdp2gen::llvm::package_metadata_counter
                                    &pckg_metadata_counter) {
                                auto name_iter = record_map.find(std::make_pair(
                                    pckg_metadata_counter.offset,
                                    pckg_metadata_counter.counter_idx_size));

                                if (name_iter != record_map.end()) {
                                    plog::log(std::cout)
                                        << "package_metadata_counter : "
                                        << name_iter->second << std::endl;
                                    pckg_metadata_counter.name =
                                        name_iter->second;
                                }
                            };
                        std::string name;
                        auto &&n = graph[vd];
                        for (auto mvar : matches) {
                            if (auto *p = std::get_if<0>(&mvar)) {
                                auto offset = p->dst_bit_offset;
                                auto field_size = p->bit_size;
                                if (p->different_dst_bit_size)
                                    field_size =
                                        p->different_dst_bit_size.value();

                                if (p->pckg_metadata_counter.has_value()) {
                                    get_counter_index_name(
                                        p->pckg_metadata_counter.value());
                                    offset += p->pckg_metadata_counter
                                                  ->local_to_counter_offset;
                                    field_size =
                                        p->pckg_metadata_counter->field_size;
                                }

                                auto name_iter = record_map.find(
                                    std::make_pair(offset, field_size));

                                if (name_iter != record_map.end()) {
                                    plog::log(std::cout)
                                        << "metdata_name: " << name_iter->second
                                        << std::endl;
                                    name = name_iter->second;
                                }

                                n.metadata_transfers.push_back({ name, *p });
                            } else if (auto *p = std::get_if<1>(&mvar)) {
                                auto offset = p->dst_bit_offset;
                                auto field_size = p->bit_size;
                                if (p->different_dst_bit_size)
                                    field_size =
                                        p->different_dst_bit_size.value();

                                if (p->pckg_metadata_counter.has_value()) {
                                    get_counter_index_name(
                                        p->pckg_metadata_counter.value());
                                    offset += p->pckg_metadata_counter
                                                  ->local_to_counter_offset;
                                    field_size =
                                        p->pckg_metadata_counter->field_size;
                                }

                                auto name_iter = record_map.find(
                                    std::make_pair(offset, field_size));

                                if (name_iter != record_map.end()) {
                                    plog::log(std::cout)
                                        << "metdata_name: " << name_iter->second
                                        << std::endl;
                                    name = name_iter->second;
                                }

                                n.metadata_transfers.push_back({ name, *p });
                            } else if (auto *p = std::get_if<2>(&mvar)) {
                                auto offset = p->dst_bit_offset;
                                auto field_size = p->bit_size;
                                if (p->different_dst_bit_size)
                                    field_size =
                                        p->different_dst_bit_size.value();

                                if (p->pckg_metadata_counter.has_value()) {
                                    get_counter_index_name(
                                        p->pckg_metadata_counter.value());
                                    offset += p->pckg_metadata_counter
                                                  ->local_to_counter_offset;
                                    field_size =
                                        p->pckg_metadata_counter->field_size;
                                }

                                auto name_iter = record_map.find(
                                    std::make_pair(offset, field_size));
                                if (name_iter != record_map.end()) {
                                    plog::log(std::cout)
                                        << "metdata_name: " << name_iter->second
                                        << std::endl;
                                    name = name_iter->second;
                                }

                                n.metadata_transfers.push_back({ name, *p });
                            } else if (auto *p = std::get_if<3>(&mvar)) {
                                auto offset = p->dst_bit_offset;
                                auto field_size = p->bit_size;

                                if (p->pckg_metadata_counter.has_value()) {
                                    get_counter_index_name(
                                        p->pckg_metadata_counter.value());
                                    offset += p->pckg_metadata_counter
                                                  ->local_to_counter_offset;
                                    field_size =
                                        p->pckg_metadata_counter->field_size;
                                }

                                auto name_iter = record_map.find(
                                    std::make_pair(offset, field_size));

                                if (name_iter != record_map.end()) {
                                    plog::log(std::cout)
                                        << "metdata_name: " << name_iter->second
                                        << std::endl;
                                    name = name_iter->second;
                                }

                                n.metadata_transfers.push_back({ name, *p });
                            } else if (auto *p = std::get_if<4>(&mvar)) {
                                auto offset = p->dst_bit_offset;
                                auto field_size = p->bit_size;

                                if (p->pckg_metadata_counter.has_value()) {
                                    get_counter_index_name(
                                        p->pckg_metadata_counter.value());
                                    offset += p->pckg_metadata_counter
                                                  ->local_to_counter_offset;
                                    field_size =
                                        p->pckg_metadata_counter->field_size;
                                }

                                auto name_iter = record_map.find(
                                    std::make_pair(offset, field_size));

                                if (name_iter != record_map.end()) {
                                    plog::log(std::cout)
                                        << "metdata_name: " << name_iter->second
                                        << std::endl;
                                    name = name_iter->second;
                                }

                                n.metadata_transfers.push_back({ name, *p });
                            }
                        }
                    }
                }

                if (graph[vd].proto_len.has_value()) {
                    auto proto_len =
                        Mod->getFunction(graph[vd].proto_len.value());
                    if (proto_len) {
                        try {
                            for (auto &&block :
                                 std::ranges::views::reverse(*proto_len)) {
                                xdp2gen::llvm::llvm_graph lg{ &block };
                                plog::log(std::cout)
                                    << "Length function is "
                                    << graph[vd].proto_len.value() << std::endl;
                                auto matches =
                                    xdp2gen::llvm::proto_next_patterns.match_all<
                                        xdp2gen::llvm::
                                            packet_buffer_offset_masked_multiplied>(
                                        lg, { 0 });
                                for (auto mvar : matches) {
                                    if (auto const *m = std::get_if<0>(&mvar)) {
                                        graph[vd].len_data = *m;
                                    }
                                }
                            }
                        } catch (std::runtime_error const &e) {
                            plog::log(std::cout)
                                << "error trying to match next_proto "
                                << e.what() << std::endl;
                        }
                    }
                }

                if (graph[vd].tlv_type.has_value()) {
                    plog::log(std::cout) << "TLVs node was detected in "
                                         << graph[vd].name << std::endl;

                    auto tlv_type_func =
                        Mod->getFunction(graph[vd].tlv_type.value());

                    if (tlv_type_func) {
                        try {
                            for (auto &&block :
                                 std::ranges::views::reverse(*tlv_type_func)) {
                                xdp2gen::llvm::llvm_graph lg{ &block };
                                auto matches =
                                    xdp2gen::llvm::tlv_patterns.match_all<
                                        xdp2gen::llvm::packet_buffer_load>(
                                        lg, { 0 });
                                for (auto mvar : matches) {
                                    if (auto const *m = std::get_if<0>(&mvar)) {
                                        graph[vd].tlv_type_val = *m;
                                    }
                                }
                            }

                        } catch (std::runtime_error const &e) {
                            plog::log(std::cout)
                                << "error trying to match tlv_type " << e.what()
                                << std::endl;
                        }
                    }
                }

                if (graph[vd].tlv_len.has_value()) {
                    plog::log(std::cout) << "TLVs node was detected in "
                                         << graph[vd].name << std::endl;

                    auto tlv_len_func =
                        Mod->getFunction(graph[vd].tlv_len.value());

                    if (tlv_len_func) {
                        try {
                            for (auto &&block :
                                 std::ranges::views::reverse(*tlv_len_func)) {
                                xdp2gen::llvm::llvm_graph lg{ &block };
                                auto matches =
                                    xdp2gen::llvm::tlv_patterns.match_all<
                                        xdp2gen::llvm::
                                            packet_buffer_offset_masked_multiplied>(
                                        lg, { 0 });
                                for (auto mvar : matches) {
                                    if (auto const *m = std::get_if<0>(&mvar)) {
                                        graph[vd].tlv_len_val = *m;
                                    }
                                }
                            }

                        } catch (std::runtime_error const &e) {
                            plog::log(std::cout)
                                << "error trying to match tlv_len " << e.what()
                                << std::endl;
                        }
                    }
                }

                if (graph[vd].tlv_start_offset.has_value()) {
                    plog::log(std::cout) << "TLVs node was detected in "
                                         << graph[vd].name << std::endl;

                    auto tlv_start_offset_func =
                        Mod->getFunction(graph[vd].tlv_start_offset.value());

                    if (tlv_start_offset_func) {
                        try {
                            for (auto &&block : std::ranges::views::reverse(
                                     *tlv_start_offset_func)) {
                                xdp2gen::llvm::llvm_graph lg{ &block };
                                auto matches =
                                    xdp2gen::llvm::tlv_patterns.match_all<
                                        xdp2gen::llvm::constant_value>(lg,
                                                                        { 0 });
                                for (auto mvar : matches) {
                                    if (auto const *m = std::get_if<0>(&mvar)) {
                                        graph[vd].tlv_start_offset_val = *m;
                                    }
                                }
                            }

                        } catch (std::runtime_error const &e) {
                            plog::log(std::cout)
                                << "error trying to match tlv_len " << e.what()
                                << std::endl;
                        }
                    }
                }

                if (!graph[vd].tlv_nodes.empty()) {
                    auto tlv_node_ref_list = graph[vd].get_all_tlv_node_list();

                    for (auto tlv_node_ref_item : tlv_node_ref_list) {
                        tlv_node_ref_item.get().parent_parse_node = &graph[vd];
                        auto metadata =
                            Mod->getFunction(tlv_node_ref_item.get().metadata);
                        if (metadata) {
                            auto record_map =
                                xdp2gen::ast_consumer::metadata_records(
                                    Tool, tlv_node_ref_item.get().metadata,
                                    graph[vd].metadata_record, metadata_record);

                            plog::log(std::cout)
                                << "Handling TLV nodes metadata: "
                                << tlv_node_ref_item.get().metadata
                                << std::endl;

                            try {
                                for (auto &&block :
                                     std::ranges::views::reverse(*metadata)) {
                                    xdp2gen::llvm::llvm_graph lg{ &block };
                                    auto matches =
                                        xdp2gen::llvm::metadata_patterns
                                            .match_all<
                                                xdp2gen::llvm::metadata_transfer,
                                                xdp2gen::llvm::
                                                    metadata_write_constant>(
                                                lg, { 0, 0 });
                                    std::string name;
                                    for (auto mvar : matches) {
                                        if (auto *p = std::get_if<0>(&mvar)) {
                                            auto offset = p->dst_bit_offset;
                                            auto field_size = p->bit_size;

                                            auto name_iter = record_map.find(
                                                std::make_pair(offset,
                                                               field_size));

                                            if (name_iter != record_map.end()) {
                                                plog::log(std::cout)
                                                    << "metdata_name: "
                                                    << name_iter->second
                                                    << std::endl;
                                                name = name_iter->second;
                                            }

                                            tlv_node_ref_item.get()
                                                .metadata_transfers.push_back(
                                                    xdp2gen::metadata_transfer{
                                                        name, *p });
                                        } else if (auto *p =
                                                       std::get_if<1>(&mvar)) {
                                            auto offset = p->dst_bit_offset;
                                            auto field_size = p->bit_size;

                                            auto name_iter = record_map.find(
                                                std::make_pair(offset,
                                                               field_size));

                                            if (name_iter != record_map.end()) {
                                                plog::log(std::cout)
                                                    << "metdata_name: "
                                                    << name_iter->second
                                                    << std::endl;
                                                name = name_iter->second;
                                            }

                                            tlv_node_ref_item.get()
                                                .metadata_transfers.push_back(
                                                    xdp2gen::metadata_transfer{
                                                        name, *p });
                                        }
                                    }
                                }
                            } catch (std::runtime_error const &e) {
                                plog::log(std::cout)
                                    << "error trying to match metadata "
                                    << e.what() << std::endl;
                            }
                        }
                    }
                }

                if (!graph[vd].flag_fields_nodes.empty()) {
                    for (auto &flag_field_item : graph[vd].flag_fields_nodes) {
                        auto metadata =
                            Mod->getFunction(flag_field_item.metadata);
                        if (metadata) {
                            auto record_map =
                                xdp2gen::ast_consumer::metadata_records(
                                    Tool, flag_field_item.metadata,
                                    graph[vd].metadata_record, metadata_record);

                            plog::log(std::cout)
                                << "Handling Flag Fields nodes metadata: "
                                << flag_field_item.metadata << std::endl;

                            try {
                                for (auto &&block :
                                     std::ranges::views::reverse(*metadata)) {
                                    xdp2gen::llvm::llvm_graph lg{ &block };
                                    auto matches =
                                        xdp2gen::llvm::metadata_patterns
                                            .match_all<xdp2gen::llvm::
                                                           metadata_transfer>(
                                                lg, { 0 });
                                    std::string name;
                                    for (auto mvar : matches) {
                                        if (auto *p = std::get_if<0>(&mvar)) {
                                            auto offset = p->dst_bit_offset;
                                            auto field_size = p->bit_size;

                                            auto name_iter = record_map.find(
                                                std::make_pair(offset,
                                                               field_size));

                                            if (name_iter != record_map.end()) {
                                                plog::log(std::cout)
                                                    << "metdata_name: "
                                                    << name_iter->second
                                                    << std::endl;
                                                name = name_iter->second;
                                            }

                                            flag_field_item.metadata_transfers
                                                .push_back({ name, *p });
                                        }
                                    }
                                }
                            } catch (std::runtime_error const &e) {
                                plog::log(std::cout)
                                    << "error trying to match metadata "
                                    << e.what() << std::endl;
                            }
                        }
                    }
                }
                if (!graph[vd].start_fields_offset.empty()) {
                    auto start_fields_offset_func =
                        Mod->getFunction(graph[vd].start_fields_offset);

                    if (start_fields_offset_func) {
                        try {
                            for (auto &&block : std::ranges::views::reverse(
                                     *start_fields_offset_func)) {
                                xdp2gen::llvm::llvm_graph lg{ &block };
                                auto matches =
                                    xdp2gen::llvm::tlv_patterns.match_all<
                                        xdp2gen::llvm::constant_value>(lg,
                                                                        { 0 });
                                for (auto mvar : matches) {
                                    if (auto const *m = std::get_if<0>(&mvar)) {
                                        graph[vd].start_fields_offset_value =
                                            m->value * CHAR_BIT;
                                    }
                                }
                            }

                        } catch (std::runtime_error const &e) {
                            plog::log(std::cout)
                                << "error trying to match start_fields_offset: "
                                << e.what() << std::endl;
                        }
                    }

                    auto get_flags_func = Mod->getFunction(graph[vd].get_flags);

                    if (get_flags_func) {
                        try {
                            for (auto &&block :
                                 std::ranges::views::reverse(*get_flags_func)) {
                                xdp2gen::llvm::llvm_graph lg{ &block };
                                auto matches =
                                    xdp2gen::llvm::proto_next_patterns.match_all<
                                        xdp2gen::llvm::
                                            packet_buffer_offset_masked_multiplied>(
                                        lg, { 0 });
                                for (auto mvar : matches) {
                                    if (auto const *m = std::get_if<0>(&mvar)) {
                                        graph[vd].flags_data = *m;
                                    }
                                }
                            }

                        } catch (std::runtime_error const &e) {
                            plog::log(std::cout)
                                << "error trying to match get_flags: "
                                << e.what() << std::endl;
                        }
                    }
                }
            }
        }
    } else {
        plog::log(std::cout) << "There's an error" << std::endl;
        llvm::handleAllErrors(OptionsParser.takeError());
        return -1;
    }
    plog::log(std::cout) << "Returning correctly" << std::endl;
    return 0;
}
