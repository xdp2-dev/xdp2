
#ifndef INCLUDE_XDP2GEN_LLVM_PATTERNS_H_CPP2
#define INCLUDE_XDP2GEN_LLVM_PATTERNS_H_CPP2


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

#include <functional>
#include <set>
#include <tuple>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"

#include "xdp2gen/llvm/llvm_graph.h"

using match_type = std::function<std::vector<std::vector<std::tuple<size_t, std::any>>>(xdp2gen::llvm::llvm_graph const &)>;
/// LLVM patterns

// TLV patterns
/* The following pattern matches a calculation of a tlv parameter value that is
  is performed by just loading the value of a memory region at an offset of a
  struct pointer as the first argument of the function. The action will return
  an instance of 'struct
  xdp2gen::llvm::packet_buffer_offset_masked_multiplied' if successful.
  This pattern would match the following LLVM block:
 
  `%2 = getelementptr inbounds %struct.tcp_opt, ptr %0, i64 0, i32 1`
  `%3 = load i8, ptr %2, align 1, !tbaa !25`
  `%4 = zext i8 %3 to i64`
  `ret i64 %4`
 */
#line 55 "include/xdp2gen/llvm/patterns.h2"
extern match_type tlv_pattern_load_gep;

#line 82 "include/xdp2gen/llvm/patterns.h2"
        /// TODO: insert the asserts and exceptions later?

#line 90 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches a calculation of a tlv parameter value that is
  performed by just loading the value of a memory region pointed by the first
  argument of the function. The action will return an instance of 'struct
  xdp2gen::llvm::packet_buffer_load' if successful.
  This pattern would match the following LLVM block:
 
  `%2 = load i8, ptr %0, align 1, !tbaa !27`
  `%3 = zext i8 %2 to i32`
  `ret i32 %3`
 */
extern match_type tlv_pattern_load;

#line 121 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches a calculation of a tlv parameter value that is
  performed by just returning a constant. The action will return an instance
  of 'struct xdp2gen::llvm::constant_value' if successful.
  This pattern would match the following LLVM block:
 
  `ret i64 20`
 */
extern match_type tlv_pattern_const;

#line 147 "include/xdp2gen/llvm/patterns.h2"
// Proto patterns
/* The following pattern matches a calculation of a "proto next" value that is
  performed by just loading the value of a memory region at an offset of a
  struct pointer as the first argument of the function. The action will return
  an instance of 'struct
  xdp2gen::llvm::packet_buffer_offset_masked_multiplied' if successful.
  This pattern would match the following LLVM block:
 
  `%2 = getelementptr inbounds %struct.ethhdr, ptr %0, i64 0, i32 2`
  `%3 = load i16, ptr %2, align 1, !tbaa !5`
  `%4 = zext i16 %3 to i32`
  `ret i32 %4`
 */
extern match_type proto_next_pattern_load_gep;

#line 194 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches a calculation of a "proto next" value that is
  performed by loading the value of a memory region and then performing a
  shift operation. The pointer used in the load instruction must be the first
  argument of the function. The action will return an instance of 'struct
  xdp2gen::llvm::packet_buffer_offset_masked_multiplied' if successful.
  This pattern would match the following LLVM block:
 
  `%2 = load i8, ptr %0, align 1`
  `%3 = lshr i8 %2, 4`
  `%4 = zext i8 %3 to i32`
  `ret i32 %4`
 */
extern match_type proto_next_pattern_shift_load;

#line 257 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches a calculation of a "proto next" value that is
  performed by just returning a constant. No action will be performed.
  This pattern would match the following LLVM block:
 
  `ret i32 0`
 */
extern match_type proto_next_pattern_const;

#line 273 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches a calculation of a "proto next" value in
  a few more steps than the previous patterns. It starts by loading a value
  from the memory region pointed by the first argument of the function, and
  then it is applied a bit mask for an integer comparison against a constant.
  Different constants are returned depending on the result of the comparison.
  The first action will return an instance of 'struct
  xdp2gen::llvm::packet_buffer_offset_masked_multiplied' if the match occurs.
  On other hand, the second action will return an instance of 'struct
  xdp2gen::llvm::condition', representing the condition that must be met for
  a successful operation.
  This pattern would match the following LLVM block:
 
  `%2 = load i8, ptr %0, align 1`
  `%3 = and i8 %2, -16`
  `%4 = icmp eq i8 %3, 64`
  `%5 = select i1 %4, i32 0, i32 -14`
  `ret i32 %5`
 */
extern match_type proto_next_pattern_cond_mask_load;

#line 388 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches a calculation of a "proto next" value that is
  very similar to the 'proto_next_pattern_shift_load' pattern, the difference
  being that a bit mask operation ('and') is performed after the 'shift'
  operation. It will return an instance of 'struct
  xdp2gen::llvm::packet_buffer_offset_masked_multiplied' if successful.
  This pattern would match the following LLVM block:
 
  `%2 = load i16, ptr %0, align 2, !tbaa !36`
  `%3 = lshr i16 %2, 8`
  `%4 = and i16 %3, 7`
  `%5 = zext i16 %4 to i32`
  `ret i32 %5`
 */
extern match_type proto_next_pattern_mask_shift_load;

#line 477 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches a calculation of a "proto next" value that is
  very similar to the 'proto_next_pattern_mask_shift_load' pattern, the
  difference being that the load occurs from a pointer calculated with a 'get
  element ptr' operation. It will return an instance of 'struct
  xdp2gen::llvm::packet_buffer_offset_masked_multiplied' if successful.
  This pattern would match the following LLVM block:
 
  `%1 = getelementptr inbounds %struct.gre_hdr, ptr %0, i64 0, i32 1`
  `%2 = load i16, ptr %1, align 2, !tbaa !36`
  `%3 = lshr i16 %2, 8`
  `%4 = and i16 %3, 7`
  `%5 = zext i16 %4 to i32`
  `ret i32 %5`
 */
extern match_type proto_next_pattern_mask_shift_gep;

#line 579 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches a calculation of a "proto next" value that is
  performed by just loading the value of a memory region pointed by the first
  argument of the function. The action will return an instance of 'struct
  xdp2gen::llvm::packet_buffer_offset_masked_multiplied' if successful.
  This pattern would match the following LLVM block:
 
  `%2 = load i16, ptr %0, align 2, !tbaa !39`
  `%3 = zext i16 %2 to i32`
  `ret i32 %4`
 */
extern match_type proto_next_pattern_load;

#line 613 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches a calculation of a "proto next" value that is
  similar to the 'proto_next_pattern_cond_mask_load', but it takes a few steps
  further. It starts by loading a value from a memory region at an offset of
  the pointer represented by the first argument. It then applies a mask to
  this value and compares it against a constant. If the test is successful,
  a new value at an offset of the first argument is loaded and then returned,
  otherwise a constant is returned. This selection of which SSA value to
  return is performed via 'phi node' with its respective branches.
  This pattern would match the following LLVM blocks:
 
  `%2 = getelementptr inbounds %struct.iphdr, ptr %0, i64 0, i32 4`
  `%3 = load i16, ptr %2, align 2, !tbaa !23`
  `%4 = and i16 %3, -225`
  `%5 = icmp eq i16 %4, 0`
  `br i1 %5, label %6, label %10`
 
  `6:`
  `%7 = getelementptr inbounds %struct.iphdr, ptr %0, i64 0, i32 6`
  `%8 = load i8, ptr %7, align 1, !tbaa !14`
  `%9 = zext i8 %8 to i32`
  `br label %10`
 
  `10:`
  `%11 = phi i32 [ %9, %6 ], [ -4, %1 ]`
  `ret i32 %11`
 */
extern match_type proto_next_pattern_branch_mask_load_gep;

#line 759 "include/xdp2gen/llvm/patterns.h2"
// Metadata patterns
// transfer
/* The following pattern matches an operation of metadata transfer that is
  performed by a pair of pointer offset calculations (both source and
  destination), a load from the source and a store to the destination. The
  source pointer must be calculated from the first argument and the
  destination pointer must be calculated from either the fourth or fifth
  arguments. The action will return an instance of 'struct
  xdp2gen::llvm::metadata_transfer' if successful.
  This pattern would match the following LLVM block:
 
  `%7 = getelementptr inbounds %struct.ipv6hdr, ptr %0, i64 0, i32 3`
  `%8 = load i8, ptr %7, align 2, !tbaa !43`
  `%9 = getelementptr inbounds %struct.metadata, ptr %4, i64 0, i32 9`
  `store i8 %8, ptr %9, align 1, !tbaa !16`
 */
extern match_type metadata_pattern_transfer_lhs_load_gep_rhs_gep;

#line 827 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches an operation of metadata transfer that is
  performed by a load from the memory region that is pointer by the first
  argument of the function (source) and a pointer offset calculation for the
  destination. A store operation is then performed. The destination pointer
  must be calculated from either the fourth or fifth arguments. The action
  will return an instance of 'struct xdp2gen::llvm::metadata_transfer' if
  successful.
  This pattern would match the following LLVM block:
 
  `%7 = load i16, ptr %0, align 2, !tbaa !24`
  `%8 = getelementptr inbounds %struct.metadata, ptr %4, i64 0, i32 11`
  `store i16 %7, ptr %8, align 4, !tbaa !18`
 */
