
#ifndef INCLUDE_XDP2GEN_AST_CONSUMER_PATTERNS_H_CPP2
#define INCLUDE_XDP2GEN_AST_CONSUMER_PATTERNS_H_CPP2


//=== Cpp2 type declarations ====================================================


#include "cpp2util.h"



//=== Cpp2 type definitions and function declarations ===========================

// SPDX-License-Identifier: BSD-2-Clause-FreeBSD
/*
 * Copyright (c) 2024 SiXDP2 Inc.
 *
 * Authors: Otávio Lucas Alves da Silva <otavio.silva@expertisesolutions.com.br>
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

#include <any>
#include <functional>
#include <vector>
#include <string>
#include <tuple>

#include "xdp2gen/ast-consumer/clang_ast_graph.h"

namespace xdp2gen
{
namespace ast_consumer
{

struct metadata_record {
    clang::RecordDecl *record;
    std::string record_name;
    std::string field_name;

    inline friend std::ostream &operator<<(std::ostream &os, metadata_record r)
    {
        return os << "metadata_record [ record_name: " << r.record_name
                  << ", field_name: " << r.field_name << "]";
    }
};

}
}

using clang_match_type = std::function<std::vector<std::vector<std::tuple<size_t, std::any>>>(xdp2gen::ast_consumer::clang_ast_graph const &)>;

#line 59 "include/xdp2gen/ast-consumer/patterns.h2"
[[nodiscard]] auto get_metadata_record(clang::MemberExpr const* expr) -> std::optional<xdp2gen::ast_consumer::metadata_record>;
    

#line 137 "include/xdp2gen/ast-consumer/patterns.h2"
// Assignment patterns
/* The following pattern matches an assignment operation (clang::BinaryOperator
  class) to a member expression of a struct (clang::MemberExpr). The action
  will return an instance of 'struct xdp2gen::ast_consumer::metadata_record'
  if successful.
  This pattern would match the following statement:
 
  `frame->ip_proto = iph->protocol;`
 */
extern clang_match_type clang_ast_assignment_pattern;

#line 176 "include/xdp2gen/ast-consumer/patterns.h2"
// Increment patterns
/* The following pattern matches an increment operation (clang::UnaryOperator
  class) to a member expression of a struct (clang::MemberExpr). The action
  will return an instance of 'struct xdp2gen::ast_consumer::metadata_record'
  if successful.
  This pattern would match the following statement:
 
  `frame->ip_proto++;`
 */
extern clang_match_type clang_ast_increment_pattern;

#line 215 "include/xdp2gen/ast-consumer/patterns.h2"
// Call patterns
/* The following pattern matches a call to a memcpy function (clang::CallExpr
  class) whose first operand is an "address of" operation (prefixed '&',
  clang::UnaryOperator class) of a member expression of a struct
  (clang::MemberExpr). The action will return an instance of 'struct
  xdp2gen::ast_consumer::metadata_record' if successful.
  This pattern would match the following statement:
 
  `memcpy(&frame->ip_proto, &iph->protocol, sizeof(frame->ip_proto));`
 */
extern clang_match_type clang_ast_call_pattern1;

#line 270 "include/xdp2gen/ast-consumer/patterns.h2"
/* The following pattern matches a call to a memcpy function (clang::CallExpr
  class) whose first operand is a member expression of a struct
  (clang::MemberExpr). The action will return an instance of 'struct
  xdp2gen::ast_consumer::metadata_record' if successful.
  This pattern would match the following statement:
 
  `memcpy(meta->tcp_options.sack, opt->sack, sizeof(meta->tcp_options.sack));`
 */
extern clang_match_type clang_ast_call_pattern2;

//=== Cpp2 function definitions =================================================


#line 59 "include/xdp2gen/ast-consumer/patterns.h2"
[[nodiscard]] auto get_metadata_record(clang::MemberExpr const* expr) -> std::optional<xdp2gen::ast_consumer::metadata_record>

