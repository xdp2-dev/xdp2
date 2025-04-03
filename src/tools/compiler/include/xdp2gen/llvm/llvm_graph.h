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

#ifndef XDP2GEN_LLVM_LLVM_GRAPH_H
#define XDP2GEN_LLVM_LLVM_GRAPH_H

#include "xdp2gen/util.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Statepoint.h"

#include <boost/range.hpp>
#include <unordered_map>
#include <variant>
#include <vector>

namespace xdp2gen::llvm
{

class llvm_graph {
public:
    using node_type = ::llvm::Value const *;

private:
    ::llvm::BasicBlock const *bb_ptr = nullptr;

    static constexpr size_t npos = -1;
    size_t curr_size = 0;
    std::vector<node_type> index_node_map;
    std::unordered_map<node_type, size_t> ptr_index_map;
    std::vector<std::tuple<std::vector<size_t>, std::vector<size_t>>> adj_list;

    size_t __increase_graph(node_type const &n, node_type ptr)
    {
        index_node_map.push_back(n);
        ptr_index_map.insert({ ptr, curr_size });
        adj_list.push_back({ {}, {} });

        return curr_size++;
    }

    template <typename N>
    std::pair<node_type, size_t> __search_and_insert(N const *n)
        requires std::is_base_of_v<::llvm::Value, N> ||
                 std::is_same_v<::llvm::BasicBlock, N>
    {
        if (auto it = ptr_index_map.find(n); it != ptr_index_map.end()) {
            return *it;
        }
        auto idx = __increase_graph(n, n);

        return { n, idx };
    }

    bool __add_edge(size_t source, size_t target)
    {
        assert(source < curr_size && target < curr_size);
        std::get<0>(adj_list[source]).push_back(target);
        std::get<1>(adj_list[target]).push_back(source);
        /// TODO: maybe insert sorted to avoid repetition?
        return true;
    }

    template <typename N>
    size_t increase(N const *n, bool insert_adj = false)
        requires(!xdp2gen::one_of_v<N, ::llvm::Instruction, ::llvm::PHINode,
                                     ::llvm::BasicBlock, ::llvm::CallInst>)
    {
        auto [raw_ptr, idx] = __search_and_insert(n);
        if constexpr (std::is_base_of_v<::llvm::Instruction, N>) {
            if (insert_adj) {
                // size_t i = 0;
                for (auto &&op : n->operands()) {
                    if (auto *i = ::llvm::dyn_cast<::llvm::Instruction>(op)) {
                        __add_edge(idx, increase(i, insert_adj));
                    } else if (auto *c =
                                   ::llvm::dyn_cast<::llvm::Constant>(op)) {
                        __add_edge(idx, increase(c, false));
                    } else if (auto *a =
                                   ::llvm::dyn_cast<::llvm::Argument>(op)) {
                        __add_edge(idx, increase(a, false));
                    } else {
                        std::cerr << "Could not comprehend Operand\n";
                    }
                }
            }
        }

        return idx;
    }

    size_t increase(::llvm::PHINode const *pn, bool insert_adj = false)
    {
        auto [raw_ptr, idx] = __search_and_insert(pn);
        if (insert_adj) {
            for (auto &&bb : pn->blocks()) {
                __add_edge(idx, increase(bb, insert_adj));
                auto *v = pn->getIncomingValueForBlock(bb);
                if (auto *i = ::llvm::dyn_cast<::llvm::Instruction>(v)) {
                    __add_edge(idx, increase(i, insert_adj));
                } else if (auto *c = ::llvm::dyn_cast<::llvm::Constant>(v)) {
                    __add_edge(idx, increase(c, false));
                } else if (auto *a = ::llvm::dyn_cast<::llvm::Argument>(v)) {
                    __add_edge(idx, increase(a, false));
                } else {
                    std::cerr << "Could not comprehend Value\n";
                }
            }
        }

        return idx;
    }

    size_t increase(::llvm::BasicBlock const *bb, bool insert_adj = false)
    {
        auto idx = increase(bb->getTerminator(), insert_adj);
        auto const insts = bb->instructionsWithoutDebug();
        auto first = std::make_reverse_iterator(insts.end());
        auto const last = std::make_reverse_iterator(insts.begin());
        for (; first != last; ++first) {
            increase(std::addressof(*first), insert_adj);
        }

        return idx;
    }