extern match_type metadata_pattern_transfer_lhs_load_rhs_gep;

#line 881 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches an operation of metadata transfer that is
  similar to the pattern 'metadata_pattern_transfer_lhs_load_gep_rhs_gep', the
  difference being that a call to a byte swap intrinsic is performed and then
  the resulting SSA value is stored into the destination.
  This pattern would match the following LLVM block:
 
  `%7 = getelementptr inbounds %struct.ethhdr, ptr %0, i64 0, i32 2`
  `%8 = load i16, ptr %7, align 1, !tbaa !5`
  `%9 = tail call i16 @llvm.bswap.i16(i16 %8)`
  `%10 = getelementptr inbounds %struct.metadata, ptr %4, i64 0, i32 10`
  `store i16 %9, ptr %10, align 2, !tbaa !10`
 */
extern match_type metadata_pattern_transfer_lhs_bswap_load_gep_rhs_gep;

#line 950 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches an operation of metadata transfer that is
  similar to the pattern 'metadata_pattern_transfer_lhs_load_rhs_gep', the
  difference being that a call to a byte swap intrinsic is performed and then
  the resulting SSA value is stored into the destination.
  This pattern would match the following LLVM block:
 
  `%7 = load i32, ptr %0, align 4, !tbaa !40`
  `%8 = tail call i32 @llvm.bswap.i32(i32 %7)`
  `%9 = getelementptr inbounds %struct.anon.10, ptr %4, i64 0, i32 1`
  `store i32 %8, ptr %9, align 4, !tbaa !41`
 */
extern match_type metadata_pattern_transfer_lhs_bsawp_load_rhs_gep;

#line 1007 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches an operation of metadata transfer that is
  performed by a load from an offset of the memory region pointed by the first
  argument of the function. Then a byte swap intrinsic is called and the
  resulting SSA value is stored at a memory region pointed by either the
  fourth or fifth arguments. The action will return an instance of 'struct
  xdp2gen::llvm::metadata_transfer' if successful.
  This pattern would match the following LLVM block:
 
  %7 = getelementptr inbounds %struct.tcp_opt_union, ptr %0, i64 0, i32 1
  %8 = load i32, ptr %7, align 1, !tbaa !18
  %9 = tail call i32 @llvm.bswap.i32(i32 %8)
  store i32 %9, ptr %3, align 8, !tbaa !28
 */
extern match_type metadata_pattern_transfer_lhs_bsawp_load_gep_rhs_arg;

#line 1066 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches an operation of metadata transfer that is
  performed by just loading a value from the memory region pointed by the
  first argument of the function, performing a call to a byte swap intrinsic
  and then storing the resulting SSA value into a memory region pointed by
  either the the fourth or fifth arguments. The action will return an instance
  of 'struct xdp2gen::llvm::metadata_transfer' if successful.
  This pattern would match the following LLVM block:
 
  `%7 = load i32, ptr %0, align 4, !tbaa !40`
  `%8 = tail call i32 @llvm.bswap.i32(i32 %7)`
  `store i32 %8, ptr %4, align 8, !tbaa !18`
 */
extern match_type metadata_pattern_transfer_lhs_bswap_load_rhs_arg;

#line 1112 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches an operation of metadata transfer that is
  similar to the pattern 'metadata_pattern_transfer_lhs_load_gep_rhs_gep', the
  difference being that a call to a 'memcpy' intrinsic is performed instead of
  a store operation.
  This pattern would match the following LLVM block:
 
  `%11 = getelementptr inbounds %struct.metadata, ptr %4, i64 0, i32 12`
  `%12 = getelementptr inbounds %struct.ipv6hdr, ptr %0, i64 0, i32 5`
  `tail call void @llvm.memcpy.p0.p0.i64(ptr noundef nonnull align 8
  `  dereferenceable(16) %11, ptr noundef nonnull align 4 dereferenceable(16)
  `  %12, i64 16, i1 false), !tbaa.struct !45`
 */
extern match_type metadata_pattern_transfer_memcpy_lhs_gep_rhs_gep;

#line 1180 "include/xdp2gen/llvm/patterns.h2"
// write constant
/* The following pattern matches an operation of metadata write constant that
  is performed by just storing a constant value into a destination pointer.
  The destination pointer must be calculated at an offset from either the
  fourth or fifth arguments. The action will return an instance of 'struct
  xdp2gen::llvm::metadata_write_constant' if successful.
  This pattern would match the following LLVM block:
 
  `%10 = getelementptr inbounds %struct.metadata, ptr %4, i64 0, i32 8`
  `store i8 1, ptr %10, align 8, !tbaa !17`
 */
extern match_type metadata_pattern_const;

#line 1229 "include/xdp2gen/llvm/patterns.h2"
// write header offset
/* The following pattern matches an operation of metadata write header offset
  that is performed by just storing the third argument of the function into a
  destination pointer. The destination pointer must be calculated at an offset
  from either the fourth or fifth arguments. The action will return an
  instance of 'struct xdp2gen::llvm::metadata_write_header_offset' if
  successful.
  This pattern would match the following LLVM block:
 
  `%20 = trunc i64 %2 to i16`
  `%21 = getelementptr inbounds %struct.metadata, ptr %4, i64 0, i32 6`
  `store i16 %20, ptr %21, align 1, !tbaa !21`
 */
extern match_type metadata_pattern_hdr_off;

#line 1285 "include/xdp2gen/llvm/patterns.h2"
// write header length
/* The following pattern matches an operation of metadata write header length
  that is performed by just storing the second argument of the function into a
  destination pointer. The destination pointer must be calculated at an offset
  from either the fourth or fifth arguments. The action will return an
  instance of 'struct xdp2gen::llvm::metadata_write_header_length' if
  successful.
  This pattern would match the following LLVM block:
 
  `%22 = trunc i64 %1 to i8`
  `%23 = getelementptr inbounds %struct.metadata, ptr %4, i64 0, i32 5`
  `store i8 %22, ptr %23, align 2, !tbaa !22`
 */
extern match_type metadata_pattern_hdr_len;

#line 1341 "include/xdp2gen/llvm/patterns.h2"
// value transfer
/* The following pattern matches an operation of metadata write value transfer
  that is performed by just storing the loaded value at an offset of the
  memory region pointer the sixth argument of the function into a destination
  pointer. The loaded value must either be the first, sixth or seventh member
  variables of the 'struct xdp2_ctrl_data' record, corresponding to "ret",
  "node_cnt" and "encap_levels", respectively. The destination pointer must be
  calculated at an offset from either the fourth or fifth arguments. The
  action will return an instance of 'struct
  xdp2gen::llvm::metadata_value_transfer' if successful.
 */
extern match_type metadata_pattern_value_transfer_lhs_load_gep_rhs_gep;

#line 1415 "include/xdp2gen/llvm/patterns.h2"
        // elements from struct xdp2_ctrl_data

#line 1426 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches an operation of metadata write value transfer
  that is performed by just storing the loaded value of the memory region
  pointed by the sixth argument of the function into a destination pointer,
  that corresponds to the "ret" member variable of the 'struct
  xdp2_ctrl_data' record. The destination pointer must be calculated at an
  offset from either the fourth or fifth arguments. The action will return an
  instance of 'struct xdp2gen::llvm::metadata_value_transfer' if successful.
 */
extern match_type metadata_pattern_value_transfer_ret_code;

#line 1472 "include/xdp2gen/llvm/patterns.h2"
/* The following pattern matches an operation of metadata write value transfer
  that is very similar to the
  'metadata_pattern_value_transfer_lhs_load_gep_rhs_gep' pattern, the only
  difference being that no offset is calculated for the destination pointer.
  The action will return an instance of 'struct
  xdp2gen::llvm::metadata_value_transfer' if successful.
 */
extern match_type metadata_pattern_value_transfer_lhs_load_gep_rhs_arg;

//=== Cpp2 function definitions =================================================


#line 55 "include/xdp2gen/llvm/patterns.h2"
match_type tlv_pattern_load_gep {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 5>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &return_)