{
    auto member_expr {expr}; 
    auto range {CPP2_UFCS_0(children, (*cpp2::assert_not_null(expr)))}; 
    auto current_field_name {std::string()}; 
    auto name {std::string()}; 
    auto record_name {std::string()}; 
    auto is_array {false}; 
    auto literal_index {-1UL}; 
    while( std::distance(std::begin(range), std::end(range)) == 1 && (
        clang::dyn_cast<clang::MemberExpr>(*cpp2::assert_not_null(std::begin(range))) != nullptr 
        || clang::dyn_cast<clang::ArraySubscriptExpr>(*cpp2::assert_not_null(std::begin(range))) != nullptr) ) 
    {
        auto field {clang::dyn_cast<clang::FieldDecl>(
            CPP2_UFCS_0(getMemberDecl, (*cpp2::assert_not_null(member_expr))))}; 
        current_field_name = CPP2_UFCS_0(getNameAsString, (*cpp2::assert_not_null(field)));
        if (!(CPP2_UFCS_0(empty, current_field_name))) {
            auto ss {std::stringstream()}; 
            if ((is_array)) {
                ss << "[";
                if (literal_index != -1UL) {
                    ss << literal_index;
                }
                ss << "]";
                is_array = false;
                literal_index = -1UL;
            }
            if (CPP2_UFCS_0(empty, name)) {
                name = current_field_name + CPP2_UFCS_0(str, ss);
            }else {
                name = current_field_name + CPP2_UFCS_0(str, ss) + "." + name;
            }
        }
        clang::Stmt const* range_begin {expr}; 
        auto ar_expr {clang::dyn_cast<clang::ArraySubscriptExpr>(
            *cpp2::assert_not_null(std::begin(range)))}; 
        if (ar_expr) {
            auto ar_range {CPP2_UFCS_0(children, (*cpp2::assert_not_null(CPP2_UFCS_0(getBase, (*cpp2::assert_not_null(ar_expr))))))}; 
            range_begin = *cpp2::assert_not_null(std::begin(ar_range));
            is_array = true;
            auto int_lit_expr {clang::dyn_cast<clang::IntegerLiteral>(CPP2_UFCS_0(getIdx, (*cpp2::assert_not_null(ar_expr))))}; 
            if (int_lit_expr) {
                literal_index = CPP2_UFCS_0(getZExtValue, CPP2_UFCS_0(getValue, (*cpp2::assert_not_null(int_lit_expr))));
            }
        }else {
            range_begin = *cpp2::assert_not_null(std::begin(range));
        }
        auto child {clang::dyn_cast<clang::MemberExpr>(range_begin)}; 

        if (child != nullptr) {
            member_expr = child;
            range = CPP2_UFCS_0(children, (*cpp2::assert_not_null(member_expr)));
        }else {
            std::cerr << "child is null\n";
            std::abort();
        }
    }
    if (std::distance(std::begin(range), std::end(range)) == 0 || 
        clang::dyn_cast<clang::MemberExpr>(*cpp2::assert_not_null(std::begin(std::move(range)))) == nullptr) 
    {
        auto field {clang::dyn_cast<clang::FieldDecl>(
            CPP2_UFCS_0(getMemberDecl, (*cpp2::assert_not_null(member_expr))))}; 
        current_field_name = CPP2_UFCS_0(getNameAsString, (*cpp2::assert_not_null(field)));
        if (!(CPP2_UFCS_0(empty, current_field_name))) {
            if (CPP2_UFCS_0(empty, name)) {
                name = std::move(current_field_name);
            }else {
                name = std::move(current_field_name) + "." + name;
            }
        }
        auto record {CPP2_UFCS_0(getParent, (*cpp2::assert_not_null(field)))}; 
        record_name = CPP2_UFCS_0(getNameAsString, (*cpp2::assert_not_null(record)));
        return xdp2gen::ast_consumer::metadata_record(std::move(record), std::move(record_name), std::move(name)); 
    }
    return std::nullopt; 
}

#line 146 "include/xdp2gen/ast-consumer/patterns.h2"
clang_match_type clang_ast_assignment_pattern {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 2>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &assign)