    size_t increase(::llvm::CallInst const *ci, bool insert_adj = false)
    {
        auto [raw_ptr, idx] = __search_and_insert(ci);
        if (insert_adj) {
            for (auto &&arg : ci->args()) {
                if (auto *i = ::llvm::dyn_cast<::llvm::Instruction>(arg)) {
                    __add_edge(idx, increase(i, insert_adj));
                } else if (auto *c = ::llvm::dyn_cast<::llvm::Constant>(arg)) {
                    __add_edge(idx, increase(c, false));
                } else if (auto *a = ::llvm::dyn_cast<::llvm::Argument>(arg)) {
                    __add_edge(idx, increase(a, false));
                } else {
                    std::cerr << "Could not comprehend Argument\n";
                }
            }
        }

        return idx;
    }

    size_t increase(::llvm::Instruction const *i, bool insert_adj = false)
    {
        auto it = ptr_index_map.find(i);
        if (it != ptr_index_map.end() && !insert_adj) {
            return it->second;
        }
        if (it == ptr_index_map.end()) {
            if (auto *n = ::llvm::dyn_cast<::llvm::AtomicCmpXchgInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::AtomicRMWInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::BinaryOperator>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::BranchInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::CatchReturnInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::CatchSwitchInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n =
                           ::llvm::dyn_cast<::llvm::CleanupReturnInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n =
                           ::llvm::dyn_cast<::llvm::ExtractElementInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::FenceInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n =
                           ::llvm::dyn_cast<::llvm::GetElementPtrInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::IndirectBrInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n =
                           ::llvm::dyn_cast<::llvm::InsertElementInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::InsertValueInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::LandingPadInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::PHINode>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::ResumeInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::ReturnInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::SelectInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n =
                           ::llvm::dyn_cast<::llvm::ShuffleVectorInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::StoreInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::SwitchInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::UnreachableInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::CallBrInst>(i)) {
                // directly derived from CallBase
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::CallInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n =
                           ::llvm::dyn_cast<::llvm::GCStatepointInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::InvokeInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::CallBase>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::FCmpInst>(i)) {
                // directly derived from CmpInst
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::ICmpInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::CmpInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::CatchPadInst>(i)) {
                // directly derived from FunclePadInst
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::CleanupPadInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::FuncletPadInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::AllocaInst>(i)) {
                // directly derived from UnaryInstruction
                return increase(n, insert_adj);
            } else if (auto *n =
                           ::llvm::dyn_cast<::llvm::AddrSpaceCastInst>(i)) {
                // directly derived from CastInst
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::BitCastInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::FPExtInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::FPToSIInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::FPToUIInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::FPTruncInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::IntToPtrInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::PtrToIntInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::SExtInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::SIToFPInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::TruncInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::UIToFPInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::ZExtInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n = ::llvm::dyn_cast<::llvm::CastInst>(i)) {
                return increase(n, insert_adj);
            } else if (auto *n =
                           ::llvm::dyn_cast<::llvm::UnaryInstruction>(i)) {
                return increase(n, insert_adj);
            } else {
                std::cerr << "Can't comprehend Instruction\n";
            }
        }

        return it->second;
    }

public:
    llvm_graph(::llvm::BasicBlock const *bb)
        : bb_ptr{ bb }
        , curr_size{ 0 }
    {
        increase(bb, true);
    }

    auto adj_nodes(size_t i) const
    {
        assert(i < curr_size);

        return boost::make_iterator_range(std::get<0>(adj_list[i]).cbegin(),
                                          std::get<0>(adj_list[i]).cend());
    }

    auto inv_adj_nodes(size_t i) const
    {
        assert(i < curr_size);

        return boost::make_iterator_range(std::get<1>(adj_list[i]).cbegin(),
                                          std::get<1>(adj_list[i]).cend());
    }

    size_t size() const
    {
        return curr_size;
    }

    node_type const &attrs(size_t i) const
    {
        assert(i < curr_size);

        return index_node_map.at(i);
    }
};

}

auto get_size(xdp2gen::llvm::llvm_graph const &g) -> size_t
{
    return g.size();
}

auto get_adj_list(xdp2gen::llvm::llvm_graph const &g, size_t i)
{
    return g.adj_nodes(i);
}

auto get_inv_adj_list(xdp2gen::llvm::llvm_graph const &g, size_t i)
{
    return g.inv_adj_nodes(i);
}

auto get_attrs(xdp2gen::llvm::llvm_graph const &g, size_t i)
    -> xdp2gen::llvm::llvm_graph::node_type const &
{
    return g.attrs(i);
}

#endif