#line 58 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::ReturnInst>(return_); 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &return_, const graph_attrs &zext, const graph_attrs &load, const graph_attrs &get_element_ptr, const graph_attrs &argument) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 

        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto load_size {xdp2gen::llvm::get_integer_size(load)}; 

        return xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
            CPP2_UFCS_0_NONLOCAL(getSExtValue, offset) << 3, load_size, size_t(1ul << load_size) - 1, 0
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &zext)
    {
        return ::llvm::isa<::llvm::ZExtInst>(zext); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(2, 3);pat.add_attrs(3, [] (graph_attrs const &get_element_ptr)
    {

        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr); 
    });pat.add_edge(3, 4);pat.add_attrs(4, [] (graph_attrs const &argument)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pattern_edges_map.insert({{0, 1}, {1, std::nullopt}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});pattern_edges_map.insert({{3, 4}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 100 "include/xdp2gen/llvm/patterns.h2"
match_type tlv_pattern_load {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 4>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &return_)

#line 103 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::ReturnInst>(return_); 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &return_, const graph_attrs &zext, const graph_attrs &load, const graph_attrs &argument) -> std::any{
        return xdp2gen::llvm::packet_buffer_load(
            xdp2gen::llvm::get_integer_size(load)
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &zext)
    {
        return ::llvm::isa<::llvm::ZExtInst>(zext); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(2, 3);pat.add_attrs(3, [] (graph_attrs const &argument)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pattern_edges_map.insert({{0, 1}, {1, std::nullopt}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 128 "include/xdp2gen/llvm/patterns.h2"
match_type tlv_pattern_const {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 2>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &return_)

#line 131 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::ReturnInst>(return_); 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &return_, const graph_attrs &constant) -> std::any{
        auto v {::llvm::dyn_cast<::llvm::Constant>(constant)}; 
        auto const_val {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(v))))}; 

        return xdp2gen::llvm::constant_value(
            const_val, 
            xdp2gen::llvm::get_integer_size(v)
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &constant)
    {
        return ::llvm::isa<::llvm::Constant>(constant); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 160 "include/xdp2gen/llvm/patterns.h2"
match_type proto_next_pattern_load_gep {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 5>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &return_)

#line 163 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::ReturnInst>(return_); 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &return_, const graph_attrs &zext, const graph_attrs &load, const graph_attrs &get_element_ptr, const graph_attrs &argument) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 

        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto load_size {xdp2gen::llvm::get_integer_size(load)}; 

        return xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
            CPP2_UFCS_0_NONLOCAL(getSExtValue, offset) << 3, load_size, size_t(1ul << load_size) - 1, 0, 0
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &zext)
    {
        return ::llvm::isa<::llvm::ZExtInst>(zext); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(2, 3);pat.add_attrs(3, [] (graph_attrs const &get_element_ptr)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr); 
    });pat.add_edge(3, 4);pat.add_attrs(4, [] (graph_attrs const &argument)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});pattern_edges_map.insert({{3, 4}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 206 "include/xdp2gen/llvm/patterns.h2"
match_type proto_next_pattern_shift_load {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 6>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &return_)

#line 210 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::ReturnInst>(return_); 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &return_, const graph_attrs &zext, const graph_attrs &shift, const graph_attrs &load, const graph_attrs &argument, const graph_attrs &shift_rhs) -> std::any{
        size_t bit_size {xdp2gen::llvm::get_integer_size(load)}; 
        size_t bit_mask {(1ul << (bit_size + 1)) - 1}; 

        auto rhs_const {::llvm::dyn_cast<::llvm::Constant>(shift_rhs)}; 
        auto sign {1}; 
        auto bo {::llvm::dyn_cast<::llvm::BinaryOperator>(shift)}; 
        if (CPP2_UFCS_0_NONLOCAL(getOpcode, (*cpp2::assert_not_null(bo))) == ::llvm::Instruction::Shl) {
            sign = -1;
        }
        int64_t shift_right {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(rhs_const)))) * sign}; 

        if (cpp2::cmp_less(shift_right,0)) {
            return xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
                0, bit_size, bit_mask, 1ul << (-shift_right)
            ); 
        }else {
            return xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
                0, bit_size, bit_mask, 1, shift_right
            ); 
        }
    }}});pat.add_attrs(1, [] (graph_attrs const &zext)
    {
        return ::llvm::isa<::llvm::ZExtInst>(zext); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &shift)
    {
        auto bo {::llvm::dyn_cast<::llvm::BinaryOperator>(shift)}; 
        if (!(bo)) {
            return false; 
        }
        auto opcode {CPP2_UFCS_0(getOpcode, (*cpp2::assert_not_null(bo)))}; 
        return opcode == ::llvm::Instruction::Shl 
            || opcode == ::llvm::Instruction::LShr; 
    });pat.add_edge(2, 3);pat.add_edge(2, 5);pat.add_attrs(3, [] (graph_attrs const &load)

#line 249 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(3, 4);pat.add_attrs(4, [] (graph_attrs const &argument)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pat.add_attrs(5, [] (graph_attrs const &shift_rhs){return ::llvm::isa<::llvm::Constant>(shift_rhs); });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});pattern_edges_map.insert({{2, 5}, {1, 1}});pattern_edges_map.insert({{3, 4}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4]), get_attrs(g, mat[5])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 263 "include/xdp2gen/llvm/patterns.h2"
match_type proto_next_pattern_const {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 2>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &return_)

#line 266 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::ReturnInst>(return_); 
    });pat.add_edge(0, 1);pat.add_attrs(1, [] (graph_attrs const &constant)
    {
        return ::llvm::isa<::llvm::Constant>(constant); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 291 "include/xdp2gen/llvm/patterns.h2"
match_type proto_next_pattern_cond_mask_load {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 10>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &return_)

#line 298 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::ReturnInst>(return_); 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &return_, const graph_attrs &select, const graph_attrs &icmp, const graph_attrs &binop_and, const graph_attrs &load, const graph_attrs &argument, const graph_attrs &select_lhs, const graph_attrs &select_rhs, const graph_attrs &icmp_rhs, const graph_attrs &binop_and_rhs) -> std::any{
        auto load_size {xdp2gen::llvm::get_integer_size(load)}; 

        auto rhs_const {::llvm::dyn_cast<::llvm::Constant>(binop_and_rhs)}; 
        auto rhs_const_size {xdp2gen::llvm::get_integer_size(rhs_const)}; 
        auto apint {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(rhs_const))))}; 
        int64_t value_bit_size_mask {(1l << rhs_const_size) - 1}; 
        auto mask_value {apint & value_bit_size_mask}; 
        auto corrected_mask {size_t(mask_value)}; 

        auto arr {std::bit_cast<std::array<std::byte,sizeof(corrected_mask)>>(corrected_mask)}; 
        std::reverse(CPP2_UFCS_0_NONLOCAL(begin, arr), CPP2_UFCS_0_NONLOCAL(end, arr));
        corrected_mask = std::bit_cast<decltype(corrected_mask)>(arr);

        corrected_mask = 
            corrected_mask >> 
                (((sizeof(corrected_mask) << 3) / rhs_const_size) - 1) * 
                    rhs_const_size;

        return xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
            0, load_size, corrected_mask, 0, 0
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &select)
    {
        return ::llvm::isa<::llvm::SelectInst>(select); 
    });pat.add_edge(1, 2);pat.add_edge(1, 6);pat.add_edge(1, 7);pattern_nodes_action_map.insert({1, graph_attrs_action{[] (const graph_attrs &return_, const graph_attrs &select, const graph_attrs &icmp, const graph_attrs &binop_and, const graph_attrs &load, const graph_attrs &argument, const graph_attrs &select_lhs, const graph_attrs &select_rhs, const graph_attrs &icmp_rhs, const graph_attrs &binop_and_rhs) -> std::any{
        xdp2gen::llvm::condition cond {}; 

        auto icmp_inst {::llvm::dyn_cast<::llvm::ICmpInst>(icmp)}; 
        cond.comparison_op = CPP2_UFCS_0_NONLOCAL(getPredicate, (*cpp2::assert_not_null(icmp_inst)));

        auto rhs_const {::llvm::dyn_cast<::llvm::Constant>(select_rhs)}; 
        auto const_value {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(rhs_const))))}; 
        auto const_bit_size {xdp2gen::llvm::get_integer_size(rhs_const)}; 
        cond.default_fail = xdp2gen::llvm::constant_value(const_value, const_bit_size);

        rhs_const = ::llvm::dyn_cast<::llvm::Constant>(icmp_rhs);
        const_value = CPP2_UFCS_0_NONLOCAL(getSExtValue, CPP2_UFCS_0_NONLOCAL(getUniqueInteger, (*cpp2::assert_not_null(rhs_const))));
        const_bit_size = xdp2gen::llvm::get_integer_size(rhs_const);
        cond.rhs = xdp2gen::llvm::constant_value(const_value, const_bit_size);

        auto load_size {xdp2gen::llvm::get_integer_size(load)}; 
        rhs_const = ::llvm::dyn_cast<::llvm::Constant>(binop_and_rhs);
        auto rhs_const_size {xdp2gen::llvm::get_integer_size(rhs_const)}; 
        auto apint {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(rhs_const))))}; 
        int64_t value_bit_size_mask {(1l << rhs_const_size) - 1}; 
        auto mask_value {apint & value_bit_size_mask}; 
        auto corrected_mask {size_t(mask_value)}; 
        auto arr {std::bit_cast<std::array<std::byte,sizeof(corrected_mask)>>(corrected_mask)}; 
        std::reverse(CPP2_UFCS_0_NONLOCAL(begin, arr), CPP2_UFCS_0_NONLOCAL(end, arr));
        corrected_mask = std::bit_cast<decltype(corrected_mask)>(arr);
        corrected_mask = 
            corrected_mask >> 
                (((sizeof(corrected_mask) << 3) / rhs_const_size) - 1) * 
                    rhs_const_size;
        cond.lhs = xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
            0, load_size, corrected_mask, 0, 0
        );

        return cond; 
    }}});pat.add_attrs(2, [] (graph_attrs const &icmp)

#line 367 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::ICmpInst>(icmp); 
    });pat.add_edge(2, 3);pat.add_edge(2, 8);pat.add_attrs(3, [] (graph_attrs const &binop_and)

#line 373 "include/xdp2gen/llvm/patterns.h2"
    {
        auto bo {::llvm::dyn_cast<::llvm::BinaryOperator>(binop_and)}; 
        return bo && CPP2_UFCS_0_NONLOCAL(getOpcode, (*cpp2::assert_not_null(bo))) == ::llvm::Instruction::And; 
    });pat.add_edge(3, 4);pat.add_edge(3, 9);pat.add_attrs(4, [] (graph_attrs const &load)

#line 380 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(4, 5);pat.add_attrs(5, [] (graph_attrs const &argument)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pat.add_attrs(6, [] (graph_attrs const &select_lhs){return ::llvm::isa<::llvm::Constant>(select_lhs); });pat.add_attrs(7, [] (graph_attrs const &select_rhs){return ::llvm::isa<::llvm::Constant>(select_rhs); });pat.add_attrs(8, [] (graph_attrs const &icmp_rhs){return ::llvm::isa<::llvm::Constant>(icmp_rhs); });pat.add_attrs(9, [] (graph_attrs const &binop_and_rhs){return ::llvm::isa<::llvm::Constant>(binop_and_rhs); });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{1, 6}, {1, 1}});pattern_edges_map.insert({{1, 7}, {1, 2}});pattern_edges_map.insert({{2, 3}, {1, 0}});pattern_edges_map.insert({{2, 8}, {1, 1}});pattern_edges_map.insert({{3, 4}, {1, 0}});pattern_edges_map.insert({{3, 9}, {1, 1}});pattern_edges_map.insert({{4, 5}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4]), get_attrs(g, mat[5]), get_attrs(g, mat[6]), get_attrs(g, mat[7]), get_attrs(g, mat[8]), get_attrs(g, mat[9])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 401 "include/xdp2gen/llvm/patterns.h2"
match_type proto_next_pattern_mask_shift_load {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 8>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &return_)

#line 406 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::ReturnInst>(return_); 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &return_, const graph_attrs &zext, const graph_attrs &binop_and, const graph_attrs &shift, const graph_attrs &load, const graph_attrs &argument, const graph_attrs &binop_and_rhs, const graph_attrs &shift_rhs) -> std::any{
        auto rhs_const {::llvm::dyn_cast<::llvm::Constant>(binop_and_rhs)}; 
        size_t bit_size {xdp2gen::llvm::get_integer_size(rhs_const)}; 
        int64_t apint {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(rhs_const))))}; 
        int64_t value_bit_size_mask {(1ll << bit_size) - 1}; 
        int64_t mask_value {apint & value_bit_size_mask}; 

        rhs_const = ::llvm::dyn_cast<::llvm::Constant>(shift_rhs);
        auto sign {1}; 
        auto bo {::llvm::dyn_cast<::llvm::BinaryOperator>(shift)}; 
        if (CPP2_UFCS_0_NONLOCAL(getOpcode, (*cpp2::assert_not_null(bo))) == ::llvm::Instruction::Shl) {
            sign = -1;
        }
        int64_t shift_right {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(rhs_const)))) * sign}; 

        size_t bit_mask {0}; 
        if (cpp2::cmp_less(shift_right,0)) {
            bit_mask = mask_value >> (-shift_right);
        }else {
            bit_mask = mask_value << shift_right;
        }

        auto corrected_mask {bit_mask}; 
        auto size_bytes {bit_size >> 3}; 
        if (cpp2::cmp_greater(size_bytes,1ul)) {
            auto arr {std::bit_cast<std::array<std::byte,sizeof(corrected_mask)>>(corrected_mask)}; 
            std::reverse(CPP2_UFCS_0_NONLOCAL(begin, arr), CPP2_UFCS_0_NONLOCAL(begin, arr) + size_bytes);
            corrected_mask = std::bit_cast<decltype(corrected_mask)>(arr);
        }
        if (cpp2::cmp_less(shift_right,0)) {
            return xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
                0, bit_size, corrected_mask, 1ul << (-shift_right)
            ); 
        }else {
            return xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
                0, bit_size, corrected_mask, 1, shift_right
            ); 
        }
    }}});pat.add_attrs(1, [] (graph_attrs const &zext)
    {
        return ::llvm::isa<::llvm::ZExtInst>(zext); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &binop_and)
    {
        auto bo {::llvm::dyn_cast<::llvm::BinaryOperator>(binop_and)}; 
        return bo && CPP2_UFCS_0_NONLOCAL(getOpcode, (*cpp2::assert_not_null(bo))) == ::llvm::Instruction::And; 
    });pat.add_edge(2, 3);pat.add_edge(2, 6);pat.add_attrs(3, [] (graph_attrs const &shift)

#line 457 "include/xdp2gen/llvm/patterns.h2"
    {
        auto bo {::llvm::dyn_cast<::llvm::BinaryOperator>(shift)}; 
        if (!(bo)) {
            return false; 
        }
        auto opcode {CPP2_UFCS_0(getOpcode, (*cpp2::assert_not_null(bo)))}; 
        return opcode == ::llvm::Instruction::Shl 
            || opcode == ::llvm::Instruction::LShr; 
    });pat.add_edge(3, 4);pat.add_edge(3, 7);pat.add_attrs(4, [] (graph_attrs const &load)

#line 469 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(4, 5);pat.add_attrs(5, [] (graph_attrs const &argument)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pat.add_attrs(6, [] (graph_attrs const &binop_and_rhs){return ::llvm::isa<::llvm::Constant>(binop_and_rhs); });pat.add_attrs(7, [] (graph_attrs const &shift_rhs){return ::llvm::isa<::llvm::Constant>(shift_rhs); });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});pattern_edges_map.insert({{2, 6}, {1, 1}});pattern_edges_map.insert({{3, 4}, {1, 0}});pattern_edges_map.insert({{3, 7}, {1, 1}});pattern_edges_map.insert({{4, 5}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4]), get_attrs(g, mat[5]), get_attrs(g, mat[6]), get_attrs(g, mat[7])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 491 "include/xdp2gen/llvm/patterns.h2"
match_type proto_next_pattern_mask_shift_gep {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 9>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &return_)

#line 496 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::ReturnInst>(return_); 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &return_, const graph_attrs &zext, const graph_attrs &binop_and, const graph_attrs &shift, const graph_attrs &load, const graph_attrs &get_element_ptr, const graph_attrs &argument, const graph_attrs &binop_and_rhs, const graph_attrs &shift_rhs) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        auto rhs_const {::llvm::dyn_cast<::llvm::Constant>(binop_and_rhs)}; 
        size_t bit_size {xdp2gen::llvm::get_integer_size(rhs_const)}; 
        int64_t apint {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(rhs_const))))}; 
        int64_t value_bit_size_mask {(1ll << bit_size) - 1}; 
        int64_t mask_value {apint & value_bit_size_mask}; 

        rhs_const = ::llvm::dyn_cast<::llvm::Constant>(shift_rhs);
        auto sign {1}; 
        auto bo {::llvm::dyn_cast<::llvm::BinaryOperator>(shift)}; 
        if (CPP2_UFCS_0_NONLOCAL(getOpcode, (*cpp2::assert_not_null(bo))) == ::llvm::Instruction::Shl) {
            sign = -1;
        }
        int64_t shift_right {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(rhs_const)))) * sign}; 

        size_t bit_mask {0}; 
        if (cpp2::cmp_less(shift_right,0)) {
            bit_mask = mask_value >> (-shift_right);
        }else {
            bit_mask = mask_value << shift_right;
        }

        auto corrected_mask {bit_mask}; 
        auto size_bytes {bit_size >> 3}; 
        if (cpp2::cmp_greater(size_bytes,1ul)) {
            auto arr {std::bit_cast<std::array<std::byte,sizeof(corrected_mask)>>(corrected_mask)}; 
            std::reverse(CPP2_UFCS_0_NONLOCAL(begin, arr), CPP2_UFCS_0_NONLOCAL(begin, arr) + size_bytes);
            corrected_mask = std::bit_cast<decltype(corrected_mask)>(arr);
        }
        if (cpp2::cmp_less(shift_right,0)) {
            return xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
                bit_offset, bit_size, corrected_mask, 1ul << (-shift_right)
            ); 
        }else {
            return xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
                bit_offset, bit_size, corrected_mask, 1, shift_right
            ); 
        }
    }}});pat.add_attrs(1, [] (graph_attrs const &zext)
    {
        return ::llvm::isa<::llvm::ZExtInst>(zext); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &binop_and)
    {
        auto bo {::llvm::dyn_cast<::llvm::BinaryOperator>(binop_and)}; 
        return bo && CPP2_UFCS_0_NONLOCAL(getOpcode, (*cpp2::assert_not_null(bo))) == ::llvm::Instruction::And; 
    });pat.add_edge(2, 3);pat.add_edge(2, 7);pat.add_attrs(3, [] (graph_attrs const &shift)

#line 556 "include/xdp2gen/llvm/patterns.h2"
    {
        auto bo {::llvm::dyn_cast<::llvm::BinaryOperator>(shift)}; 
        if (!(bo)) {
            return false; 
        }
        auto opcode {CPP2_UFCS_0(getOpcode, (*cpp2::assert_not_null(bo)))}; 
        return opcode == ::llvm::Instruction::Shl 
            || opcode == ::llvm::Instruction::LShr; 
    });pat.add_edge(3, 4);pat.add_edge(3, 8);pat.add_attrs(4, [] (graph_attrs const &load)

#line 568 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(4, 5);pat.add_attrs(5, [] (graph_attrs const &get_element_ptr)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr); 
    });pat.add_edge(5, 6);pat.add_attrs(6, [] (graph_attrs const &argument)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pat.add_attrs(7, [] (graph_attrs const &binop_and_rhs){return ::llvm::isa<::llvm::Constant>(binop_and_rhs); });pat.add_attrs(8, [] (graph_attrs const &shift_rhs){return ::llvm::isa<::llvm::Constant>(shift_rhs); });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});pattern_edges_map.insert({{2, 7}, {1, 1}});pattern_edges_map.insert({{3, 4}, {1, 0}});pattern_edges_map.insert({{3, 8}, {1, 1}});pattern_edges_map.insert({{4, 5}, {1, 0}});pattern_edges_map.insert({{5, 6}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4]), get_attrs(g, mat[5]), get_attrs(g, mat[6]), get_attrs(g, mat[7]), get_attrs(g, mat[8])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 589 "include/xdp2gen/llvm/patterns.h2"
