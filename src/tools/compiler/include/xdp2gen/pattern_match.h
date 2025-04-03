/* SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2024 SiXDP2 Inc.
 *
 * Authors: Felipe Magno de Almeida <felipe@expertise.dev>
 *          Otávio Lucas Alves da Silva <otavio.silva@expertisesolutions.com.br>          
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

#ifndef XDP2GEN_PATTERN_MATCH_H
#define XDP2GEN_PATTERN_MATCH_H

#include <vector>

#include "xdp2gen/llvm/patterns.h"
#include "xdp2gen/ast-consumer/patterns.h"
#include "xdp2gen/util.h"

namespace xdp2gen
{
template <typename match_type>
class pattern_match_factory {
    std::vector<match_type> patterns;

public:
    pattern_match_factory() = default;
    pattern_match_factory(pattern_match_factory const &) = default;
    pattern_match_factory(pattern_match_factory &&other)
        : patterns{ std::move(other.patterns) }
    {
    }
    pattern_match_factory(std::vector<match_type> &&ps)
        : patterns{ std::move(ps) }
    {
    }
    pattern_match_factory(std::initializer_list<match_type> ps)
        : patterns{ ps }
    {
    }

    void add_pattern(match_type const &m)
    {
        patterns.push_back(m);
    }

    template <typename... Ts, size_t... I, typename G>
    std::vector<std::variant<Ts...>>
    match_all_aux(G const &g, std::initializer_list<size_t> idxs,
                  std::index_sequence<I...>) const
    {
        // static_assert (sizeof...(Ts) == sizeof...(idx));
        std::vector<std::variant<Ts...>> ret;
        for (auto &&p : patterns) {
            auto const matches = p(g);
            for (auto &&match : matches) {
                ((std::any_cast<select_type_t<I, Ts...>>(
                      std::addressof(std::get<1>(match[*(idxs.begin() + I)]))) ?
                      ret.push_back(std::any_cast<select_type_t<I, Ts...>>(
                          std::get<1>(match[*(idxs.begin() + I)]))) :
                      (void)0),
                 ...);
            }
        }

        return ret;
    }

    template <typename... Ts, typename G>
    std::vector<std::variant<Ts...>>
    match_all(G const &g, std::initializer_list<size_t> idxs) const
    {
        assert(args_size_v<Ts...> == std::size(idxs));
        return match_all_aux<Ts...>(
            g, idxs, std::make_index_sequence<args_size_v<Ts...>>{});
    }
};

namespace llvm
{
pattern_match_factory const tlv_patterns{ tlv_pattern_load_gep,
                                          tlv_pattern_load, tlv_pattern_const };
pattern_match_factory const proto_next_patterns{
    proto_next_pattern_load_gep,
    proto_next_pattern_shift_load,
    proto_next_pattern_const,
    proto_next_pattern_cond_mask_load,
    proto_next_pattern_mask_shift_load,
    proto_next_pattern_mask_shift_gep,
    proto_next_pattern_load,
    proto_next_pattern_branch_mask_load_gep
};
pattern_match_factory const metadata_patterns{
    metadata_pattern_transfer_lhs_load_gep_rhs_gep,
    metadata_pattern_transfer_lhs_load_rhs_gep,
    metadata_pattern_transfer_lhs_bswap_load_gep_rhs_gep,
    metadata_pattern_transfer_lhs_bsawp_load_rhs_gep,
    metadata_pattern_transfer_lhs_bsawp_load_gep_rhs_arg,
    metadata_pattern_transfer_lhs_bswap_load_rhs_arg,
    metadata_pattern_transfer_memcpy_lhs_gep_rhs_gep,
    metadata_pattern_const,
    metadata_pattern_hdr_off,
    metadata_pattern_hdr_len,
    metadata_pattern_value_transfer_lhs_load_gep_rhs_gep,
    metadata_pattern_value_transfer_ret_code,
    metadata_pattern_value_transfer_lhs_load_gep_rhs_arg
};
}

namespace ast_consumer
{
pattern_match_factory const metadata_ast_patterns{ clang_ast_assignment_pattern,
                                                   clang_ast_increment_pattern,
                                                   clang_ast_call_pattern1,
                                                   clang_ast_call_pattern2 };
}

}

#endif