#line 149 "include/xdp2gen/ast-consumer/patterns.h2"
    {
        if (!(std::holds_alternative<clang::Stmt const*>(assign))) {
            return false; 
        }
        auto stmt {std::get<clang::Stmt const*>(assign)}; 
        auto binop {clang::dyn_cast<clang::BinaryOperator>(stmt)}; 

        return binop && CPP2_UFCS_0_NONLOCAL(isAssignmentOp, (*cpp2::assert_not_null(binop))); 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &assign, const graph_attrs &member) -> std::any{
        auto expr {clang::dyn_cast<clang::MemberExpr>(
            std::get<clang::Stmt const*>(member))}; 
        auto mr {get_metadata_record(expr)}; 
        if (CPP2_UFCS_0_NONLOCAL(has_value, mr)) {
            return CPP2_UFCS_0_NONLOCAL(value, mr); 
        }
        return std::any(); 
    }}});pat.add_attrs(1, [] (graph_attrs const &member)

    {
        if (!(std::holds_alternative<clang::Stmt const*>(member))) {
            return false; 
        }
        auto stmt {std::get<clang::Stmt const*>(member)}; 
        return clang::dyn_cast<clang::MemberExpr>(stmt) != nullptr; 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 185 "include/xdp2gen/ast-consumer/patterns.h2"
clang_match_type clang_ast_increment_pattern {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 2>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &inc)

#line 188 "include/xdp2gen/ast-consumer/patterns.h2"
    {
        if (!(std::holds_alternative<clang::Stmt const*>(inc))) {
            return false; 
        }
        auto stmt {std::get<clang::Stmt const*>(inc)}; 
        auto unop {clang::dyn_cast<clang::UnaryOperator>(stmt)}; 

        return unop && CPP2_UFCS_0_NONLOCAL(isIncrementOp, (*cpp2::assert_not_null(unop))); 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &inc, const graph_attrs &member) -> std::any{
        auto expr {clang::dyn_cast<clang::MemberExpr>(
            std::get<clang::Stmt const*>(member))}; 
        auto mr {get_metadata_record(expr)}; 
        if (CPP2_UFCS_0_NONLOCAL(has_value, mr)) {
            return CPP2_UFCS_0_NONLOCAL(value, mr); 
        }
        return std::any(); 
    }}});pat.add_attrs(1, [] (graph_attrs const &member)

    {
        if (!(std::holds_alternative<clang::Stmt const*>(member))) {
            return false; 
        }
        auto stmt {std::get<clang::Stmt const*>(member)}; 
        return clang::dyn_cast<clang::MemberExpr>(stmt) != nullptr; 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 225 "include/xdp2gen/ast-consumer/patterns.h2"
clang_match_type clang_ast_call_pattern1 {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 3>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &call)

#line 228 "include/xdp2gen/ast-consumer/patterns.h2"
    {
        if (!(std::holds_alternative<clang::Stmt const*>(call))) {
            return false; 
        }
        auto stmt {std::get<clang::Stmt const*>(call)}; 
        auto call_expr {clang::dyn_cast<clang::CallExpr>(stmt)}; 

        if (call_expr) {
            auto func_decl {CPP2_UFCS_0(getDirectCallee, (*cpp2::assert_not_null(call_expr)))}; 
            return func_decl && CPP2_UFCS_0_NONLOCAL(getName, (*cpp2::assert_not_null(func_decl))) == "memcpy"; 
        }
        return false; 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &call, const graph_attrs &addressof, const graph_attrs &member) -> std::any{
        auto expr {clang::dyn_cast<clang::MemberExpr>(
            std::get<clang::Stmt const*>(member))}; 
        auto mr {get_metadata_record(expr)}; 
        if (CPP2_UFCS_0_NONLOCAL(has_value, mr)) {
            return CPP2_UFCS_0_NONLOCAL(value, mr); 
        }
        return std::any(); 
    }}});pat.add_attrs(1, [] (graph_attrs const &addressof)

    {
        if (!(std::holds_alternative<clang::Stmt const*>(addressof))) {
            return false; 
        }
        auto stmt {std::get<clang::Stmt const*>(addressof)}; 
        auto unop {clang::dyn_cast<clang::UnaryOperator>(stmt)}; 

        return unop 
            && CPP2_UFCS_0_NONLOCAL(getOpcode, (*cpp2::assert_not_null(unop))) == clang::UnaryOperator::Opcode::UO_AddrOf; 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &member)

    {
        if (!(std::holds_alternative<clang::Stmt const*>(member))) {
            return false; 
        }
        auto stmt {std::get<clang::Stmt const*>(member)}; 
        return clang::dyn_cast<clang::MemberExpr>(stmt) != nullptr; 
    });pattern_edges_map.insert({{0, 1}, {1, 1}});pattern_edges_map.insert({{1, 2}, {1, std::nullopt}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 278 "include/xdp2gen/ast-consumer/patterns.h2"
clang_match_type clang_ast_call_pattern2 {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 2>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &call)

#line 281 "include/xdp2gen/ast-consumer/patterns.h2"
    {
        if (!(std::holds_alternative<clang::Stmt const*>(call))) {
            return false; 
        }
        auto stmt {std::get<clang::Stmt const*>(call)}; 
        auto call_expr {clang::dyn_cast<clang::CallExpr>(stmt)}; 

        if (call_expr) {
            auto func_decl {CPP2_UFCS_0(getDirectCallee, (*cpp2::assert_not_null(call_expr)))}; 
            return func_decl && CPP2_UFCS_0_NONLOCAL(getName, (*cpp2::assert_not_null(func_decl))) == "memcpy"; 
        }
        return false; 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &call, const graph_attrs &member) -> std::any{
        auto expr {clang::dyn_cast<clang::MemberExpr>(
            std::get<clang::Stmt const*>(member))}; 
        auto mr {get_metadata_record(expr)}; 
        if (CPP2_UFCS_0_NONLOCAL(has_value, mr)) {
            return CPP2_UFCS_0_NONLOCAL(value, mr); 
        }
        return std::any(); 
    }}});pat.add_attrs(1, [] (graph_attrs const &member)

    {
        if (!(std::holds_alternative<clang::Stmt const*>(member))) {
            return false; 
        }
        auto stmt {std::get<clang::Stmt const*>(member)}; 
        return clang::dyn_cast<clang::MemberExpr>(stmt) != nullptr; 
    });pattern_edges_map.insert({{0, 1}, {1, 1}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 
#endif