match_type proto_next_pattern_load {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 4>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &return_)

#line 592 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::ReturnInst>(return_); 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &return_, const graph_attrs &zext, const graph_attrs &load, const graph_attrs &argument) -> std::any{
        size_t bit_size {xdp2gen::llvm::get_integer_size(load)}; 
        size_t bit_mask {(1ul << bit_size) - 1}; 

        return xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
            0, bit_size, bit_mask, 0, 0
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &zext)
    {
        return ::llvm::isa<::llvm::ZExtInst>(zext); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(2, 3);pat.add_attrs(3, [] (graph_attrs const &argument)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 639 "include/xdp2gen/llvm/patterns.h2"
match_type proto_next_pattern_branch_mask_load_gep {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 15>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &return_)

#line 649 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::ReturnInst>(return_); 
    });pat.add_edge(0, 1);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &return_, const graph_attrs &phi, const graph_attrs &branch1, const graph_attrs &zext, const graph_attrs &load1, const graph_attrs &get_element_ptr1, const graph_attrs &argument, const graph_attrs &branch2, const graph_attrs &icmp, const graph_attrs &binop_and, const graph_attrs &load2, const graph_attrs &get_element_ptr2, const graph_attrs &icmp_rhs, const graph_attrs &binop_and_rhs, const graph_attrs &phi_constant) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr1)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 

        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto load_size {xdp2gen::llvm::get_integer_size(load1)}; 

        return xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
            CPP2_UFCS_0_NONLOCAL(getSExtValue, offset) << 3, load_size, size_t(1ul << load_size) - 1, 0, 0
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &phi)
    {
        auto pn {::llvm::dyn_cast<::llvm::PHINode>(phi)}; 
        return pn && CPP2_UFCS_0_NONLOCAL(getNumOperands, (*cpp2::assert_not_null(pn))) == 2; 
    });pat.add_edge(1, 2);pat.add_edge(1, 3);pat.add_edge(1, 7);pat.add_edge(1, 14);pattern_nodes_action_map.insert({1, graph_attrs_action{[] (const graph_attrs &return_, const graph_attrs &phi, const graph_attrs &branch1, const graph_attrs &zext, const graph_attrs &load1, const graph_attrs &get_element_ptr1, const graph_attrs &argument, const graph_attrs &branch2, const graph_attrs &icmp, const graph_attrs &binop_and, const graph_attrs &load2, const graph_attrs &get_element_ptr2, const graph_attrs &icmp_rhs, const graph_attrs &binop_and_rhs, const graph_attrs &phi_constant) -> std::any{
        xdp2gen::llvm::condition cond {}; 

        auto icmp_inst {::llvm::dyn_cast<::llvm::ICmpInst>(icmp)}; 
        cond.comparison_op = CPP2_UFCS_0_NONLOCAL(getPredicate, (*cpp2::assert_not_null(icmp_inst)));

        auto rhs_const {::llvm::dyn_cast<::llvm::Constant>(phi_constant)}; 
        auto const_value {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(rhs_const))))}; 
        auto const_bit_size {xdp2gen::llvm::get_integer_size(rhs_const)}; 
        cond.default_fail = xdp2gen::llvm::constant_value(const_value, const_bit_size);

        rhs_const = ::llvm::dyn_cast<::llvm::Constant>(icmp_rhs);
        const_value = CPP2_UFCS_0_NONLOCAL(getSExtValue, CPP2_UFCS_0_NONLOCAL(getUniqueInteger, (*cpp2::assert_not_null(rhs_const))));
        const_bit_size = xdp2gen::llvm::get_integer_size(rhs_const);
        cond.rhs = xdp2gen::llvm::constant_value(const_value, const_bit_size);

        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr2)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto bit_offset {CPP2_UFCS_0(getSExtValue, offset) << 3}; 
        auto load_size {xdp2gen::llvm::get_integer_size(load2)}; 
        rhs_const = ::llvm::dyn_cast<::llvm::Constant>(binop_and_rhs);
        auto rhs_const_size {xdp2gen::llvm::get_integer_size(rhs_const)}; 
        auto apint {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(rhs_const))))}; 
        int64_t value_bit_size_mask {(1l << rhs_const_size) - 1}; 
        auto mask_value {apint & value_bit_size_mask}; 
        auto corrected_mask {size_t(mask_value)}; 
        auto arr {std::bit_cast<std::array<std::byte,sizeof(corrected_mask)>>(corrected_mask)}; 
        std::reverse(CPP2_UFCS_0_NONLOCAL(begin, arr), CPP2_UFCS_0_NONLOCAL(end, arr));
        corrected_mask = std::bit_cast<decltype(corrected_mask)>(arr);
        corrected_mask = 
            corrected_mask >> 
                (((sizeof(corrected_mask) << 3) / rhs_const_size) - 1) * 
                    rhs_const_size;
        cond.lhs = xdp2gen::llvm::packet_buffer_offset_masked_multiplied(
            bit_offset, load_size, corrected_mask, 0, 0
        );

        return cond; 
    }}});pat.add_attrs(2, [] (graph_attrs const &branch1)
    {
        auto b {::llvm::dyn_cast<::llvm::BranchInst>(branch1)}; 
        return b && CPP2_UFCS_0_NONLOCAL(isUnconditional, (*cpp2::assert_not_null(b))); 
    });pat.add_attrs(3, [] (graph_attrs const &zext)
    {
        return ::llvm::isa<::llvm::ZExtInst>(zext); 
    });pat.add_edge(3, 4);pat.add_attrs(4, [] (graph_attrs const &load1)
    {
        return ::llvm::isa<::llvm::LoadInst>(load1); 
    });pat.add_edge(4, 5);pat.add_attrs(5, [] (graph_attrs const &get_element_ptr1)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr1); 
    });pat.add_edge(5, 6);pat.add_attrs(6, [] (graph_attrs const &argument)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pat.add_attrs(7, [] (graph_attrs const &branch2)

    {
        auto b {::llvm::dyn_cast<::llvm::BranchInst>(branch2)}; 
        return b && CPP2_UFCS_0_NONLOCAL(isConditional, (*cpp2::assert_not_null(b))); 
    });pat.add_edge(7, 8);pat.add_attrs(8, [] (graph_attrs const &icmp)
    {
        return ::llvm::isa<::llvm::ICmpInst>(icmp); 
    });pat.add_edge(8, 9);pat.add_edge(8, 12);pat.add_attrs(9, [] (graph_attrs const &binop_and)

#line 741 "include/xdp2gen/llvm/patterns.h2"
    {
        auto bo {::llvm::dyn_cast<::llvm::BinaryOperator>(binop_and)}; 
        return bo && CPP2_UFCS_0_NONLOCAL(getOpcode, (*cpp2::assert_not_null(bo))) == ::llvm::Instruction::And; 
    });pat.add_edge(9, 10);pat.add_edge(9, 13);pat.add_attrs(10, [] (graph_attrs const &load2)

#line 748 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::LoadInst>(load2); 
    });pat.add_edge(10, 11);pat.add_attrs(11, [] (graph_attrs const &get_element_ptr2)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr2); 
    });pat.add_edge(11, 6);pat.add_attrs(12, [] (graph_attrs const &icmp_rhs){return ::llvm::isa<::llvm::Constant>(icmp_rhs); });pat.add_attrs(13, [] (graph_attrs const &binop_and_rhs){return ::llvm::isa<::llvm::Constant>(binop_and_rhs); });pat.add_attrs(14, [] (graph_attrs const &phi_constant)
    {
        return ::llvm::isa<::llvm::Constant>(phi_constant); 
    });pattern_edges_map.insert({{0, 1}, {1, std::nullopt}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{1, 3}, {1, 1}});pattern_edges_map.insert({{1, 7}, {1, 2}});pattern_edges_map.insert({{1, 14}, {1, 3}});pattern_edges_map.insert({{3, 4}, {1, 0}});pattern_edges_map.insert({{4, 5}, {1, 0}});pattern_edges_map.insert({{5, 6}, {1, 0}});pattern_edges_map.insert({{7, 8}, {1, 0}});pattern_edges_map.insert({{8, 9}, {1, 0}});pattern_edges_map.insert({{8, 12}, {1, 1}});pattern_edges_map.insert({{9, 10}, {1, std::nullopt}});pattern_edges_map.insert({{9, 13}, {1, std::nullopt}});pattern_edges_map.insert({{10, 11}, {1, 0}});pattern_edges_map.insert({{11, 6}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4]), get_attrs(g, mat[5]), get_attrs(g, mat[6]), get_attrs(g, mat[7]), get_attrs(g, mat[8]), get_attrs(g, mat[9]), get_attrs(g, mat[10]), get_attrs(g, mat[11]), get_attrs(g, mat[12]), get_attrs(g, mat[13]), get_attrs(g, mat[14])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 775 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_transfer_lhs_load_gep_rhs_gep {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 6>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &store)

#line 779 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::StoreInst>(store); 
    });pat.add_edge(0, 1);pat.add_edge(0, 4);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &store, const graph_attrs &load, const graph_attrs &get_element_ptr1, const graph_attrs &argument1, const graph_attrs &get_element_ptr2, const graph_attrs &argument2) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr1)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto src_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        gep = ::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr2);
        offset = ::llvm::APInt(64, 0, false);
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto dst_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        size_t bit_size {xdp2gen::llvm::get_integer_size(load)}; 

        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        auto is_frame {CPP2_UFCS_0(getArgNo, (*cpp2::assert_not_null(arg))) == 4}; 

        return xdp2gen::llvm::metadata_transfer(
            src_bit_offset, dst_bit_offset, bit_size, is_frame, 
            std::nullopt, std::nullopt, std::nullopt, std::nullopt
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &get_element_ptr1)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr1); 
    });pat.add_edge(2, 3);pat.add_attrs(3, [] (graph_attrs const &argument1)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pat.add_attrs(4, [] (graph_attrs const &get_element_ptr2)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr2); 
    });pat.add_edge(4, 5);pat.add_attrs(5, [] (graph_attrs const &argument2)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 4}, {1, 1}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});pattern_edges_map.insert({{4, 5}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4]), get_attrs(g, mat[5])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 840 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_transfer_lhs_load_rhs_gep {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 5>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &store)

#line 844 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::StoreInst>(store); 
    });pat.add_edge(0, 1);pat.add_edge(0, 3);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &store, const graph_attrs &load, const graph_attrs &argument1, const graph_attrs &get_element_ptr, const graph_attrs &argument2) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto dst_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        size_t bit_size {xdp2gen::llvm::get_integer_size(load)}; 

        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        auto is_frame {CPP2_UFCS_0(getArgNo, (*cpp2::assert_not_null(arg))) == 4}; 

        return xdp2gen::llvm::metadata_transfer(
            0, dst_bit_offset, bit_size, is_frame, 
            std::nullopt, std::nullopt, std::nullopt, std::nullopt
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &argument1)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pat.add_attrs(3, [] (graph_attrs const &get_element_ptr)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr); 
    });pat.add_edge(3, 4);pat.add_attrs(4, [] (graph_attrs const &argument2)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 3}, {1, 1}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{3, 4}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 893 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_transfer_lhs_bswap_load_gep_rhs_gep {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 7>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &store)

#line 897 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::StoreInst>(store); 
    });pat.add_edge(0, 1);pat.add_edge(0, 5);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &store, const graph_attrs &call, const graph_attrs &load, const graph_attrs &get_element_ptr1, const graph_attrs &argument1, const graph_attrs &get_element_ptr2, const graph_attrs &argument2) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr1)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto src_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        gep = ::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr2);
        offset = ::llvm::APInt(64, 0, false);
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto dst_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        size_t bit_size {xdp2gen::llvm::get_integer_size(load)}; 

        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        auto is_frame {CPP2_UFCS_0(getArgNo, (*cpp2::assert_not_null(arg))) == 4}; 

        return xdp2gen::llvm::metadata_transfer(
            src_bit_offset, dst_bit_offset, bit_size, is_frame, 
            std::nullopt, std::nullopt, std::nullopt, true
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &call)
    {
        auto c {::llvm::dyn_cast<::llvm::CallInst>(call)}; 
        return c 
            && CPP2_UFCS_NONLOCAL(starts_with, CPP2_UFCS_0_NONLOCAL(getName, (*cpp2::assert_not_null(CPP2_UFCS_0_NONLOCAL(getCalledFunction, (*cpp2::assert_not_null(c)))))), "llvm.bswap"); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(2, 3);pat.add_attrs(3, [] (graph_attrs const &get_element_ptr1)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr1); 
    });pat.add_edge(3, 4);pat.add_attrs(4, [] (graph_attrs const &argument1)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pat.add_attrs(5, [] (graph_attrs const &get_element_ptr2)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr2); 
    });pat.add_edge(5, 6);pat.add_attrs(6, [] (graph_attrs const &argument2)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 5}, {1, 1}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});pattern_edges_map.insert({{3, 4}, {1, 0}});pattern_edges_map.insert({{5, 6}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4]), get_attrs(g, mat[5]), get_attrs(g, mat[6])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 961 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_transfer_lhs_bsawp_load_rhs_gep {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 6>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &store)

#line 965 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::StoreInst>(store); 
    });pat.add_edge(0, 1);pat.add_edge(0, 4);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &store, const graph_attrs &call, const graph_attrs &load, const graph_attrs &argument1, const graph_attrs &get_element_ptr, const graph_attrs &argument2) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto dst_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        size_t bit_size {xdp2gen::llvm::get_integer_size(load)}; 

        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        auto is_frame {CPP2_UFCS_0(getArgNo, (*cpp2::assert_not_null(arg))) == 4}; 

        return xdp2gen::llvm::metadata_transfer(
            0, dst_bit_offset, bit_size, is_frame, 
            std::nullopt, std::nullopt, std::nullopt, true
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &call)
    {
        auto c {::llvm::dyn_cast<::llvm::CallInst>(call)}; 
        return c 
            && CPP2_UFCS_NONLOCAL(starts_with, CPP2_UFCS_0_NONLOCAL(getName, (*cpp2::assert_not_null(CPP2_UFCS_0_NONLOCAL(getCalledFunction, (*cpp2::assert_not_null(c)))))), "llvm.bswap"); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(2, 3);pat.add_attrs(3, [] (graph_attrs const &argument1)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pat.add_attrs(4, [] (graph_attrs const &get_element_ptr)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr); 
    });pat.add_edge(4, 5);pat.add_attrs(5, [] (graph_attrs const &argument2)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 4}, {1, 1}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});pattern_edges_map.insert({{4, 5}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4]), get_attrs(g, mat[5])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 1020 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_transfer_lhs_bsawp_load_gep_rhs_arg {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 6>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &store)

#line 1024 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::StoreInst>(store); 
    });pat.add_edge(0, 1);pat.add_edge(0, 5);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &store, const graph_attrs &call, const graph_attrs &load, const graph_attrs &get_element_ptr, const graph_attrs &argument1, const graph_attrs &argument2) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto src_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        size_t bit_size {xdp2gen::llvm::get_integer_size(load)}; 

        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        auto is_frame {CPP2_UFCS_0(getArgNo, (*cpp2::assert_not_null(arg))) == 4}; 

        return xdp2gen::llvm::metadata_transfer(
            src_bit_offset, 0, bit_size, is_frame, 
            std::nullopt, std::nullopt, std::nullopt, true
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &call)
    {
        auto c {::llvm::dyn_cast<::llvm::CallInst>(call)}; 
        return c 
            && CPP2_UFCS_NONLOCAL(starts_with, CPP2_UFCS_0_NONLOCAL(getName, (*cpp2::assert_not_null(CPP2_UFCS_0_NONLOCAL(getCalledFunction, (*cpp2::assert_not_null(c)))))), "llvm.bswap"); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(2, 3);pat.add_attrs(3, [] (graph_attrs const &get_element_ptr)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr); 
    });pat.add_edge(3, 4);pat.add_attrs(4, [] (graph_attrs const &argument1)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pat.add_attrs(5, [] (graph_attrs const &argument2)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 5}, {1, 1}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});pattern_edges_map.insert({{3, 4}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4]), get_attrs(g, mat[5])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 1078 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_transfer_lhs_bswap_load_rhs_arg {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 5>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &store)

#line 1082 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::StoreInst>(store); 
    });pat.add_edge(0, 1);pat.add_edge(0, 4);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &store, const graph_attrs &call, const graph_attrs &load, const graph_attrs &argument1, const graph_attrs &argument2) -> std::any{
        size_t bit_size {xdp2gen::llvm::get_integer_size(load)}; 

        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        auto is_frame {CPP2_UFCS_0(getArgNo, (*cpp2::assert_not_null(arg))) == 4}; 

        return xdp2gen::llvm::metadata_transfer(
            0, 0, bit_size, is_frame, 
            std::nullopt, std::nullopt, std::nullopt, true
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &call)
    {
        auto c {::llvm::dyn_cast<::llvm::CallInst>(call)}; 
        return c 
            && CPP2_UFCS_NONLOCAL(starts_with, CPP2_UFCS_0_NONLOCAL(getName, (*cpp2::assert_not_null(CPP2_UFCS_0_NONLOCAL(getCalledFunction, (*cpp2::assert_not_null(c)))))), "llvm.bswap"); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(2, 3);pat.add_attrs(3, [] (graph_attrs const &argument1)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pat.add_attrs(4, [] (graph_attrs const &argument2)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 4}, {1, 1}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 1124 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_transfer_memcpy_lhs_gep_rhs_gep {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 6>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &call)

#line 1129 "include/xdp2gen/llvm/patterns.h2"
    {
        auto c {::llvm::dyn_cast<::llvm::CallInst>(call)}; 
        return c 
            && CPP2_UFCS_NONLOCAL(starts_with, CPP2_UFCS_0_NONLOCAL(getName, (*cpp2::assert_not_null(CPP2_UFCS_0_NONLOCAL(getCalledFunction, (*cpp2::assert_not_null(c)))))), "llvm.memcpy"); 
    });pat.add_edge(0, 1);pat.add_edge(0, 3);pat.add_edge(0, 5);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &call, const graph_attrs &get_element_ptr1, const graph_attrs &argument1, const graph_attrs &get_element_ptr2, const graph_attrs &argument2, const graph_attrs &memcpy_size) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr2)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto src_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        gep = ::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr1);
        offset = ::llvm::APInt(64, 0, false);
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto dst_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        auto arg2_const {::llvm::dyn_cast<::llvm::Constant>(memcpy_size)}; 
        auto bit_size {CPP2_UFCS_0(getZExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(arg2_const)))) << 3}; 

        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        auto is_frame {CPP2_UFCS_0(getArgNo, (*cpp2::assert_not_null(arg))) == 4}; 

        return xdp2gen::llvm::metadata_transfer(
            src_bit_offset, dst_bit_offset, bit_size, is_frame, 
            std::nullopt, std::nullopt, std::nullopt, std::nullopt
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &get_element_ptr1)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr1); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &argument1)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pat.add_attrs(3, [] (graph_attrs const &get_element_ptr2)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr2); 
    });pat.add_edge(3, 4);pat.add_attrs(4, [] (graph_attrs const &argument2)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 0; 
    });pat.add_attrs(5, [] (graph_attrs const &memcpy_size)
    {
        return ::llvm::isa<::llvm::Constant>(memcpy_size); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 3}, {1, 1}});pattern_edges_map.insert({{0, 5}, {1, 2}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{3, 4}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4]), get_attrs(g, mat[5])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 1191 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_const {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 4>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &store)

#line 1195 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::StoreInst>(store); 
    });pat.add_edge(0, 1);pat.add_edge(0, 2);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &store, const graph_attrs &constant, const graph_attrs &get_element_ptr, const graph_attrs &argument) -> std::any{
        auto const_store {::llvm::dyn_cast<::llvm::Constant>(constant)}; 
        auto const_value {CPP2_UFCS_0(getZExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(const_store))))}; 
        auto const_size {xdp2gen::llvm::get_integer_size(const_store)}; 

        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto bit_offset {CPP2_UFCS_0(getSExtValue, offset) << 3}; 

        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument)}; 
        auto is_frame {CPP2_UFCS_0(getArgNo, (*cpp2::assert_not_null(arg))) == 4}; 

        return xdp2gen::llvm::metadata_write_constant(
            const_value, const_size, bit_offset, is_frame, std::nullopt, std::nullopt, std::nullopt
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &constant)
    {
        return ::llvm::isa<::llvm::Constant>(constant); 
    });pat.add_attrs(2, [] (graph_attrs const &get_element_ptr)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr); 
    });pat.add_edge(2, 3);pat.add_attrs(3, [] (graph_attrs const &argument)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 2}, {1, 1}});pattern_edges_map.insert({{2, 3}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 1242 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_hdr_off {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 5>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &store)

#line 1246 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::StoreInst>(store); 
    });pat.add_edge(0, 1);pat.add_edge(0, 3);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &store, const graph_attrs &trunc, const graph_attrs &argument1, const graph_attrs &get_element_ptr, const graph_attrs &argument2) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto bit_offset {CPP2_UFCS_0(getSExtValue, offset) << 3}; 

        auto src_ty {CPP2_UFCS_0(getSourceElementType, (*cpp2::assert_not_null(gep)))}; 
        auto ops {::llvm::SmallVector<::llvm::Value*>(CPP2_UFCS_0(idx_begin, (*cpp2::assert_not_null(gep))), CPP2_UFCS_0(idx_end, (*cpp2::assert_not_null(gep))))}; 
        auto ty {::llvm::GetElementPtrInst::getIndexedType(src_ty, ops)}; 
        auto dl {CPP2_UFCS_0(getDataLayout, (*cpp2::assert_not_null(mod)))}; 
        auto bit_size {CPP2_UFCS(getTypeAllocSizeInBits, dl, ty)}; 

        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 

        return xdp2gen::llvm::metadata_write_header_offset(
            bit_offset, bit_size, 0, CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4, std::nullopt
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &trunc)
    {
        return ::llvm::isa<::llvm::TruncInst>(trunc); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &argument1)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 2; 
    });pat.add_attrs(3, [] (graph_attrs const &get_element_ptr)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr); 
    });pat.add_edge(3, 4);pat.add_attrs(4, [] (graph_attrs const &argument2)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 3}, {1, 1}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{3, 4}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 1298 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_hdr_len {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 5>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &store)

#line 1302 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::StoreInst>(store); 
    });pat.add_edge(0, 1);pat.add_edge(0, 3);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &store, const graph_attrs &trunc, const graph_attrs &argument1, const graph_attrs &get_element_ptr, const graph_attrs &argument2) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto bit_offset {CPP2_UFCS_0(getSExtValue, offset) << 3}; 

        auto src_ty {CPP2_UFCS_0(getSourceElementType, (*cpp2::assert_not_null(gep)))}; 
        auto ops {::llvm::SmallVector<::llvm::Value*>(CPP2_UFCS_0(idx_begin, (*cpp2::assert_not_null(gep))), CPP2_UFCS_0(idx_end, (*cpp2::assert_not_null(gep))))}; 
        auto ty {::llvm::GetElementPtrInst::getIndexedType(src_ty, ops)}; 
        auto dl {CPP2_UFCS_0(getDataLayout, (*cpp2::assert_not_null(mod)))}; 
        auto bit_size {CPP2_UFCS(getTypeAllocSizeInBits, dl, ty)}; 

        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 

        return xdp2gen::llvm::metadata_write_header_length(
            bit_offset, bit_size, 0, CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4, std::nullopt
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &trunc)
    {
        return ::llvm::isa<::llvm::TruncInst>(trunc); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &argument1)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 1; 
    });pat.add_attrs(3, [] (graph_attrs const &get_element_ptr)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr); 
    });pat.add_edge(3, 4);pat.add_attrs(4, [] (graph_attrs const &argument2)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 3}, {1, 1}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{3, 4}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 1352 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_value_transfer_lhs_load_gep_rhs_gep {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 7>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &store)

#line 1357 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::StoreInst>(store); 
    });pat.add_edge(0, 1);pat.add_edge(0, 5);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &store, const graph_attrs &load, const graph_attrs &get_element_ptr1, const graph_attrs &argument1, const graph_attrs &constant, const graph_attrs &get_element_ptr2, const graph_attrs &argument2) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr1)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto src_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        auto src_ty {CPP2_UFCS_0(getSourceElementType, (*cpp2::assert_not_null(gep)))}; 
        auto ops {::llvm::SmallVector<::llvm::Value*>(CPP2_UFCS_0(idx_begin, (*cpp2::assert_not_null(gep))), CPP2_UFCS_0(idx_end, (*cpp2::assert_not_null(gep))))}; 
        auto ty {::llvm::GetElementPtrInst::getIndexedType(src_ty, ops)}; 
        auto dl {CPP2_UFCS_0(getDataLayout, (*cpp2::assert_not_null(mod)))}; 
        size_t bit_size {CPP2_UFCS(getTypeAllocSizeInBits, dl, ty)}; 

        gep = ::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr2);
        offset = ::llvm::APInt(64, 0, false);
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto dst_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        auto c {::llvm::dyn_cast<::llvm::Constant>(constant)}; 
        auto val {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(c))))}; 
        std::string type {""}; 
        if (val == 5) {
            type = "num_nodes";
        }else {if (val == 6) {
            type = "num_encaps";
        }else {
            type = "return_code";
        }}

        return xdp2gen::llvm::metadata_value_transfer(
            src_bit_offset, dst_bit_offset, bit_size, 
            type, std::nullopt, std::nullopt, std::nullopt
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &get_element_ptr1)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr1); 
    });pat.add_edge(2, 3);pat.add_edge(2, 4);pat.add_attrs(3, [] (graph_attrs const &argument1)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 5; 
    });pat.add_attrs(4, [] (graph_attrs const &constant)
    {
        auto c {::llvm::dyn_cast<::llvm::Constant>(constant)}; 
        if (!(c)) {
            return false; 
        }
        auto val {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(c))))}; 

        return val == 0 || val == 5 || val == 6; 
    });pat.add_attrs(5, [] (graph_attrs const &get_element_ptr2)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr2); 
    });pat.add_edge(5, 6);pat.add_attrs(6, [] (graph_attrs const &argument2)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 5}, {1, 1}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});pattern_edges_map.insert({{2, 4}, {1, 2}});pattern_edges_map.insert({{5, 6}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4]), get_attrs(g, mat[5]), get_attrs(g, mat[6])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 1434 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_value_transfer_ret_code {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 5>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &store)

#line 1438 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::StoreInst>(store); 
    });pat.add_edge(0, 1);pat.add_edge(0, 3);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &store, const graph_attrs &load, const graph_attrs &argument1, const graph_attrs &get_element_ptr, const graph_attrs &argument2) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto dst_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        size_t bit_size {xdp2gen::llvm::get_integer_size(load)}; 

        return xdp2gen::llvm::metadata_value_transfer(
            0, dst_bit_offset, bit_size, "return_code", 
            std::nullopt, std::nullopt, std::nullopt
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &argument1)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 5; 
    });pat.add_attrs(3, [] (graph_attrs const &get_element_ptr)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr); 
    });pat.add_edge(3, 4);pat.add_attrs(4, [] (graph_attrs const &argument2)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 3}, {1, 1}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{3, 4}, {1, 0}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 

#line 1479 "include/xdp2gen/llvm/patterns.h2"
match_type metadata_pattern_value_transfer_lhs_load_gep_rhs_arg {[](auto const &g) -> std::vector<std::vector<std::tuple<size_t, std::any>>> requires cpp2::Graph<decltype(g)> {using graph_type = std::remove_cvref_t<decltype(g)>;using graph_attrs = decltype(get_attrs(g, 0));using graph_adj_list = decltype(get_adj_list(g, 0));using graph_attrs_pred = std::function<bool(graph_attrs const&)>;auto match = [](graph_attrs_pred const& pred, auto&& attrs){ return pred(attrs); };using graph_attrs_action = std::function<std::any(const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&, const graph_attrs&)>;auto pat = cpp2::fixed_size_pattern_graph<graph_type, 6>{};auto pattern_edges_map = std::map<std::tuple<size_t, size_t>, std::tuple<std::optional<size_t>, std::optional<long>>>{};auto pattern_nodes_action_map = std::unordered_map<size_t, graph_attrs_action>{}; pat.add_attrs(0, [] (graph_attrs const &store)

#line 1484 "include/xdp2gen/llvm/patterns.h2"
    {
        return ::llvm::isa<::llvm::StoreInst>(store); 
    });pat.add_edge(0, 1);pat.add_edge(0, 5);pattern_nodes_action_map.insert({0, graph_attrs_action{[] (const graph_attrs &store, const graph_attrs &load, const graph_attrs &get_element_ptr, const graph_attrs &argument1, const graph_attrs &constant, const graph_attrs &argument2) -> std::any{
        auto gep {::llvm::dyn_cast<::llvm::GetElementPtrInst>(get_element_ptr)}; 
        auto mod {CPP2_UFCS_0(getModule, (*cpp2::assert_not_null(gep)))}; 
        auto offset {::llvm::APInt(64, 0, false)}; 
        if (!(CPP2_UFCS_NONLOCAL(accumulateConstantOffset, (*cpp2::assert_not_null(gep)), CPP2_UFCS_0_NONLOCAL(getDataLayout, (*cpp2::assert_not_null(mod))), offset))) {
            std::cerr << "Could not get constant offset";
            return std::any(); 
        }
        auto src_bit_offset {size_t(CPP2_UFCS_0(getSExtValue, offset) << 3)}; 

        auto src_ty {CPP2_UFCS_0(getSourceElementType, (*cpp2::assert_not_null(gep)))}; 
        auto ops {::llvm::SmallVector<::llvm::Value*>(CPP2_UFCS_0(idx_begin, (*cpp2::assert_not_null(gep))), CPP2_UFCS_0(idx_end, (*cpp2::assert_not_null(gep))))}; 
        auto ty {::llvm::GetElementPtrInst::getIndexedType(src_ty, ops)}; 
        auto dl {CPP2_UFCS_0(getDataLayout, (*cpp2::assert_not_null(mod)))}; 
        auto bit_size {CPP2_UFCS(getTypeAllocSizeInBits, dl, ty)}; 

        auto c {::llvm::dyn_cast<::llvm::Constant>(constant)}; 
        auto val {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(c))))}; 
        std::string type {""}; 
        if (val == 5) {
            type = "num_nodes";
        }else {if (val == 6) {
            type = "num_encaps";
        }else {
            type = "return_code";
        }}

        return xdp2gen::llvm::metadata_value_transfer(
            src_bit_offset, 0, bit_size, 
            type, std::nullopt, std::nullopt, std::nullopt
        ); 
    }}});pat.add_attrs(1, [] (graph_attrs const &load)
    {
        return ::llvm::isa<::llvm::LoadInst>(load); 
    });pat.add_edge(1, 2);pat.add_attrs(2, [] (graph_attrs const &get_element_ptr)
    {
        return ::llvm::isa<::llvm::GetElementPtrInst>(get_element_ptr); 
    });pat.add_edge(2, 3);pat.add_edge(2, 4);pat.add_attrs(3, [] (graph_attrs const &argument1)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument1)}; 
        return arg && CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 5; 
    });pat.add_attrs(4, [] (graph_attrs const &constant)
    {
        auto c {::llvm::dyn_cast<::llvm::Constant>(constant)}; 
        if (!(c)) {
            return false; 
        }
        auto val {CPP2_UFCS_0(getSExtValue, CPP2_UFCS_0(getUniqueInteger, (*cpp2::assert_not_null(c))))}; 

        return val == 0 || val == 5 || val == 6; 
    });pat.add_attrs(5, [] (graph_attrs const &argument2)
    {
        auto arg {::llvm::dyn_cast<::llvm::Argument>(argument2)}; 
        return arg && (CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 3 || CPP2_UFCS_0_NONLOCAL(getArgNo, (*cpp2::assert_not_null(arg))) == 4); 
    });pattern_edges_map.insert({{0, 1}, {1, 0}});pattern_edges_map.insert({{0, 5}, {1, 1}});pattern_edges_map.insert({{1, 2}, {1, 0}});pattern_edges_map.insert({{2, 3}, {1, 0}});pattern_edges_map.insert({{2, 4}, {1, 2}});auto matcher = cpp2::vf2::vf2_matcher{g, pat, match};auto matches = matcher.match();auto filter_indexes = [&] (std::vector<size_t> const& mat) {    auto filter_edges = [](auto const &p) {        auto const [attr, index] = p.second;        return index && attr && *attr == 1;    };    for (auto const [edge, edge_attrs] :        std::views::filter(pattern_edges_map, filter_edges)) {        auto const [u, v] = edge;        auto const succ_u = get_adj_list(g, mat[u]);        auto index = *std::get<1>(edge_attrs);        if (index < 0) {            index += std::ssize(succ_u);        }        if (index < 0 || index >= std::ssize(succ_u)) {            return false;        }        auto const it = std::ranges::find(succ_u, mat[v]);        if (it != succ_u.end()) {            if (std::distance(succ_u.begin(), it) != index) {                return false;            }        }    }    return true;};auto S = std::vector<std::vector<std::tuple<size_t, std::any>>>{};for (auto const &mat : matches) {    if (!filter_indexes(mat)) {        continue;    }    size_t i = 0;    auto S_row = std::vector<std::tuple<size_t, std::any>>{};    for (const auto j : mat) {        if (            const auto it = pattern_nodes_action_map.find(i);            it != pattern_nodes_action_map.end()        ) {            S_row.emplace_back(j, it->second(get_attrs(g, mat[0]), get_attrs(g, mat[1]), get_attrs(g, mat[2]), get_attrs(g, mat[3]), get_attrs(g, mat[4]), get_attrs(g, mat[5])            ));        } else {            S_row.emplace_back(j, std::any{});        }        ++i;    }    S.push_back(std::move(S_row));}return S;}}; 
#endif
        // elements from struct xdp2_ctrl_data

